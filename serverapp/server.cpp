#include <boost/asio.hpp>

#include "persistence/sqlite/sqlitebackend.h"
#include "persistence/json/jsonserializer.h"
#include "persistence/net/jsonserializer.h"

#include "extern/nlohmann_json/json.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <queue>

class StreamMessageSender
{
public:
  virtual ~StreamMessageSender() {}
  virtual void sendMessage(const nlohmann::json& json) = 0;
};

class SessionStreamObserver final : public persistence::DataStreamObserver
{
public:
  SessionStreamObserver(StreamMessageSender &sender, int clientStreamId) : _sender(sender), _clientStreamId(clientStreamId) {}
  virtual ~SessionStreamObserver() {}

  virtual void addItems(const persistence::StreamableItems& items) override
  {
    auto message = persistence::net::JsonSerializer::serializeStreamAddMessage(_clientStreamId, items);
    _sender.sendMessage(message);
  }

  virtual void updateItems(const persistence::StreamableItems& items) override
  {
    auto message = persistence::net::JsonSerializer::serializeStreamUpdateMessage(_clientStreamId, items);
    _sender.sendMessage(message);
  }

  virtual void removeItems(const std::vector<int>& ids) override
  {
    auto message = persistence::net::JsonSerializer::serializeStreamRemoveMessage(_clientStreamId, ids);
    _sender.sendMessage(message);
  }

  virtual void clear() override
  {
    auto message = persistence::net::JsonSerializer::serializeStreamClearMessage(_clientStreamId);
    _sender.sendMessage(message);
  }

  virtual void initialized() override
  {
    auto message = persistence::net::JsonSerializer::serializeStreamInitializeMessage(_clientStreamId);
    _sender.sendMessage(message);
  }

private:
  StreamMessageSender &_sender;
  int _clientStreamId;
};



class ClientSession : public std::enable_shared_from_this<ClientSession>, public StreamMessageSender
{
public:
  ClientSession(boost::asio::io_service &ioService, persistence::sqlite::SqliteBackend& backend) : _backend(backend), _socket(ioService)
  {
  }

  void start()
  {
    doRead();
  }

  boost::asio::ip::tcp::socket &socket() { return _socket; }

  virtual void sendMessage(const nlohmann::json& json) override
  {
    bool sendInProgress = !_outgoingMessages.empty();
    _outgoingMessages.push(json.dump());
    if (!sendInProgress)
      doSend();
  }
private:
  void doRead()
  {
    _socket.async_read_some(boost::asio::buffer(_headerData.data(), _headerData.size()), [session=this->shared_from_this()](const boost::system::error_code &ec, size_t length)
    {
      if (!ec)
      {
        session->doReadBody();
      }
    });
  }

