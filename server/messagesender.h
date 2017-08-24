#ifndef SERVER_MESSAGESENDER_H
#define SERVER_MESSAGESENDER_H

#include "extern/nlohmann_json/json.hpp"

namespace server
{

  /**
   * @brief Interface for servers with the ability to send messages.
   */
  class MessageSender
  {
  public:
    virtual ~MessageSender();
    virtual void sendMessage(const nlohmann::json& json) = 0;
  };

} // namespace server

#endif // SERVER_MESSAGESENDER_H
