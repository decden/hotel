#include "persistence/datasource.h"

#include <algorithm>
#include <iostream>

namespace persistence
{

  DataSource::DataSource(const std::string& databaseFile) : _storage(databaseFile)
  {
    _hotels = _storage.loadHotels();
    _planning = _storage.loadPlanning(_hotels->allRoomIDs());
  }

  hotel::HotelCollection& DataSource::hotels() { return *_hotels; }
  const hotel::HotelCollection& DataSource::hotels() const { return *_hotels; }
  hotel::PlanningBoard& DataSource::planning() { return *_planning; }
  const hotel::PlanningBoard& DataSource::planning() const { return *_planning; }

} // namespace persistence
