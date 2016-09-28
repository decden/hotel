#include "cli/testdata.h"

#include "hotel/persistence/jsonserializer.h"
#include "hotel/persistence/sqlitestorage.h"

#include <chrono>
#include <fstream>
#include <iostream>

void createTestDatabase(const std::string& db)
{
  // Get us some random test data
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::mt19937 rng(seed);

  // Store all of the random test data into the database
  hotel::persistence::SqliteStorage storage(db.c_str());
  storage.deleteAll();

  auto hotels = cli::createTestHotels(rng);
  storage.beginTransaction();
  for (auto& hotel : hotels)
    storage.storeNewHotel(*hotel);
  storage.commitTransaction();

  auto planning = cli::createTestPlanning(rng, hotels);
  storage.beginTransaction();
  for (auto& reservation : planning->reservations())
    storage.storeNewReservationAndAtoms(*reservation);
  storage.commitTransaction();
}

int main(int argc, char** argv)
{
  // Fill the test.db with randomly generated test data
  createTestDatabase("test.db");
  return 0;
}
