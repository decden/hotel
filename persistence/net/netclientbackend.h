#ifndef PERSISTENCE_NET_NETCLIENTBACKEND_H
#define PERSISTENCE_NET_NETCLIENTBACKEND_H

#include "persistence/backend.h"
#include "persistence/changequeue.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <thread>
#include <atomic>
#include <condition_variable>
#include <map>

namespace persistence
{
  namespace net
  {
    /**
     * @brief The NetClientBackend class is a backend which connects to a remote server
     *
     * This particular backend allows multiple clients to work on the same data, by having a single central server
     * which provides the data and executes the operations;
     */
    class NetClientBackend final : public Backend
    {
    public:
      NetClientBackend(const std::string& host, int port);
      virtual ~NetClientBackend();

      virtual ChangeQueue& changeQueue() override;
      virtual UniqueTaskHandle queueOperations(op::Operations operations, TaskObserver* observer = nullptr) override;

      virtual persistence::UniqueDataStreamHandle createStream(DataStreamObserver* observer, StreamableType type,
                                                               const std::string& service,
                                                               const nlohmann::json& options) override;

    protected:
      virtual void removeStream(std::shared_ptr<persistence::DataStream> stream) override;
      virtual void removeTask(std::shared_ptr<persistence::Task> task) override;

    private:
      void start();
      void stopAndJoin();

      // Internal interface for higher level communication operations
      void submit(std::string message);

      // Server communication
      void socketConnected(boost::system::error_code ec);
      void doSumitMessages();
      void doReadNextMessageHeader();
      void doReadNextMessageBody(size_t size);

      void processMessage(const nlohmann::json& obj);


      std::array<char, 4> _headerBuffer;
      std::vector<char> _bodyBuffer;
      std::map<int, std::shared_ptr<Task>> _tasks;


      std::string _host;
      int _port;
      enum ConnectionStatus { NotConnected, Connecting, Connected };
      ConnectionStatus _status;

      std::thread _communicationThread;
      boost::asio::io_service _ioService;
      boost::asio::ip::tcp::socket _socket;

      int _nextOperationId;
      int _nextStreamId;

      ChangeQueue _changeQueue;

      std::mutex _communicationMutex;
      std::queue<std::string> _unsubmittedMessages;
    };

  } // namespace net
} // namespace persistence

#endif // PERSISTENCE_NET_NETCLIENTBACKEND_H
