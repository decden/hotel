#include "webapi/server.h"

#include "json.hpp"

#include <iostream>

namespace webapi
{
  void handleServerEvent(mg_connection* nc, int ev, void* p)
  {
    auto server = (Server*)nc->user_data;
    server->handleEvent(nc, ev, p);
  }

  Server::Server() : _connection(nullptr), _errorStringPtr(nullptr)
  {
    _mongooseManager = {};
    _httpServerSettings = {};
    _httpServerSettings.document_root = ".";
    _httpServerSettings.enable_directory_listing = "yes";
  }

  void Server::start()
  {
    mg_mgr_init(&_mongooseManager, nullptr);

    mg_bind_opts opts = {};
    opts.user_data = this;
    opts.error_string = &_errorStringPtr;

    _connection = mg_bind_opt(&_mongooseManager, "8080", handleServerEvent, opts);
    if (_connection == nullptr)
    {
      std::cerr << "Cannot establish connection: " << _errorStringPtr << std::endl;
      return;
    }
    mg_set_protocol_http_websocket(_connection);

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
      auto msg = (http_message*)p;
      using json = nlohmann::json;

      json jsonObj = {
        {"status", "ok"},
        {"api", "core"},
        {"data", {
         {"version", "0.01"}
        }}
      };

      std::string result  = jsonObj.dump(0);

      mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, result.c_str(), result.length());
    }

    if (ev == MG_EV_HTTP_REQUEST)
    {
      mg_serve_http(nc, (http_message*)p, _httpServerSettings);
    }
  }

} // namespace webapi
