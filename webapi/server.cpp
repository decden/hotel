#include "webapi/server.h"

namespace webapi
{
  static void handleServerEvent(mg_connection* nc, int ev, void* p)
  {
    auto server = (Server*)nc->user_data;
    server->handleEvent(nc, ev, p);
  }

  Server::Server() : _connection(nullptr)
  {
    _mongooseManager = {};
    _httpServerSettings = {};
    _httpServerSettings.document_root = ".";
    _httpServerSettings.enable_directory_listing = "yes";
  }

  void Server::start()
  {
    mg_mgr_init(&_mongooseManager, nullptr);

    mg_bind_opts opts;
    opts.user_data = this;

    _connection = mg_bind_opt(&_mongooseManager, "8080", handleServerEvent, opts);

    for (;;)
    {
      mg_mgr_poll(&_mongooseManager, 1000);
    }

    mg_mgr_free(&_mongooseManager);
  }

  void Server::handleEvent(mg_connection* nc, int ev, void* p)
  {
    if (ev == MG_EV_HTTP_REQUEST)
    {
      mg_serve_http(nc, (http_message*)p, _httpServerSettings);
    }
  }

} // namespace webapi
