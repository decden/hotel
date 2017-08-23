#include "serverapp/netserver.h"

#include "serverapp/netclientsession.h"

namespace serverapp
{

  NetServer::NetServer(persistence::Backend &backend)
      : _backend(backend), _ioService(), _endpoint(boost::asio::ip::tcp::v4(), 46835), _acceptor(_ioService, _endpoint),
        _socket(_ioService)
  {
    doAccept();
  }

  void NetServer::run() { _ioService.run(); }

  void NetServer::doAccept()
  {
    // Create a new blank session
    auto session = std::make_shared<serverapp::NetClientSession>(_ioService, _backend);
    _sessions.push_back(session);

    // Wait for some client to accept the connection
    _acceptor.async_accept(session->socket(), [this, session](const boost::system::error_code& ec) {
      if (!ec)
      {
        session->start();
        std::cout << " [+] Accepted connection from " << session->socket().remote_endpoint().address().to_string() << std::endl;
        this->doAccept();
      }
    });
  }

} // namespace serverapp
