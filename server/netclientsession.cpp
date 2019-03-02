#include "server/netclientsession.h"

#include "server/messagesender.h"

#include "persistence/datastreamobserver.h"
#include "persistence/json/jsonserializer.h"
#include "persistence/net/jsonserializer.h"

namespace server::detail
{
  class IoServiceExecutor
  {
  public:
    IoServiceExecutor(boost::asio::io_service& ioService) : _ioService(&ioService) {}

    template <class Fn> void spawn(Fn&& fn) { _ioService->post(std::forward<Fn>(fn)); }

  private:
    boost::asio::io_service* _ioService;
  };

  class SessionStreamObserver final : public persistence::DataStreamObserver
  {
  public:
    SessionStreamObserver(MessageSender& sender, int clientStreamId) : _sender(sender), _clientStreamId(clientStreamId)
    {
    }
    virtual ~SessionStreamObserver() {}
    int clientStreamId() { return _clientStreamId; }

    virtual void addItems(const persistence::StreamableItems& items) override
    {
      _sender.sendMessage(persistence::net::JsonSerializer::serializeStreamAddMessage(_clientStreamId, items));
    }

    virtual void updateItems(const persistence::StreamableItems& items) override
    {
      _sender.sendMessage(persistence::net::JsonSerializer::serializeStreamUpdateMessage(_clientStreamId, items));
    }

    virtual void removeItems(const std::vector<int>& ids) override
    {
      _sender.sendMessage(persistence::net::JsonSerializer::serializeStreamRemoveMessage(_clientStreamId, ids));
    }

    virtual void clear() override
    {
      _sender.sendMessage(persistence::net::JsonSerializer::serializeStreamClearMessage(_clientStreamId));
    }

    virtual void initialized() override
    {
      _sender.sendMessage(persistence::net::JsonSerializer::serializeStreamInitializeMessage(_clientStreamId));
    }

  private:
    MessageSender& _sender;
    int _clientStreamId;
  };

} // namespace server::detail

namespace server
{
  NetClientSession::NetClientSession(boost::asio::io_service& ioService, persistence::Backend& backend)
      : _backend(backend), _socket(ioService)
  {
  }

  NetClientSession::~NetClientSession()
  {
    if (_socket.is_open())
      std::cout << " [-] Client disconnected " << _socket.remote_endpoint().address().to_string() << std::endl;
  }

  void NetClientSession::start() { doReadHeader(); }

  void NetClientSession::close()
  {
    if (_socket.is_open())
      _socket.close();
  }

  void NetClientSession::sendMessage(const nlohmann::json& json)
  {
    bool sendInProgress = !_outgoingMessages.empty();
    _outgoingMessages.push(json.dump());
    if (!sendInProgress)
      doSend();
  }

  void NetClientSession::doReadHeader()
  {
    boost::asio::async_read(_socket, boost::asio::buffer(_headerData.data(), _headerData.size()),
                            boost::asio::transfer_exactly(4),
                            [session = this->shared_from_this()](const boost::system::error_code& ec, size_t length) {
                              if (!ec)
                              {
                                session->doReadBody();
                              }
                            });
  }

  void NetClientSession::doReadBody()
  {
    size_t size = static_cast<size_t>(static_cast<unsigned char>(_headerData[0])) << 0 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[1])) << 8 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[2])) << 16 |
                  static_cast<size_t>(static_cast<unsigned char>(_headerData[3])) << 24;

    _bodyData.resize(size, 0);
    boost::asio::async_read(_socket, boost::asio::buffer(_bodyData.data(), _bodyData.size()),
                            boost::asio::transfer_exactly(_bodyData.size()),
                            [session = this->shared_from_this()](const boost::system::error_code& ec, size_t length) {
                              if (!ec)
                              {
                                std::string message(session->_bodyData.data(), session->_bodyData.size());
                                session->runCommand(nlohmann::json::parse(message));
                                session->doReadHeader();
                              }
                            });
  }

  void NetClientSession::doSend()
  {
    auto message = _outgoingMessages.front();
    std::cout << " [W] " << nlohmann::json::parse(message)["op"] << " " << message.size() << " bytes" << std::endl;
    _writeData.resize(message.size() + 4);
    auto size = message.size();
    _writeData[0] = (size >> 0) & 0xff;
    _writeData[1] = (size >> 8) & 0xff;
    _writeData[2] = (size >> 16) & 0xff;
    _writeData[3] = (size >> 24) & 0xff;
    memcpy(_writeData.data() + 4, message.data(), size);

    boost::asio::async_write(_socket, boost::asio::buffer(_writeData.data(), _writeData.size()),
                             [session = this->shared_from_this()](const boost::system::error_code& ec, size_t length) {
                               if (!ec)
                               {
                                 session->_outgoingMessages.pop();
                                 if (!session->_outgoingMessages.empty())
                                   session->doSend();
                               }
                             });
  }

  void NetClientSession::runCommand(const nlohmann::json& obj)
  {
    std::string operation = obj["op"];

    if (operation == "create_stream")
      runCreateStream(obj);
    else if (operation == "remove_stream")
      runRemoveStream(obj);
    else if (operation == "schedule_operations")
      runScheduleOperations(obj);
    else
      std::cout << " [!] Unknown operation: " << operation << std::endl;
  }

  void NetClientSession::runCreateStream(const nlohmann::json& obj)
  {
    int clientId = obj["id"];
    auto type = static_cast<persistence::StreamableType>((int)obj["type"]);
    auto observer = std::make_unique<detail::SessionStreamObserver>(*this, clientId);
    auto streamHandle = _backend.createStream(observer.get(), type, obj["service"], obj["options"]);
    int serverId = streamHandle.stream()->streamId();
    std::cout << " [R] Create stream s[" << serverId << "] => c[" << clientId << "]" << std::endl;
    _streams.emplace_back(std::move(streamHandle), std::move(observer));
  }

  void NetClientSession::runRemoveStream(const nlohmann::json& obj)
  {
    int clientId = obj["id"];
    auto it = std::find_if(_streams.begin(), _streams.end(),
                           [clientId](const auto& pair) { return pair.second->clientStreamId() == clientId; });

    if (it != _streams.end())
    {
      std::cout << " [R] Removed stream s[" << it->first.stream()->streamId() << "] => c["
                << it->second->clientStreamId() << "]" << std::endl;
      _streams.erase(it);
    }
  }

  void NetClientSession::runScheduleOperations(const nlohmann::json& obj)
  {
    std::cout << " [R] Schedule " << obj["operations"].size() << " operation(s)" << std::endl;

    persistence::op::Operations operations;
    for (auto& operationObj : obj["operations"])
    {
      auto operation = persistence::json::deserialize<std::optional<persistence::op::Operation>>(operationObj);
      if (operation)
        operations.push_back(std::move(*operation));
      else
        std::cout << " [!] Unknown operation " << operationObj << std::endl;
    }

    // TODO: We might want to have a primitive for this lifetime handling stuff, or provide some guarantees about
    // cancellation
    auto future =
        _backend.queueOperations(std::move(operations))
            .then(detail::IoServiceExecutor(_socket.get_io_service()),
                  [weakThis = weak_from_this(), id = obj["id"]](std::vector<persistence::TaskResult> results) {
                    auto self = weakThis.lock();
                    if (self != nullptr)
                    {
                      auto message = persistence::net::JsonSerializer::serializeTaskResultsMessage(id, results);
                      self->sendMessage(message);
                    }
                    // TODO: Future<void> specialization
                    return 0;
                  });
    // TODO: Keep future alive
  }

} // namespace server
