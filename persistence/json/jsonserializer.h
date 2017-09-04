#ifndef PERSISTENCE_JSON_JSONSERIALIZER_H
#define PERSISTENCE_JSON_JSONSERIALIZER_H

#include "persistence/op/operations.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include "json.hpp"

#include <boost/optional.hpp>

namespace persistence
{
  namespace json
  {
    template <class T> nlohmann::json serialize(const T& item);
    template <class T> T deserialize(const nlohmann::json& json);

    // Basic hotel datatypes
    template <> nlohmann::json serialize(const hotel::PersistentObject& item);
    template <> nlohmann::json serialize(const hotel::Hotel& item);
    template <> nlohmann::json serialize(const hotel::RoomCategory& item);
    template <> nlohmann::json serialize(const hotel::HotelRoom& item);
    template <> nlohmann::json serialize(const hotel::Reservation& item);
    template <> nlohmann::json serialize(const hotel::Reservation::ReservationStatus& item);
    template <> nlohmann::json serialize(const hotel::ReservationAtom& item);
    template <> nlohmann::json serialize(const hotel::Person& item);
    template <> nlohmann::json serialize(const persistence::op::StreamableType& type);

    void deserializePersistentObject(hotel::PersistentObject& item, const nlohmann::json& json);
    template <> hotel::Hotel deserialize(const nlohmann::json& json);
    template <> hotel::RoomCategory deserialize(const nlohmann::json& json);
    template <> hotel::HotelRoom deserialize(const nlohmann::json& json);
    template <> hotel::Reservation deserialize(const nlohmann::json& json);
    template <> hotel::ReservationAtom deserialize(const nlohmann::json& json);
    template <> hotel::Reservation::ReservationStatus deserialize(const nlohmann::json& json);
    template <> hotel::Person deserialize(const nlohmann::json& json);

    template <> persistence::op::StreamableType deserialize(const nlohmann::json& json);
    template <> boost::optional<persistence::op::Operation> deserialize(const nlohmann::json& json);

    template <> nlohmann::json serialize(const op::Operation& operation);
    template <> nlohmann::json serialize(const op::EraseAllData& operation);
    template <> nlohmann::json serialize(const op::StoreNew& operation);
    template <> nlohmann::json serialize(const op::Update& operation);
    template <> nlohmann::json serialize(const op::Delete& operation);

  } // namespace json
} // namespace persistence

#endif // PERSISTENCE_JSON_JSONSERIALIZER_H
