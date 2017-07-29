#include "cli/testdata.h"

#include "persistence/datasource.h"
#include "persistence/sqlite/sqlitestorage.h"

#include <chrono>
#include <iostream>
#include <memory>

void createTestDatabase(const std::string& db)
{

  // Get us some random test data
  auto seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
  std::mt19937 rng(seed);

  // Store all of the random test data into the database
  persistence::DataSource dataSource(db);
  auto waitForAllOperations = [&]() { while (dataSource.hasPendingTasks()) { dataSource.processIntegrationQueue(); std::this_thread::sleep_for(std::chrono::milliseconds(100)); } };
  persistence::VectorDataStreamObserver<hotel::Hotel> hotelsStream;
  auto hotelsStreamHandle = dataSource.connectToStream(&hotelsStream);

  // Store hotels
  {
    dataSource.queueOperation(persistence::op::EraseAllData());
    auto hotels = cli::createTestHotels(rng);
    for (auto& hotel : hotels)
      dataSource.queueOperation(persistence::op::StoreNewHotel{ std::move(hotel) });
    waitForAllOperations();
  }

  // Store reservations
  {
    persistence::op::Operations operations;
    auto planning = cli::createTestPlanning(rng, hotelsStream.items());
    for (auto& reservation : planning->reservations())
      operations.push_back(persistence::op::StoreNewReservation{ std::make_unique<hotel::Reservation>(*reservation) });
    dataSource.queueOperations(std::move(operations));
  }

  waitForAllOperations();

}

int main(int argc, char** argv)
{
  // Fill the test.db with randomly generated test data
  createTestDatabase("test.db");
  return 0;
}
