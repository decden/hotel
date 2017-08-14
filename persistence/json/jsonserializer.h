#ifndef PERSISTENCE_JSON_JSONSERIALIZER_H
#define PERSISTENCE_JSON_JSONSERIALIZER_H

#include "persistence/op/operations.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "json.hpp"

namespace persistence
{
  namespace json
  {

    class JsonSerializer
    {
    public:
      JsonSerializer();

      nlohmann::json serializeHotelCollection(const hotel::HotelCollection& hotelCollection);
      nlohmann::json serializePlanning(const hotel::PlanningBoard& planning);

      static nlohmann::json serialize(const hotel::Hotel& item);
      static nlohmann::json serialize(const hotel::RoomCategory& item);
      static nlohmann::json serialize(const hotel::HotelRoom& item);
      static nlohmann::json serialize(const hotel::Reservation& item);
      static nlohmann::json serialize(const hotel::Person& item);

      static nlohmann::json serialize(const op::Operation& operation);
      static nlohmann::json serialize(const op::EraseAllData& operation);
      static nlohmann::json serialize(const op::StoreNewHotel& operation);
      static nlohmann::json serialize(const op::StoreNewReservation& operation);
      static nlohmann::json serialize(const op::StoreNewPerson& operation);
      static nlohmann::json serialize(const op::UpdateHotel& operation);
      static nlohmann::json serialize(const op::UpdateReservation& operation);
      static nlohmann::json serialize(const op::DeleteReservation& operation);

    private:
      static void setCommonPersistentObjectFields(const hotel::PersistentObject &obj, nlohmann::json &json);
    };

  } // namespace json
} // namespace persistence

#endif // PERSISTENCE_JSON_JSONSERIALIZER_H
