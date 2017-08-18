#include "persistence/net/jsonserializer.h"
#include "persistence/json/jsonserializer.h"

namespace persistence
{
  namespace net
  {
    nlohmann::json JsonSerializer::serializeStreamAddMessage(int streamId, const persistence::StreamableItems &items)
    {
      nlohmann::json obj = {
        {"op", "stream_add"},
        {"type", serializeListType(items)},
        {"id", streamId}
      };

      auto array = nlohmann::json::array();
      boost::apply_visitor([&](auto& items) {
        for (auto& item : items)
          array.push_back(persistence::json::JsonSerializer::serialize(item));
      }, items);
      obj["items"] = array;

      return obj;
    }

    nlohmann::json JsonSerializer::serializeStreamUpdateMessage(int streamId, const persistence::StreamableItems &items)
    {
      nlohmann::json obj = {
        {"op", "stream_update"},
        {"type", serializeListType(items)},
        {"id", streamId}
      };

      auto array = nlohmann::json::array();
      boost::apply_visitor([&](auto& items) {
        for (auto& item : items)
          array.push_back(persistence::json::JsonSerializer::serialize(item));
      }, items);
      obj["items"] = array;

      return obj;
    }

    nlohmann::json JsonSerializer::serializeStreamRemoveMessage(int streamId, const std::vector<int> &ids)
    {
      nlohmann::json obj = {
        {"op", "stream_remove"},
        {"id", streamId},
        {"items", ids}
      };

      return obj;
    }

    nlohmann::json JsonSerializer::serializeStreamClearMessage(int streamId)
    {
      nlohmann::json obj = {
        {"op", "stream_clear"},
        {"id", streamId}
      };

      return obj;
    }

    nlohmann::json JsonSerializer::serializeStreamInitializeMessage(int streamId)
    {
      nlohmann::json obj = {
        {"op", "stream_initialize"},
        {"id", streamId}
      };

      return obj;
    }

    std::pair<int, StreamableItems> JsonSerializer::deserializeStreamAddMessage(const nlohmann::json &json)
    {
      assert(json["op"] == "stream_add");
      int id = json["id"];

      std::string type = json["type"];
      StreamableItems items = deserializeStreamableItems(type, json["items"]);

      return std::make_pair(id, std::move(items));
    }

    std::pair<int, StreamableItems> JsonSerializer::deserializeStreamUpdateMessage(const nlohmann::json &json)
    {
      assert(json["op"] == "stream_update");
      int id = json["id"];

      std::string type = json["type"];
      StreamableItems items = deserializeStreamableItems(type, json["items"]);

      return std::make_pair(id, std::move(items));
    }

    StreamableItems JsonSerializer::deserializeStreamableItems(const std::string &type, const nlohmann::json &array)
    {
      if (type == "hotel")
      {
        std::vector<hotel::Hotel> data;
        data.reserve(array.size());
        for (auto &item : array)
          data.push_back(persistence::json::JsonSerializer::deserializeHotel(item));
        return  StreamableItems(std::move(data));
      }
      else if (type == "reservation")
      {
        std::vector<hotel::Reservation> data;
        data.reserve(array.size());
        for (auto &item : array)
          data.push_back(persistence::json::JsonSerializer::deserializeReservation(item));
        return StreamableItems(std::move(data));
      }
      else
      {
        // Unknown type...
        assert(false);
      }
    }

    template<> std::string JsonSerializer::serializeListType<std::vector<hotel::Hotel>>() { return "hotel"; }
    template<> std::string JsonSerializer::serializeListType<std::vector<hotel::Reservation>>() { return "reservation"; }

    std::string JsonSerializer::serializeListType(const StreamableItems &items)
    {
      return boost::apply_visitor([](const auto &list) {
        return JsonSerializer::serializeListType<typename std::decay<decltype(list)>::type>();
      }, items);
    }

  } // namespace net
} // namespace persistence
