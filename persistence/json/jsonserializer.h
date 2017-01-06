#ifndef PERSISTENCE_JSON_JSONSERIALIZER_H
#define PERSISTENCE_JSON_JSONSERIALIZER_H

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
    };

  } // namespace json
} // namespace persistence

#endif // PERSISTENCE_JSON_JSONSERIALIZER_H
