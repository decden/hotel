#include "gtest/gtest.h"

#include "persistence/backend.h"
#include "persistence/changequeue.h"
#include "persistence/sqlite/sqlitebackend.h"
#include "persistence/op/operations.h"
#include "persistence/json/jsonserializer.h"

#include "hotel/hotelcollection.h"

#include <condition_variable>
#include <thread>

void waitForStreamInitialization(persistence::Backend& backend)
{
  while (backend.changeQueue().hasUninitializedStreams())
  {
    backend.changeQueue().applyStreamChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void waitForTask(persistence::Backend& backend, persistence::op::Task<persistence::op::OperationResults>& task)
{
  // Waits for one task to complete and to be integrated
  task.waitForCompletion();
  backend.changeQueue().applyStreamChanges();
  backend.changeQueue().notifyCompletedTasks();
}

class Persistence : public testing::Test
{
public:
  void SetUp() override
  {
    // Make sure the database is empty before each test
    persistence::sqlite::SqliteBackend backend("test.db");
    auto task = backend.queueOperation(persistence::op::EraseAllData());
    waitForTask(backend, task);
  }

  hotel::Hotel makeNewHotel(const std::string& name, const std::string& category, int numberOfRooms)
  {
    hotel::Hotel hotel(name);
    hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>(category, category));
    for (int i = 0; i < numberOfRooms; ++i)
      hotel.addRoom(std::make_unique<hotel::HotelRoom>("Room " + std::to_string(i + 1)), category);
    return hotel;
  }

  hotel::Reservation makeNewReservation(const std::string& description, int roomId)
  {
    using namespace boost::gregorian;
    date_period dateRange(date(2017, 1, 1), date(2017, 1, 1) + days(10));
    auto reservation = hotel::Reservation(description, roomId, dateRange);
    reservation.setStatus(hotel::Reservation::New);
    return reservation;
  }

  void storeHotel(persistence::Backend& backend, const hotel::Hotel& hotel)
  {
    // TODO: Right now this test function only works for storing one instance
    auto task = backend.queueOperation(persistence::op::StoreNewHotel { std::make_unique<hotel::Hotel>(hotel) });
    waitForTask(backend, task);
  }

  void storeReservation(persistence::Backend& backend, const hotel::Reservation& reservation)
  {
    // TODO: Right now this test function only works for storing one instance
    auto task = backend.queueOperation(persistence::op::StoreNewReservation{ std::make_unique<hotel::Reservation>(reservation) });
    waitForTask(backend, task);
  }
};

TEST_F(Persistence, HotelPersistence)
{
  auto hotel = makeNewHotel("Hotel 1", "Category 1", 10);
  auto hotelId = 0;
  // Store hotel
  {
    persistence::sqlite::SqliteBackend backend("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
    storeHotel(backend, hotel);

    ASSERT_EQ(1u, hotels.items().size());
    ASSERT_EQ(hotel, hotels.items()[0]);
    hotelId = hotels.items()[0].id();
    ASSERT_NE(0, hotelId);
  }

  // Check data after reopening the database
  {
    persistence::sqlite::SqliteBackend backend("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
    waitForStreamInitialization(backend);

    ASSERT_EQ(1u, hotels.items().size());
    ASSERT_EQ(hotel, hotels.items()[0]);
    ASSERT_EQ(hotelId, hotels.items()[0].id());
  }
}

TEST_F(Persistence, ReservationPersistence)
{
  auto hotel = makeNewHotel("Hotel 1", "Category 1", 10);
  hotel::Reservation reservation("");
  auto reservationId = 0;
  // Store reservation
  {
    persistence::sqlite::SqliteBackend backend("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
    storeHotel(backend, hotel);

    ASSERT_EQ(1u, hotels.items().size());
    auto roomId = hotels.items()[0].rooms()[0]->id();

    persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
    auto reservationsStreamHandle = backend.createStreamTyped(&reservations);
    reservation = makeNewReservation("", roomId);
    storeReservation(backend, reservation);

    reservationId = reservations.items()[0].id();

    ASSERT_EQ(reservation, reservations.items()[0]);
    ASSERT_NE(0, reservationId);
  }

  // Check data after reopening the database
  {
    persistence::sqlite::SqliteBackend backend("test.db");
    persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
    auto reservationsStreamHandle = backend.createStreamTyped(&reservations);
    waitForStreamInitialization(backend);

    ASSERT_EQ(1u, reservations.items().size());
    ASSERT_EQ(reservation, reservations.items()[0]);
  }
}

TEST_F(Persistence, VersionConflicts)
{
  persistence::sqlite::SqliteBackend backend("test.db");
  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  auto hotelStreamHandle = backend.createStreamTyped(&hotels);
  storeHotel(backend, makeNewHotel("Hotel 1", "Category 1", 10));

  // Test if updates to wrong revisions are rejected
  auto changedHotel1 = hotels.items()[0];
  changedHotel1.setName("Changed Hotel Name 1");
  auto changedHotel2 = hotels.items()[0];
  changedHotel2.setName("Changed Hotel Name 2");
  auto task1 = backend.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel1))});
  auto task2 = backend.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(backend, task1);
  waitForTask(backend, task2);
  ASSERT_EQ(persistence::op::OperationResultStatus::Successful, task1.results()[0].status);
  ASSERT_EQ(persistence::op::OperationResultStatus::Error, task2.results()[0].status);

  // Trying to make the same change to the correct revision now works
  changedHotel2 = hotels.items()[0];
  changedHotel2.setName("Changed Hotel Name 2");
  task2 = backend.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(backend, task2);
  ASSERT_EQ(persistence::op::OperationResultStatus::Successful, task2.results()[0].status);
}