  void doReadBody()
  {
    size_t size = static_cast<size_t>(static_cast<unsigned char>(_headerData[0])) <<  0 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[1])) <<  8 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[2])) << 16 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[3])) << 24;
    _bodyData.resize(size, 0);
    _socket.async_read_some(boost::asio::buffer(_bodyData.data(), _bodyData.size()), [session=this->shared_from_this()](const boost::system::error_code &ec, size_t length)
    {
      if (!ec)
      {
        std::string message(session->_bodyData.data(), session->_bodyData.size());
        session->runCommand(nlohmann::json::parse(message));
        session->doRead();
      }
    });
  }

  void doSend()
  {
    auto message = _outgoingMessages.front();
    std::cout << " [S] " << message.size() << " bytes" << std::endl;
    std::vector<char> data(message.size() + 4, 0);
    auto size = message.size();
    data[0] = (size >> 0) & 0xff;
    data[1] = (size >> 8) & 0xff;
    data[2] = (size >> 16) & 0xff;
    data[3] = (size >> 24) & 0xff;
    memcpy(data.data() + 4, message.data(), size);

    _socket.async_write_some(boost::asio::buffer(data.data(), data.size()), [session=this->shared_from_this()](const boost::system::error_code &ec, size_t length){
      if (!ec)
      {
        session->_outgoingMessages.pop();
        if (!session->_outgoingMessages.empty())
          session->doSend();
      }
    });
  }

  void runCommand(const nlohmann::json &obj)
  {
    std::string operation = obj["op"];

    if (operation == "create_stream")
      runCreateStream(obj);
    else if (operation == "schedule_operations")
      runScheduleOperations(obj);
    else
      std::cout << " [!] Unknown operation: " << operation << std::endl;
  }

  void runCreateStream(const nlohmann::json &obj)
  {
    int clientId = obj["id"];
    auto type = static_cast<persistence::StreamableType>((int)obj["type"]);
    auto observer = std::make_unique<SessionStreamObserver>(*this, clientId);
    auto stream = _backend.createStream(observer.get(), type, obj["service"], obj["options"]);
    int serverId = stream->streamId();
    std::cout << " [R] Create stream s[" << serverId << "] => c[" << clientId << "]" << std::endl;
    _streams.emplace_back(persistence::UniqueDataStreamHandle(stream), std::move(observer));
  }

  void runScheduleOperations(const nlohmann::json &obj)
  {
    std::cout << " [R] Schedule " << obj["operations"].size() << " operation(s)" << std::endl;

    persistence::op::Operations operations;
    for (auto& operation : obj["operations"])
    {
      auto opType = operation["op"];
      std::cout << "  " << opType << std::endl;
      if (opType == "update_reservation")
      {
        auto reservation = persistence::json::JsonSerializer::deserializeReservation(operation["o"]);
        operations.push_back(persistence::op::UpdateReservation{std::make_unique<hotel::Reservation>(std::move(reservation))});
      }
      else
        std::cout << " [!] Unknown operation " << opType << ": " << operation << std::endl;
    }

    // TODO: Observe the task and notify the client when it completes
    _backend.queueOperation(std::move(operations));
  }

  std::vector<std::pair<persistence::UniqueDataStreamHandle, std::unique_ptr<persistence::DataStreamObserver>>> _streams;

  persistence::sqlite::SqliteBackend& _backend;
  std::array<char, 4> _headerData;
  std::vector<char> _bodyData;
  std::queue<std::string> _outgoingMessages;
  boost::asio::ip::tcp::socket _socket;
};

class Server
{
public:
  Server(persistence::sqlite::SqliteBackend& backend) :
    _backend(backend),
    _ioService(),
    _endpoint(boost::asio::ip::tcp::v4(), 46835),
    _acceptor(_ioService, _endpoint),
    _socket(_ioService)
  {
    doAccept();
  }

  boost::asio::io_service &ioService() { return _ioService; }

  void run() { _ioService.run(); }
  void doAccept()
  {
    auto session = std::make_shared<ClientSession>(_ioService, _backend);
    _sessions.push_back(session);
    _acceptor.async_accept(session->socket(), [this, session](const boost::system::error_code &ec)
    {
      if (!ec)
      {
        session->start();
        std::cout << "A new client just connected!!!" << std::endl;
        this->doAccept();
      }
    });
  }

private:
  persistence::sqlite::SqliteBackend &_backend;
  boost::asio::io_service _ioService;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  boost::asio::ip::tcp::socket _socket;

  std::vector<std::weak_ptr<ClientSession>> _sessions;
};

int main(int argc, char** argv)
{
  std::cout << "================================================================================" << std::endl;
  std::cout << " hotel_serverapp (listening on port 46835)                                      " << std::endl;
  std::cout << "================================================================================" << std::endl;


  persistence::sqlite::SqliteBackend dataBackend("data.db");
  Server server(dataBackend);

  dataBackend.start();
  auto handleChanges = [&]()
  {
    server.ioService().post([&](){
      dataBackend.changeQueue().applyStreamChanges();
      dataBackend.changeQueue().notifyCompletedTasks();
    });
  };
  dataBackend.changeQueue().connectToStreamChangesAvailableSignal(handleChanges);
  dataBackend.changeQueue().connectToTaskCompletedSignal(handleChanges);
  std::cout << std::endl << "starting server..." << std::endl;

  server.run();
  dataBackend.stopAndJoin();
}
