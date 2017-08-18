#ifndef PERSISTENCE_NETCLIENT_ENVELOPE_H
#define PERSISTENCE_NETCLIENT_ENVELOPE_H

#include "persistence/datastream.h"

#include "extern/nlohmann_json/json.hpp"

#include <vector>
#include <string>

namespace persistence
{
  namespace net
  {
    class JsonSerializer
    {
    public:
      static nlohmann::json serializeStreamAddMessage(int streamId, const persistence::StreamableItems& items);
      static nlohmann::json serializeStreamUpdateMessage(int streamId, const persistence::StreamableItems& items);
      static nlohmann::json serializeStreamRemoveMessage(int streamId, const std::vector<int>& ids);
      static nlohmann::json serializeStreamClearMessage(int streamId);
      static nlohmann::json serializeStreamInitializeMessage(int streamId);

      static std::pair<int, persistence::StreamableItems> deserializeStreamAddMessage(const nlohmann::json& json);
      static std::pair<int, persistence::StreamableItems> deserializeStreamUpdateMessage(const nlohmann::json& json);

    private:
      static persistence::StreamableItems deserializeStreamableItems(const std::string &type, const nlohmann::json &array);

      template <class T>
      static std::string serializeListType();
      static std::string serializeListType(const persistence::StreamableItems& items);
    };

  } // namespace net
} // namespace persistence

#endif // PERSISTENCE_NETCLIENT_ENVELOPE_H
