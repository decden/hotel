#include "guiapp/testdata.h"

#include "persistence/backend.h"
#include "persistence/changequeue.h"

#include "hotel/reservation.h"

#include <boost/range/irange.hpp>

#include <algorithm>
#include <random>

namespace guiapp
{

  std::vector<std::unique_ptr<hotel::Hotel>> createTestHotels(std::mt19937& rng)
  {
    std::vector<std::unique_ptr<hotel::Hotel>> result;
    std::uniform_int_distribution<> hotels_dist(2, 2);
    std::uniform_int_distribution<> floors_dist(1, 3);
    std::uniform_int_distribution<> rooms_dist(1, 10);

    int numOfHotels = hotels_dist(rng);
    for (auto i : boost::irange(0, numOfHotels))
    {
      auto hotel = std::make_unique<hotel::Hotel>("Hotel " + std::to_string(i));
      auto numOfFloors = floors_dist(rng);
      auto numOfCategories = numOfFloors / 3 + 1;
      for (auto c : boost::irange(0, numOfCategories))
        hotel->addRoomCategory(
            std::make_unique<hotel::RoomCategory>("cat" + std::to_string(c), "Category " + std::to_string(c)));

      auto category_dist = std::uniform_int_distribution<>(0, numOfCategories - 1);
      for (auto f : boost::irange(0, numOfFloors))
      {
        auto numOfRooms = rooms_dist(rng);
        for (auto r : boost::irange(0, numOfRooms))
        {
          auto category = "cat" + std::to_string(category_dist(rng));
          auto roomNumber = std::to_string(i) + "_" + std::to_string(100 * (1 + f) + r + 1);
          hotel->addRoom(std::make_unique<hotel::HotelRoom>(roomNumber), category);
        }
      }
      result.push_back(std::move(hotel));
    }

    return result;
  }

  void addRandomReservations(std::mt19937& rng, const hotel::Hotel& hotel, hotel::PlanningBoard& planning, int count,
                             boost::gregorian::date_period period)
  {
    // std::uniform_int_distribution<> dayDist(0, period.length().days());
    std::normal_distribution<> dayDist(period.length().days() / 4, period.length().days() / 2);
    std::uniform_int_distribution<> roomDist(0, hotel.rooms().size() - 1);
    std::uniform_int_distribution<> lengthDist(3, 21);
    std::uniform_int_distribution<> percentageDist(0, 100);

    for (auto i : boost::irange(0, count))
    {
      auto day = dayDist(rng);
      auto length = lengthDist(rng);
      if (percentageDist(rng) < 70)
      {
        // Snap to whole weeks
        day = std::floor(day / 7) * 7;
        length = std::ceil(length / 7.0) * 7;
      }

      auto startDate = period.begin() + boost::gregorian::days(day);
      auto endDate = startDate + boost::gregorian::days(length);
      auto resPeriod = boost::gregorian::date_period(startDate, endDate);
      auto additionalDays = 0;
      if (!period.contains(resPeriod))
        continue;

      auto roomId = hotel.rooms()[roomDist(rng)]->id();
      if (!planning.isFree(roomId, resPeriod))
      {
        // Try to add it with room change
        auto availableDaysInRoom = planning.getAvailableDaysFrom(roomId, resPeriod.begin());
        if (availableDaysInRoom == 0)
          continue; // Room is fully booked!

        additionalDays = (endDate - startDate).days() - availableDaysInRoom;
        endDate = startDate + boost::gregorian::days(availableDaysInRoom);
        resPeriod = boost::gregorian::date_period(startDate, endDate);
      }

      auto reservation = std::make_unique<hotel::Reservation>("Reservation " + std::to_string(i), roomId, resPeriod);
      // Fill in any missing days by changing rooms (10 attempts)
      for (int i = 0; i < 10; ++i)
      {
        startDate = endDate;
        if (additionalDays == 0)
          break;
        roomId = hotel.rooms()[roomDist(rng)]->id();
        auto availableDays = std::min(additionalDays, planning.getAvailableDaysFrom(roomId, startDate));
        if (availableDays != 0)
        {
          endDate = startDate + boost::gregorian::days(availableDays);
          reservation->addContinuation(roomId, endDate);
          additionalDays -= availableDays;
        }
      }

      // If we manage to account for all of the requested days
      if (additionalDays == 0)
      {
        // Set the reservation status
        auto today = boost::gregorian::day_clock::local_day();
        if (reservation->dateRange().contains(today))
          reservation->setStatus(hotel::Reservation::CheckedIn);
        else if (reservation->dateRange().end() < today + boost::gregorian::days(-5))
          reservation->setStatus(hotel::Reservation::Archived);
        else if (reservation->dateRange().end() <= today)
          reservation->setStatus(hotel::Reservation::CheckedOut);
        else if (percentageDist(rng) < 90)
          reservation->setStatus(hotel::Reservation::Confirmed);
        else
          reservation->setStatus(hotel::Reservation::New);

        planning.addReservation(std::move(reservation));
      }
    }
  }

  std::unique_ptr<hotel::PlanningBoard> createTestPlanning(std::mt19937& rng,
                                                           const std::vector<hotel::Hotel>& hotels)
  {
    using namespace boost::gregorian;
    auto today = day_clock::local_day();
    auto periodFrom = today + days(-14);
    auto periodTo = today + days(600);

    auto period = boost::gregorian::date_period(periodFrom, periodTo);

    auto planning = std::make_unique<hotel::PlanningBoard>();

    for (auto& hotel : hotels)
      addRandomReservations(rng, hotel, *planning, 20 * hotel.rooms().size(), period);

    return planning;
  }

  void waitForTasks(persistence::Backend& backend, std::vector<persistence::op::Task<persistence::op::OperationResults>>& pendingTasks)
  {
    std::for_each(pendingTasks.begin(), pendingTasks.end(),
                  [](persistence::op::Task<persistence::op::OperationResults>& task) { task.waitForCompletion(); });
    backend.changeQueue().applyStreamChanges();
  }

  void createTestData(persistence::Backend &backend)
  {

    // Get us some random test data
    auto seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);

    // Store all of the random test data into the database
    persistence::VectorDataStreamObserver<hotel::Hotel> hotelsStream;
    auto hotelsStreamHandle = backend.createStreamTyped(&hotelsStream);

    // Store hotels
    {
      std::vector<persistence::op::Task<persistence::op::OperationResults>> pendingTasks;
      backend.queueOperation(persistence::op::EraseAllData());
      auto hotels = createTestHotels(rng);
      for (auto& hotel : hotels)
        pendingTasks.push_back(backend.queueOperation(persistence::op::StoreNewHotel{std::move(hotel)}));
      waitForTasks(backend, pendingTasks);
    }

    // Store reservations
    {
      persistence::op::Operations operations;
      auto planning = createTestPlanning(rng, hotelsStream.items());
      for (auto& reservation : planning->reservations())
        operations.push_back(persistence::op::StoreNewReservation{ std::make_unique<hotel::Reservation>(*reservation) });
      std::vector<persistence::op::Task<persistence::op::OperationResults>> pendingTasks;
      pendingTasks.push_back(backend.queueOperations(std::move(operations)));
      waitForTasks(backend, pendingTasks);
    }
  }

} // namespace guiapp
