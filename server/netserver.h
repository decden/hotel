#ifndef SERVER_NETSERVER_H
#define SERVER_NETSERVER_H

#include "persistence/backend.h"

#include <boost/asio.hpp>

#include <vector>
#include <memory>
#include <thread>

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
    NetServer(std::unique_ptr<persistence::Backend> backend);
    ~NetServer();

    void start();
    void stopAndJoin();

  private:
    /**
     * @brief Runs the server message loop.
     */
    void run();

    // This function is contiunously being executed to accept new connections
    void doAccept();

  private:
    std::thread _serverThread;

    std::unique_ptr<persistence::Backend> _backend;
    boost::asio::io_service _ioService;
    boost::asio::ip::tcp::endpoint _endpoint;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::ip::tcp::socket _socket;

    std::vector<std::weak_ptr<NetClientSession>> _sessions;
  };

} // namespace server

#endif // SERVER_NETSERVER_H
