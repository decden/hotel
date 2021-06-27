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
      std::visit([&](auto& items) {
        for (auto& item : items)
          array.push_back(persistence::json::serialize(item));
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
      std::visit([&](auto& items) {
        for (auto& item : items)
          array.push_back(persistence::json::serialize(item));
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

    nlohmann::json JsonSerializer::serializeTaskResultsMessage(int taskId, const std::vector<TaskResult>& items)
    {
      nlohmann::json obj = nlohmann::json::object();
      obj["op"] = "task_results";
      obj["id"] = taskId;

      auto results = nlohmann::json::array();
      for (auto& item : items)
      {
        nlohmann::json result = {
          {"status", (int)item.status},
          {"data", item.result}
        };
        results.push_back(result);
      }
      obj["results"] = results;

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

    std::pair<int, std::vector<persistence::TaskResult>> JsonSerializer::deserializeTaskResultsMessage(const nlohmann::json &json)
    {
      int id = json["id"];

      std::vector<persistence::TaskResult> results;
      for (auto& item : json["results"])
      {
        int status = item["status"];
        results.push_back({(persistence::TaskResultStatus)status, item["data"]});
      }

      return {id, std::move(results)};
    }

    StreamableItems JsonSerializer::deserializeStreamableItems(const std::string &type, const nlohmann::json &array)
    {
      if (type == "hotel")
      {
        std::vector<hotel::Hotel> data;
        data.reserve(array.size());
        for (auto &item : array)
          data.push_back(persistence::json::deserialize<hotel::Hotel>(item));
        return  StreamableItems(std::move(data));
      }
      else if (type == "reservation")
      {
        std::vector<hotel::Reservation> data;
        data.reserve(array.size());
        for (auto &item : array)
          data.push_back(persistence::json::deserialize<hotel::Reservation>(item));
        return StreamableItems(std::move(data));
      }
      else
      {
        // Unknown type...
        assert(false);
        return StreamableItems();
      }
    }

    template<> std::string JsonSerializer::serializeListType<std::vector<hotel::Hotel>>() { return "hotel"; }
    template<> std::string JsonSerializer::serializeListType<std::vector<hotel::Reservation>>() { return "reservation"; }

    std::string JsonSerializer::serializeListType(const StreamableItems &items)
    {
      return std::visit([](const auto &list) {
        return JsonSerializer::serializeListType<typename std::decay<decltype(list)>::type>();
      }, items);
    }

  } // namespace net
} // namespace persistence
