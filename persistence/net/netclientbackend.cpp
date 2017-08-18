#include "persistence/net/netclientbackend.h"

#include "persistence/json/jsonserializer.h"
#include "persistence/net/jsonserializer.h"

#include "persistence/changequeue.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

namespace persistence
{
  namespace net
  {

    NetClientBackend::NetClientBackend(const std::string &host, int port)
      : _host(host), _port(port), _status(NotConnected), _ioService(), _socket(_ioService)
    {
    }

    void NetClientBackend::start()
    {
      // Try to connect to the server
      {
        std::lock_guard<std::mutex> lock(_communicationMutex);
        assert(_status == NotConnected);
        _status = Connecting;
        boost::asio::ip::tcp::resolver resolver(_ioService);
        auto endpointIterator = resolver.resolve({_host, std::to_string(_port)});
        boost::asio::async_connect(_socket, endpointIterator, [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) { this->socketConnected(ec); });
      }

      // Start processing the communication on a separate thread
      _communicationThread = std::thread([this]() {
        _ioService.run();
        std::cout << "IO service has terminated!" << std::endl;
      });
    }

    void NetClientBackend::stopAndJoin()
    {
      // TODO: Quit IO service
      _ioService.post([this](){ _socket.close(); });
      _communicationThread.join();
    }

    ChangeQueue &NetClientBackend::changeQueue()
    {
      return _changeQueue;
    }

    op::Task<op::OperationResults> NetClientBackend::queueOperation(op::Operations operations)
    {
      // Create a task
      auto sharedState = std::make_shared<op::TaskSharedState<op::OperationResults>>(_nextOperationId++);
      _changeQueue.addTask(sharedState);

      //auto operationAndState = QueuedOperation{std::move(operations), sharedState};
      nlohmann::json obj;
      obj["op"] = "schedule_operations";
      std::vector<nlohmann::json> operationsArray;
      for (auto &operation : operations)
        operationsArray.push_back(json::JsonSerializer::serialize(operation));
      obj["operations"]= operationsArray;
      submit(obj.dump());

      return sharedState;
    }

    std::shared_ptr<DataStream> NetClientBackend::createStream(DataStreamObserver *observer, StreamableType type, const std::string &service, const nlohmann::json &options)
    {
      auto stream = std::make_shared<DataStream>(type, service, options);
      stream->connect(_nextStreamId, observer);
      _changeQueue.addStream(stream);

      // Schedule a task...
      nlohmann::json obj;
      obj["op"] = "create_stream";
      obj["id"] = _nextStreamId;
      obj["type"] = (int)type;
      obj["service"] = service;
      obj["options"] = options;
      submit(obj.dump());

      _nextStreamId++;

      return stream;
    }

    void NetClientBackend::socketConnected(boost::system::error_code ec)
    {
      std::lock_guard<std::mutex> lock(_communicationMutex);
      if(!ec)
      {
        _status = Connected;
        std::cout << "Succesfully connected to " << _host << ":" << _port << std::endl;
        if (!_unsubmittedMessages.empty())
          doSumitMessages();
        doReadNextMessageHeader();
      }
      else
      {
        _status = NotConnected;
        std::cerr << "Failed to connect to " << _host << ":" << _port << " " << ec.message() << std::endl;
      }
    }

    void NetClientBackend::doSumitMessages()
    {
      assert(!_unsubmittedMessages.empty());
      //auto& message = _unsubmittedMessages.front();

      // TODO: Serialize message
      auto &data = _unsubmittedMessages.front();

      // Make a message...
      int messageSize = data.size();
      std::vector<char> bufferData(messageSize + 4, 0);
      bufferData[0] = (messageSize >>  0) & 0xff;
      bufferData[1] = (messageSize >>  8) & 0xff;
      bufferData[2] = (messageSize >> 16) & 0xff;
      bufferData[3] = (messageSize >> 24) & 0xff;
      memcpy(bufferData.data() + 4, data.data(), messageSize);

      auto buffer = boost::asio::buffer(bufferData.data(), bufferData.size());

      boost::asio::async_write(_socket, std::move(buffer), [this](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (!ec)
        {
          _unsubmittedMessages.pop();
          if (!_unsubmittedMessages.empty())
            doSumitMessages();
        }
        else
        {
          // TODO: Handle error...
          std::cerr << "Failed to submit message (server disconnected...)" << std::endl;
        }
      });
    }

    void NetClientBackend::doReadNextMessageHeader()
    {
      boost::asio::async_read(_socket, boost::asio::buffer(_headerBuffer.data(), _headerBuffer.size()),
                              [this](boost::system::error_code ec, std::size_t length){
        if (!ec)
        {
          size_t size = static_cast<size_t>(static_cast<unsigned char>(_headerBuffer[0])) <<  0 |
                        static_cast<size_t>(static_cast<unsigned char>(_headerBuffer[1])) <<  8 |
                        static_cast<size_t>(static_cast<unsigned char>(_headerBuffer[2])) << 16 |
                        static_cast<size_t>(static_cast<unsigned char>(_headerBuffer[3])) << 24;
          this->doReadNextMessageBody(size);
        } else
        {
          std::cerr << "Error during receive header " << ec.message() << std::endl;
        }
      });
    }

    void NetClientBackend::doReadNextMessageBody(size_t size)
    {
      _bodyBuffer.resize(size);
      boost::asio::async_read(_socket, boost::asio::buffer(_bodyBuffer.data(), _bodyBuffer.size()),
                              [this](boost::system::error_code ec, std::size_t length){
        if (!ec)
        {
          std::string msg(_bodyBuffer.data(), _bodyBuffer.size());
          this->processMessage(nlohmann::json::parse(msg));
          this->doReadNextMessageHeader();
        } else
        {
          std::cerr << "Error during receive body " << ec.message() << std::endl;
        }
      });

    }

    void NetClientBackend::processMessage(const nlohmann::json &obj)
    {
      std::string operation = obj["op"];
      if (operation == "stream_initialize")
      {
        _changeQueue.addStreamChange(obj["id"], DataStreamInitialized{});
      }
      else if (operation == "stream_add")
      {
        int id;
        persistence::StreamableItems items;
        std::tie(id, items) = persistence::net::JsonSerializer::deserializeStreamAddMessage(obj);
        _changeQueue.addStreamChange(id, DataStreamItemsAdded{std::move(items)});
      }
      else if (operation == "stream_update")
      {
        int id;
        persistence::StreamableItems items;
        std::tie(id, items) = persistence::net::JsonSerializer::deserializeStreamUpdateMessage(obj);
        _changeQueue.addStreamChange(id, DataStreamItemsUpdated{std::move(items)});
      }
      else
      {
        std::cout << "Unknown operation " << operation << std::endl;
      }
    }

    void NetClientBackend::submit(std::string message)
    {
      _ioService.post([this, message=std::move(message)]() mutable {
        bool submitInProgress = !_unsubmittedMessages.empty();
        _unsubmittedMessages.emplace(std::move(message));
        if (!submitInProgress && _status == Connected)
        {
          doSumitMessages();
        }
      });
    }

  } // namespace net
} // namespace persistence
