#include "server/netserver.h"

#include "server/netclientsession.h"

#include "persistence/changequeue.h"

namespace server
{

  NetServer::NetServer(std::unique_ptr<persistence::Backend> backend)
      : _backend(std::move(backend)), _ioService(), _endpoint(boost::asio::ip::tcp::v4(), 8081), _acceptor(_ioService, _endpoint),
        _socket(_ioService)
  {
    // Configure backend to apply changes in the ioservice thread (server thread)
    auto handleChanges = [this]()
    {
      _ioService.post([this](){
        _backend->changeQueue().applyStreamChanges();
      });
    };
    _backend->changeQueue().connectToStreamChangesAvailableSignal(handleChanges);

    // Start accepting connections
    doAccept();
  }

  NetServer::~NetServer()
  {
    stopAndJoin();
    _backend = nullptr;
  }

  void NetServer::start()
  {
    assert(!_serverThread.joinable());

    // Start processing io operations on a separate thread
    _serverThread = std::thread([this](){
      _ioService.run();
    });
  }

  void NetServer::stopAndJoin()
  {
    if (!_serverThread.joinable())
      return;

    // Stop accepting connections and close the existing connections
    _ioService.post([this](){
      _acceptor.close();
      for (auto& weakSession : _sessions)
      {
        auto session = weakSession.lock();
        if (session != nullptr)
          session->close();
      }
    });

    _serverThread.join();
  }

  void NetServer::run() { _ioService.run(); }

  void NetServer::doAccept()
  {
    // Create a new blank session
    auto session = std::make_shared<server::NetClientSession>(_ioService, *_backend);
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

} // namespace server
