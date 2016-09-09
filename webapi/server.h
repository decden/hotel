#ifndef WEBAPI_SERVER_H
#define WEBAPI_SERVER_H

#include "hotel/persistence/sqlitestorage.h"

#include "mongoose.h"

#include <memory>
#include <vector>

namespace webapi
{
  class WebsocketConnection
  {
  public:
    WebsocketConnection(mg_connection* connection);

    mg_connection* connection();

    void sendText(const std::string& text);

  private:
    mg_connection* _connection;
  };

  class Server
  {
  public:
    Server(std::unique_ptr<hotel::persistence::SqliteStorage> storage);

    void start();
    void handleEvent(mg_connection* nc, int ev, void* p);

  private:
    // Webserver connection data
    mg_mgr _mongooseManager;
    mg_serve_http_opts _httpServerSettings;
    mg_connection* _serverConnection;
    const char* _errorStringPtr;

    // List of connected websocekt clients
    std::vector<std::unique_ptr<WebsocketConnection>> _wsConnections;

    // The storage object, used to interact with the database
    std::unique_ptr<hotel::persistence::SqliteStorage> _storage;
  };

} // namespaec webapi

#endif // WEBAPI_SERVER_H
