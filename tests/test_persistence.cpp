#include "gtest/gtest.h"

#include "persistence/datasource.h"
#include "persistence/op/operations.h"

#include "hotel/hotelcollection.h"

#include <condition_variable>
#include <thread>

void waitForStreamInitialization(persistence::DataSource& ds)
{
  while(ds.changeQueue().hasUninitializedStreams())
  {
    ds.changeQueue().applyStreamChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void waitForTask(persistence::DataSource& ds, persistence::op::Task<persistence::op::OperationResults>& task)
{
  // Waits for one task to complete and to be integrated
  task.waitForCompletion();
  ds.changeQueue().applyStreamChanges();
  ds.changeQueue().notifyCompletedTasks();
}

class Persistence : public testing::Test
{
public:
  void SetUp() override
  {
    // Make sure the database is empty before each test
    persistence::DataSource dataSource("test.db");
    auto task = dataSource.queueOperation(persistence::op::EraseAllData());
    waitForTask(dataSource, task);
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

  void storeHotel(persistence::DataSource& dataSource, const hotel::Hotel& hotel)
  {
    // TODO: Right now this test function only works for storing one instance
    auto task = dataSource.queueOperation(persistence::op::StoreNewHotel { std::make_unique<hotel::Hotel>(hotel) });
    waitForTask(dataSource, task);
  }

  void storeReservation(persistence::DataSource& dataSource, const hotel::Reservation& reservation)
  {
    // TODO: Right now this test function only works for storing one instance
    auto task = dataSource.queueOperation(persistence::op::StoreNewReservation{ std::make_unique<hotel::Reservation>(reservation) });
    waitForTask(dataSource, task);
  }
};

TEST_F(Persistence, HotelPersistence)
{
  auto hotel = makeNewHotel("Hotel 1", "Category 1", 10);
  auto hotelId = 0;
  // Store hotel
  {
    persistence::DataSource dataSource("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = dataSource.connectToStream(&hotels);
    storeHotel(dataSource, hotel);

    ASSERT_EQ(1u, hotels.items().size());
    ASSERT_EQ(hotel, hotels.items()[0]);
    hotelId = hotels.items()[0].id();
    ASSERT_NE(0, hotelId);
  }

  // Check data after reopening the database
  {
    persistence::DataSource dataSource("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = dataSource.connectToStream(&hotels);
    waitForStreamInitialization(dataSource);

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
    persistence::DataSource dataSource("test.db");
    persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
    auto hotelsStreamHandle = dataSource.connectToStream(&hotels);
    storeHotel(dataSource, hotel);

    ASSERT_EQ(1u, hotels.items().size());
    auto roomId = hotels.items()[0].rooms()[0]->id();

    persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
    auto reservationsStreamHandle = dataSource.connectToStream(&reservations);
    reservation = makeNewReservation("", roomId);
    storeReservation(dataSource, reservation);

    reservationId = reservations.items()[0].id();

    ASSERT_EQ(reservation, reservations.items()[0]);
    ASSERT_NE(0, reservationId);
  }

  // Check data after reopening the database
  {
    persistence::DataSource dataSource("test.db");
    persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
    auto reservationsStreamHandle = dataSource.connectToStream(&reservations);
    waitForStreamInitialization(dataSource);

    ASSERT_EQ(1u, reservations.items().size());
    ASSERT_EQ(reservation, reservations.items()[0]);
  }
}

TEST_F(Persistence, VersionConflicts)
{
  persistence::DataSource dataSource("test.db");
  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  auto hotelStreamHandle = dataSource.connectToStream(&hotels);
  storeHotel(dataSource, makeNewHotel("Hotel 1", "Category 1", 10));

  // Test if updates to wrong revisions are rejected
  auto changedHotel1 = hotels.items()[0];
  changedHotel1.setName("Changed Hotel Name 1");
  auto changedHotel2 = hotels.items()[0];
  changedHotel2.setName("Changed Hotel Name 2");
  auto task1 = dataSource.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel1))});
  auto task2 = dataSource.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(dataSource, task1);
  waitForTask(dataSource, task2);
  ASSERT_EQ(persistence::op::OperationResultStatus::Successful, task1.results()[0].status);
  ASSERT_EQ(persistence::op::OperationResultStatus::Error, task2.results()[0].status);

  // Trying to make the same change to the correct revision now works
  changedHotel2 = hotels.items()[0];
  changedHotel2.setName("Changed Hotel Name 2");
  task2 = dataSource.queueOperation(persistence::op::UpdateHotel{std::make_unique<hotel::Hotel>(std::move(changedHotel2))});
  waitForTask(dataSource, task2);
  ASSERT_EQ(persistence::op::OperationResultStatus::Successful, task2.results()[0].status);
}

TEST_F(Persistence, DataStreams)
{
  persistence::DataSource dataSource("test.db");

  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
  auto hotelsStreamHandle = dataSource.connectToStream(&hotels);
  auto reservationsStreamHandle = dataSource.connectToStream(&reservations);

  // Adding items
  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  storeHotel(dataSource, makeNewHotel("Hotel 1", "Category 1", 10));
  ASSERT_EQ(1u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  storeReservation(dataSource, makeNewReservation("", hotels.items()[0].rooms()[0]->id()));
  storeReservation(dataSource, makeNewReservation("", hotels.items()[0].rooms()[1]->id()));
  ASSERT_EQ(1u, hotels.items().size());
  ASSERT_EQ(2u, reservations.items().size());

  // Updating streams
  auto updatedReservation = reservations.items()[0];
  updatedReservation.setDescription("Updated Reservation Description");
  auto updateTask = dataSource.queueOperation(persistence::op::UpdateReservation{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(dataSource, updateTask);
  ASSERT_EQ(reservations.items()[0], updatedReservation);

  auto task = dataSource.queueOperation(persistence::op::EraseAllData());
  waitForTask(dataSource, task);

  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());
}

TEST_F(Persistence, DataStreamsServices)
{
  persistence::DataSource dataSource("test.db");

  persistence::VectorDataStreamObserver<hotel::Hotel> hotels;
  persistence::VectorDataStreamObserver<hotel::Reservation> reservations;
  auto hotelsStreamHandle = dataSource.connectToStream(&hotels);
  auto reservationsStreamHandle = dataSource.connectToStream(&reservations);

  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, reservations.items().size());

  // Add items
  storeHotel(dataSource, makeNewHotel("Hotel 1", "Category 1", 10));
  storeHotel(dataSource, makeNewHotel("Hotel 2", "Category 2", 11));
  storeReservation(dataSource, makeNewReservation("Test", hotels.items()[0].rooms()[0]->id()));
  storeReservation(dataSource, makeNewReservation("Test", hotels.items()[1].rooms()[0]->id()));
  ASSERT_EQ(2u, hotels.items().size());
  ASSERT_EQ(2u, reservations.items().size());

  // Singnle-id stream - Initial data
  persistence::VectorDataStreamObserver<hotel::Hotel> hotel;
  nlohmann::json streamOptions;
  streamOptions["id"] = hotels.items()[1].id();
  auto hotelStreamHandle = dataSource.connectToStream(&hotel, "hotel.by_id", streamOptions);
  waitForStreamInitialization(dataSource);
  ASSERT_EQ(1u, hotel.items().size());
  ASSERT_EQ(hotels.items()[1], hotel.items()[0]);

  persistence::VectorDataStreamObserver<hotel::Reservation> reservation;
  streamOptions["id"] = reservations.items()[1].id();
  auto reservationStreamHandle = dataSource.connectToStream(&reservation, "reservation.by_id", streamOptions);
  waitForStreamInitialization(dataSource);
  ASSERT_EQ(1u, reservation.items().size());
  ASSERT_EQ(reservations.items()[1], reservation.items()[0]);

  // Test that adding an unrelated item does not affect the single-id stream
  storeHotel(dataSource, makeNewHotel("Hotel 3", "Category 3", 10));
  ASSERT_EQ(1u, hotel.items().size());

  // Test that item modifications affect single-id streams
  auto updatedReservation = reservation.items()[0];
  updatedReservation.setDescription("Updated Reservation Description");
  auto updateTask = dataSource.queueOperation(persistence::op::UpdateReservation{std::make_unique<hotel::Reservation>(updatedReservation)});
  waitForTask(dataSource, updateTask);
  ASSERT_EQ(reservation.items()[0], updatedReservation);

  // Test that removing all items clears all streams
  auto task = dataSource.queueOperation(persistence::op::EraseAllData());
  waitForTask(dataSource, task);
  ASSERT_EQ(0u, hotels.items().size());
  ASSERT_EQ(0u, hotel.items().size());
  ASSERT_EQ(0u, reservations.items().size());
  ASSERT_EQ(0u, reservation.items().size());

}
