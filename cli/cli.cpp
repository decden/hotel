#include "cli/testdata.h"

#include "hotel/persistence/sqlitestorage.h"

#include <mongoose.h>

#include <iostream>
#include <fstream>
#include <chrono>

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

mg_serve_http_opts s_http_server_opts;
void webserverHandler(mg_connection* nc, int ev, void* p)
{
  if (ev == MG_EV_HTTP_REQUEST)
  {
    mg_serve_http(nc, (http_message*)p, s_http_server_opts);
  }
}

void startWebserver()
{
  mg_mgr mgr;
  mg_mgr_init(&mgr, nullptr);
  auto nc = mg_bind(&mgr, "8080", webserverHandler);
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";
  s_http_server_opts.enable_directory_listing = "yes";

  for(;;)
  {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);
}

int main(int argc, char **argv)
{
  // Fill the test.db with randomly generated test data
  createTestDatabase("test.db");

  // Reopen the database
  hotel::persistence::SqliteStorage storage("test.db");
  auto hotels = storage.loadHotels();

  std::vector<int> roomIds;
  for(auto& hotel : hotels)
    for(auto& room : hotel->rooms())
      roomIds.push_back(room->id());
  auto planning = storage.loadPlanning(roomIds);

  hotel::persistence::JsonSerializer jsonSerializer;
  std::ofstream("hotels.json") << jsonSerializer.serializeHotels(hotels) << std::endl;
  std::ofstream("planning.json") << jsonSerializer.serializePlanning(planning) << std::endl;

  startWebserver();

  return 0;
}
