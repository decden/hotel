#ifndef SERVER_NETCLIENTSESSION_H
#define SERVER_NETCLIENTSESSION_H

#include "server/messagesender.h"

#include "persistence/backend.h"

#include <boost/asio.hpp>

#include "extern/nlohmann_json/json.hpp"

#include <memory>
#include <map>
#include <array>
#include <vector>
#include <queue>

namespace server
{

  namespace detail
  {
    // Utility classes for listening to changes in the "real" backend.
    class SessionStreamObserver;
    class SessionTaskObserver;
  }

  /**
   * @brief The NetClientSession class represents a session to a client over the network.
   *
   * The lifetime of the session corresponds to the lifetime of the network socket which establishes the connection to
   * the client.
   *
   * This class is responsible for all communication with the client as well as forwarding all of the information from
   * and to the real backend.
   *
   * The network protocol implemented by this class is relatively simple and is based on simple enveloped JSON messages.
   * Every message has the format `length | message` where `length` is the 32 bit little endian length of the message
   * and `message` is the utf-8 encoded JSON string. `length` refers to the length in bytes of the message (excluding
   * the header). All messages must at least have the 'op' field set, so that the client/server can identify the type of
   * message.
   */
  class NetClientSession : public std::enable_shared_from_this<NetClientSession>, public MessageSender
  {
  public:
    /**
     * @brief Creates a new session for a user
     * @param ioService The boost asio IO service which will be managing I/O operations
     * @param backend The "real" backend which will execute all of the operations and will provide the data streams.
     */
    NetClientSession(boost::asio::io_service &ioService, persistence::Backend& backend);
    ~NetClientSession();

    void start();
    void close();

    /**
     * @brief Returns the socket which has been allocated for this session.
     */
    boost::asio::ip::tcp::socket &socket() { return _socket; }

    /**
     * @brief sendMessage sends the given json message to the client.
     * This method takes care of encoding and enveloping the message.
     */
    virtual void sendMessage(const nlohmann::json& json) override;

  private:
    /**
     * @brief doReadHeader reads the next 4 bytes from the socket.
     *
     * These bytes will contain the length of the next message. If the operation succeeds, the message body will be read
     * right after.
     *
     * @see doReadBody
     */
    void doReadHeader();

    /**
     * @brief doReadBody reads the next message body.
     *
     * The length of the message body has been read by `doReadHeader` before calling this method. After the message body
     * has been successfully read, reading of the next message header will be scheduled.
     *
     * @see doReadHeader
     */
    void doReadBody();

    /**
     * @brief doSend sends all outstanding messages.
     *
     * @note This method shall only be called if sending is not already in progress (i.e. only if the `_outgoingMessage`
     *        queue is empty when a new message is added)
     */
    void doSend();

    // These methods are handlers for particular commands. This part of the code is not yet as polished as it could be
    // and is not yet complete. Only a few of the available commands are implemented yet...
    void runCommand(const nlohmann::json &obj);
    void runCreateStream(const nlohmann::json &obj);
    void runRemoveStream(const nlohmann::json &obj);
    void runScheduleOperations(const nlohmann::json &obj);


    persistence::Backend& _backend;
    boost::asio::ip::tcp::socket _socket;

    // Observers for open streams/operations
    std::vector<std::pair<persistence::UniqueDataStreamHandle, std::unique_ptr<detail::SessionStreamObserver>>> _streams;
    std::vector<std::unique_ptr<detail::SessionTaskObserver>> _taskObservers;

    // Buffers for reading in the next message
    std::array<char, 4> _headerData;
    std::vector<char> _bodyData;

    // Buffers for writing the next message
    std::vector<char> _writeData;

    // Queue of messages which have not yet been sent
    std::queue<std::string> _outgoingMessages;

  };



} // namespace server

#endif // SERVER_NETCLIENTSESSION_H
