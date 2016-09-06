#ifndef WEBAPI_SERVER_H
#define WEBAPI_SERVER_H

#include <mongoose.h>

#include <memory>

namespace webapi
{

  class Server
  {
  public:
    Server();

    void start();

    void handleEvent(mg_connection* nc, int ev, void* p);

  private:
    mg_mgr _mongooseManager;
    mg_serve_http_opts _httpServerSettings;
    mg_connection* _connection;
  };

} // namespaec webapi

#endif // WEBAPI_SERVER_H
