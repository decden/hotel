#include "webapi/server.h"

#include "hotel/persistence/jsonserializer.h"

#include "json.hpp"

#include <iostream>

namespace webapi
{

  WebsocketConnection::WebsocketConnection(mg_connection* connection) : _connection(connection) {}
  mg_connection* WebsocketConnection::connection() { return _connection; }

  void WebsocketConnection::sendText(const std::string& text)
  {
    mg_send_websocket_frame(_connection, WEBSOCKET_OP_TEXT, text.c_str(), text.length());
  }

  void handleServerEvent(mg_connection* nc, int ev, void* p)
  {
    auto server = (Server*)nc->user_data;
    server->handleEvent(nc, ev, p);
  }

  Server::Server(std::unique_ptr<hotel::persistence::SqliteStorage> storage)
      : _serverConnection(nullptr), _errorStringPtr(nullptr), _wsConnections(), _storage(std::move(storage))
  {
    _mongooseManager = {};
    _httpServerSettings = {};
    _httpServerSettings.document_root = "../../hotel/web";
    _httpServerSettings.enable_directory_listing = "yes";
  }

  void Server::start()
  {
    mg_mgr_init(&_mongooseManager, nullptr);

    mg_bind_opts opts = {};
    opts.user_data = this;
    opts.error_string = &_errorStringPtr;

    _serverConnection = mg_bind_opt(&_mongooseManager, "8080", handleServerEvent, opts);
    if (_serverConnection == nullptr)
    {
      std::cerr << "Cannot establish connection: " << _errorStringPtr << std::endl;
      return;
    }
    mg_set_protocol_http_websocket(_serverConnection);

    for (;;)
    {
      mg_mgr_poll(&_mongooseManager, 1000);
    }

    mg_mgr_free(&_mongooseManager);
  }

  void Server::handleEvent(mg_connection* nc, int ev, void* p)
  {
    if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE)
    {
      auto newWsConnection = std::make_unique<WebsocketConnection>(nc);
      using json = nlohmann::json;

      auto serializer = hotel::persistence::JsonSerializer();
      auto hotels = _storage->loadHotels();
      std::vector<int> roomIds;
      for (auto& hotel : hotels)
        for (auto& room : hotel->rooms())
          roomIds.push_back(room->id());
      auto planning = _storage->loadPlanning(roomIds);
      auto hotelsJson = serializer.serializeHotels(hotels);
      auto reservationsJson = serializer.serializePlanning(planning);

      json msg1 = {{"type", "hotels"}, {"data", hotelsJson}};
      json msg2 = {{"type", "reservations"}, {"data", reservationsJson}};

      newWsConnection->sendText(msg1.dump());
      newWsConnection->sendText(msg2.dump());
      _wsConnections.push_back(std::move(newWsConnection));
    }

    if (ev == MG_EV_CLOSE)
    {
      _wsConnections.erase(std::remove_if(_wsConnections.begin(), _wsConnections.end(),
                                          [nc](auto& conn) { return conn->connection() == nc; }),
                           _wsConnections.end());
    }

    if (ev == MG_EV_HTTP_REQUEST)
    {
      mg_serve_http(nc, (http_message*)p, _httpServerSettings);
    }
  }

} // namespace webapi
