#include "gtest/gtest.h"

#include "server/netserver.h"

#include "persistence/backend.h"
#include "persistence/changequeue.h"
#include "persistence/simpletaskobserver.h"
#include "persistence/sqlite/sqlitebackend.h"
#include "persistence/op/operations.h"
#include "persistence/json/jsonserializer.h"
#include "persistence/net/netclientbackend.h"

#include "hotel/hotelcollection.h"

#include <condition_variable>
#include <thread>

void waitForStreamInitialization(persistence::Backend& backend)
{
  while (backend.changeQueue().hasUninitializedStreams())
  {
    backend.changeQueue().applyStreamChanges();
    backend.changeQueue().applyTaskChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void waitForTask(persistence::Backend& backend, persistence::Task& task)
{
  while (!task.isCompleted())
  {
    backend.changeQueue().applyStreamChanges();
    backend.changeQueue().applyTaskChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

class Persistence : public testing::Test
{
public:
  void SetUp() override
  {
    // Make sure the database is empty before each test
    persistence::sqlite::SqliteBackend backend("test.db");
    auto handle = backend.queueOperation(persistence::op::EraseAllData());
    waitForTask(backend, *handle.task());
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
    auto handle = backend.queueOperation(persistence::op::StoreNew{ std::make_unique<hotel::Hotel>(hotel) });
    waitForTask(backend, *handle.task());
  }

  void storeReservation(persistence::Backend& backend, const hotel::Reservation& reservation)
  {
    // TODO: Right now this test function only works for storing one instance
    auto handle = backend.queueOperation(persistence::op::StoreNew{ std::make_unique<hotel::Reservation>(reservation) });
    waitForTask(backend, *handle.task());
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
  persistence::SimpleTaskObserver task1(backend, persistence::op::Update{std::make_unique<hotel::Hotel>(std::move(changedHotel1))});
  persistence::SimpleTaskObserver task2(backend, persistence::op::Update{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(backend, *task1.task());
  waitForTask(backend, *task2.task());
  ASSERT_EQ(persistence::TaskResultStatus::Successful, task1.results()[0].status);
  ASSERT_EQ(persistence::TaskResultStatus::Error, task2.results()[0].status);

  // Trying to make the same change to the correct revision now works
  changedHotel2 = hotels.items()[0];
  changedHotel2.setName("Changed Hotel Name 2");
  persistence::SimpleTaskObserver task3(backend, persistence::op::Update{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(backend, *task3.task());
  ASSERT_EQ(persistence::TaskResultStatus::Successful, task3.results()[0].status);
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
  auto updateTask = backend.queueOperation(persistence::op::Update{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(backend, *updateTask.task());
  ASSERT_EQ(reservations.items()[0], updatedReservation);

  auto task = backend.queueOperation(persistence::op::EraseAllData());
  waitForTask(backend, *task.task());

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
  auto updateTask = backend.queueOperation(persistence::op::Update{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(backend, *updateTask.task());
  ASSERT_EQ(reservation.items()[0], updatedReservation);

  // Test that removing all items clears all streams
  auto task = backend.queueOperation(persistence::op::EraseAllData());
  waitForTask(backend, *task.task());
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

TEST_F(Persistence, Net)
{
  server::NetServer server(std::make_unique<persistence::sqlite::SqliteBackend>("test.db"));
  server.start();

  persistence::net::NetClientBackend backend("localhost", 46835);
  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
  auto hotelsStreamHandle = backend.createStreamTyped(&hotels);
  auto reservationsStreamHandle = backend.createStreamTyped(&reservations);
  waitForStreamInitialization(backend);

  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());

  // Create new items
  auto newHotel = makeNewHotel("Hotel 1", "Category 1", 10);
  storeHotel(backend, newHotel);
  auto newReservation1 = makeNewReservation("Reservation 1", hotels.items()[0].rooms()[0]->id());
  auto newReservation2 = makeNewReservation("Reservation 2", hotels.items()[0].rooms()[1]->id());
  storeReservation(backend, newReservation1);
  storeReservation(backend, newReservation2);

  ASSERT_EQ(1u, hotels.items().size());
  ASSERT_EQ(newHotel, hotels.items()[0]);
  ASSERT_EQ(2u, reservations.items().size());
  ASSERT_EQ(newReservation1, reservations.items()[0]);
  ASSERT_EQ(newReservation2, reservations.items()[1]);

  // Update existing items
  auto updatedHotel = hotels.items()[0];
  updatedHotel.setName("Fancy Hotel 1");
  persistence::SimpleTaskObserver updateTask1(backend, persistence::op::Update{std::make_unique<hotel::Hotel>(updatedHotel)});
  waitForTask(backend, *updateTask1.task());
  auto updatedReservation = reservations.items()[0];
  updatedReservation.setDescription("Hello!");
  persistence::SimpleTaskObserver updateTask2(backend, persistence::op::Update{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(backend, *updateTask2.task());

  ASSERT_EQ(1u, hotels.items().size());
  // TODO: This does not yet pass, the update process duplicates some of the data (e.g. rooms and categories)
  //ASSERT_EQ(updatedHotel, hotels.items()[0]);
  ASSERT_EQ(2u, reservations.items().size());
  ASSERT_EQ(updatedReservation, reservations.items()[0]);
  ASSERT_EQ(newReservation2, reservations.items()[1]);
}