TEST_F(Persistence, DataStreams)
{
  persistence::sqlite::SqliteBackend backend("test.db");

  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
  auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
  auto reservationsStreamHandle = backend.createStreamTyped(&reservations);

  // Adding items
  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  storeHotel(backend, makeNewHotel("Hotel 1", "Category 1", 10));
  ASSERT_EQ(1u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  storeReservation(backend, makeNewReservation("", hotels.items()[0].rooms()[0]->id()));
  storeReservation(backend, makeNewReservation("", hotels.items()[0].rooms()[1]->id()));
  ASSERT_EQ(1u, hotels.items().size());
  ASSERT_EQ(2u, reservations.items().size());

  // Updating streams
  auto updatedReservation = reservations.items()[0];
  updatedReservation.setDescription("Updated Reservation Description");
  auto updateTask = backend.queueOperation(persistence::op::UpdateReservation{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(backend, updateTask);
  ASSERT_EQ(reservations.items()[0], updatedReservation);

  auto task = backend.queueOperation(persistence::op::EraseAllData());
  waitForTask(backend, task);

  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
}

TEST_F(Persistence, DataStreamsServices)
{
  persistence::sqlite::SqliteBackend backend("test.db");

  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
  auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
  auto reservationsStreamHandle = backend.createStreamTyped(&reservations);

  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());

  // Add items
  storeHotel(backend, makeNewHotel("Hotel 1", "Category 1", 10));
  storeHotel(backend, makeNewHotel("Hotel 2", "Category 2", 11));
  storeReservation(backend, makeNewReservation("Test", hotels.items()[0].rooms()[0]->id()));
  storeReservation(backend, makeNewReservation("Test", hotels.items()[1].rooms()[0]->id()));
  ASSERT_EQ(2u, hotels.items().size());
  ASSERT_EQ(2u, reservations.items().size());

  // Singnle-id stream - Initial data
  persistence::VectorDataStreamObserver<hotel::Hotel> hotel;
  nlohmann::json streamOptions;
  streamOptions["id"] = hotels.items()[1].id();
  auto hotelStreamHandle = backend.createStreamTyped(&hotel, "hotel.by_id", streamOptions);
  waitForStreamInitialization(backend);
  ASSERT_EQ(1u, hotel.items().size());
  ASSERT_EQ(hotels.items()[1], hotel.items()[0]);

  persistence::VectorDataStreamObserver<hotel::Reservation> reservation;
  streamOptions["id"] = reservations.items()[1].id();
  auto reservationStreamHandle = backend.createStreamTyped(&reservation, "reservation.by_id", streamOptions);
  waitForStreamInitialization(backend);
  ASSERT_EQ(1u, reservation.items().size());
  ASSERT_EQ(reservations.items()[1], reservation.items()[0]);

  // Test that adding an unrelated item does not affect the single-id stream
  storeHotel(backend, makeNewHotel("Hotel 3", "Category 3", 10));
  ASSERT_EQ(1u, hotel.items().size());

  // Test that item modifications affect single-id streams
  auto updatedReservation = reservation.items()[0];
  updatedReservation.setDescription("Updated Reservation Description");
  auto updateTask = backend.queueOperation(persistence::op::UpdateReservation{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(backend, updateTask);
  ASSERT_EQ(reservation.items()[0], updatedReservation);

  // Test that removing all items clears all streams
  auto task = backend.queueOperation(persistence::op::EraseAllData());
  waitForTask(backend, task);
  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, hotel.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  ASSERT_EQ(0u, reservation.items().size());
}

TEST_F(Persistence, Serialization)
{
  hotel::Hotel hotelOrig("hello");
  hotelOrig.setId(42);
  hotelOrig.setRevision(4200);
  auto hotelCopy = persistence::json::deserialize<hotel::Hotel>(persistence::json::serialize(hotelOrig));
  ASSERT_EQ(hotelOrig, hotelCopy);
  ASSERT_EQ(hotelOrig.id(), hotelCopy.id());
  ASSERT_EQ(hotelOrig.revision(), hotelCopy.revision());

  hotel::Reservation reservationOrig(
      "Test reservation", 123,
      boost::gregorian::date_period(boost::gregorian::date(2017, 8, 20), boost::gregorian::date(2017, 9, 10)));
  reservationOrig.setStatus(hotel::Reservation::CheckedIn);
  reservationOrig.setId(42);
  reservationOrig.setRevision(4200);
  auto reservationCopy = persistence::json::deserialize<hotel::Reservation>(persistence::json::serialize(reservationOrig));
  ASSERT_EQ(reservationCopy, reservationOrig);
  ASSERT_EQ(reservationCopy.id(), reservationOrig.id());
  ASSERT_EQ(reservationCopy.revision(), reservationOrig.revision());
}
