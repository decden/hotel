#ifndef PERSISTENCE_NET_NETCLIENTBACKEND_H
#define PERSISTENCE_NET_NETCLIENTBACKEND_H

#include "persistence/backend.h"
#include "persistence/changequeue.h"
#include "persistence/op/task.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <thread>
#include <atomic>
#include <condition_variable>

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

      virtual void start() override;
      virtual void stopAndJoin() override;

      virtual ChangeQueue& changeQueue() override;
      virtual op::Task<op::OperationResults> queueOperation(op::Operations operations) override;

      virtual std::shared_ptr<DataStream> createStream(DataStreamObserver *observer, StreamableType type,
                                                       const std::string &service, const nlohmann::json &options) override;

    private:
//      typedef std::pair<op::Operations, std::shared_ptr<op::TaskSharedState<op::OperationResults>>> QueuedOperation;
//      typedef std::shared_ptr<DataStream> QueuedStream;
//      typedef boost::variant<QueuedOperation, QueuedStream> QueuedMessage;

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
