#include "gtest/gtest.h"

#include "persistence/datasource.h"
#include "persistence/op/operations.h"

#include "hotel/hotelcollection.h"

#include <condition_variable>


void waitForAllOperations(persistence::DataSource& ds)
{
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable condition;

  ds.resultsAvailableSignal().connect([&]() { condition.notify_one(); });

  while(ds.pendingOperationCount() != 0)
  {
    ds.processIntegrationQueue();
    condition.wait_for(lock, std::chrono::milliseconds(10));
  }

  ds.resultsAvailableSignal().disconnect_all_slots();
}



class Persistence : public testing::Test
{
public:
  void SetUp() override
  {
    // Make sure the database is empty before each test
    persistence::DataSource dataSource("test.db");
    dataSource.queueOperation(persistence::op::EraseAllData());
    waitForAllOperations(dataSource);
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

  const hotel::Hotel& storeHotel(persistence::DataSource& dataSource, const hotel::Hotel& hotel)
  {
    // TODO: Right now this test function only works for storing one instance
    dataSource.queueOperation(persistence::op::StoreNewHotel { std::make_unique<hotel::Hotel>(hotel) });
    waitForAllOperations(dataSource);
    return *dataSource.hotels().hotels()[0];
  }

  const hotel::Reservation& storeReservation(persistence::DataSource& dataSource, const hotel::Reservation& reservation)
  {
    // TODO: Right now this test function only works for storing one instance
    dataSource.queueOperation(persistence::op::StoreNewReservation{ std::make_unique<hotel::Reservation>(reservation) });
    waitForAllOperations(dataSource);
    return *dataSource.planning().reservations()[0];
  }
};

TEST_F(Persistence, HotelPersistence)
{
  auto hotel = makeNewHotel("Hotel 1", "Category 1", 10);
  auto hotelId = 0;
  // Store hotel
  {
    persistence::DataSource dataSource("test.db");
    auto& storedHotel = storeHotel(dataSource, hotel);

    ASSERT_EQ(hotel, storedHotel);
    hotelId = storedHotel.id();
    ASSERT_NE(0, hotelId);
  }

  // Check data after reopening the database
  {
    persistence::DataSource dataSource("test.db");
    waitForAllOperations(dataSource);
    ASSERT_EQ(1u, dataSource.hotels().hotels().size());
    ASSERT_EQ(hotel, *dataSource.hotels().hotels()[0]);
    ASSERT_EQ(hotelId, dataSource.hotels().hotels()[0]->id());
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
    auto& storedHotel = storeHotel(dataSource, hotel);
    auto roomId = storedHotel.rooms()[0]->id();

    reservation = makeNewReservation("", roomId);
    auto& storedReservation = storeReservation(dataSource, reservation);
    reservationId = storedReservation.id();

    ASSERT_EQ(reservation, storedReservation);
    ASSERT_NE(0, reservationId);
  }

  // Check data after reopening the database
  {
    persistence::DataSource dataSource("test.db");
    waitForAllOperations(dataSource);
    ASSERT_EQ(1u, dataSource.planning().reservations().size());
    ASSERT_EQ(reservation, *dataSource.planning().reservations()[0]);
  }
}

