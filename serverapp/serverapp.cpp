#include <boost/asio.hpp>

#include "server/netserver.h"

#include "persistence/sqlite/sqlitebackend.h"

#include <iostream>

int main(int argc, char** argv)
{
  std::cout << "================================================================================" << std::endl;
  std::cout << " hotel_serverapp (listening on port 46835)                                      " << std::endl;
  std::cout << "================================================================================" << std::endl;

  persistence::sqlite::SqliteBackend dataBackend("data.db");
  server::NetServer server(dataBackend);

  auto handleChanges = [&]()
  {
    server.ioService().post([&](){
      dataBackend.changeQueue().applyStreamChanges();
      dataBackend.changeQueue().applyTaskChanges();
    });
  };
  dataBackend.changeQueue().connectToStreamChangesAvailableSignal(handleChanges);
  dataBackend.changeQueue().connectToTaskCompletedSignal(handleChanges);
  std::cout << std::endl << "starting server..." << std::endl;

  server.run();
}
