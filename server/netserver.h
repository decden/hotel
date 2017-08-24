#ifndef SERVER_NETSERVER_H
#define SERVER_NETSERVER_H

#include "persistence/backend.h"

#include <boost/asio.hpp>

#include <vector>
#include <memory>

namespace server
{
  class NetClientSession;

  /**
   * @brief The NetServer class implements a network interface to a backend.
   */
  class NetServer
  {
  public:
    /**
     * @brief Creates a new NetServer instance
     * @param backend The real backend which will handle all of the requests
     */
    NetServer(persistence::Backend & backend);

    boost::asio::io_service& ioService() { return _ioService; }

    /**
     * @brief Runs the server message loop.
     */
    void run();

  private:
    // This function is contiunously being executed to accept new connections
    void doAccept();

  private:
    persistence::Backend& _backend;
    boost::asio::io_service _ioService;
    boost::asio::ip::tcp::endpoint _endpoint;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::ip::tcp::socket _socket;

    std::vector<std::weak_ptr<NetClientSession>> _sessions;
  };

} // namespace server

#endif // SERVER_NETSERVER_H
