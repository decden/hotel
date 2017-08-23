#ifndef SERVERAPP_MESSAGESENDER_H
#define SERVERAPP_MESSAGESENDER_H

#include "extern/nlohmann_json/json.hpp"

namespace serverapp
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

} // namespace serverapp

#endif // SERVERAPP_MESSAGESENDER_H
