#include <boost/asio.hpp>

#include "server/netserver.h"

#include "persistence/sqlite/sqlitebackend.h"

#include <iostream>

int main(int argc, char** argv)
{
  std::cout << "================================================================================" << std::endl;
  std::cout << " hotel_serverapp (listening on port 46835)                                      " << std::endl;
  std::cout << "================================================================================" << std::endl;

  auto dataBackend = std::make_unique<persistence::sqlite::SqliteBackend>("data.db");
  server::NetServer server(std::move(dataBackend));

  std::cout << std::endl << "starting server..." << std::endl;
  server.start();

  // TODO: In future we should have a way to gracefully close the server
  std::this_thread::sleep_for(std::chrono::seconds::max());

  std::cout << std::endl << "Quitting server..." << std::endl;
}
