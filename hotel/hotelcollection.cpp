#include "hotelcollection.h"

namespace hotel
{
  HotelCollection::HotelCollection() : _hotels() {}

  HotelCollection::HotelCollection(std::vector<std::unique_ptr<Hotel>> hotel) : _hotels(std::move(hotel)) {}

  const std::vector<std::unique_ptr<Hotel>>& HotelCollection::hotels() const { return _hotels; }

  std::vector<int> HotelCollection::allRoomIDs() const
  {
    std::vector<int> roomIds;
    for (auto& hotel : _hotels)
      for (auto& room : hotel->rooms())
        roomIds.push_back(room->id());
    return roomIds;
  }

  std::vector<int> HotelCollection::allCategoryIDs() const
  {
    std::vector<int> categoryIds;
    for (auto& hotel : _hotels)
      for (auto& category : hotel->categories())
        categoryIds.push_back(category->id());
    return categoryIds;
  }

  std::vector<HotelRoom*> HotelCollection::allRooms()
  {
    std::vector<hotel::HotelRoom*> rooms;
    for (auto& hotel : _hotels)
      for (auto& room : hotel->rooms())
        rooms.push_back(room.get());
    return rooms;
  }

  std::vector<HotelRoom*> HotelCollection::allRoomsByCategory(int categoryId)
  {
    std::vector<hotel::HotelRoom*> rooms;
    for (auto& hotel : _hotels)
      for (auto& room : hotel->rooms())
        if (room->category()->id() == categoryId)
          rooms.push_back(room.get());
    return rooms;
  }

} // namespace hotel
