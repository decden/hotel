#include "hotel/hotel.h"

#include <algorithm>
#include <iostream>

namespace hotel {


  RoomCategory::RoomCategory(const std::string& id, const std::string& name)
    : _id(id)
    , _name(name)
  {
  }

  const std::string& RoomCategory::id() const { return _id; }
  const std::string& RoomCategory::name() const { return _name; }

  HotelRoom::HotelRoom(const std::string& id, const std::string& categoryId, const std::string& name)
    : _id(id)
    , _categoryId(categoryId)
    , _name(name)
  {
  }

  const std::string& HotelRoom::id() const { return _id; }
  const std::string& HotelRoom::categoryId() const { return _categoryId; }
  const std::string& HotelRoom::name() const { return _name; }

  Hotel::Hotel(const std::string& id, const std::string& name)
    : _id(id)
    , _name(name)
  {
  }

  const std::string& Hotel::id() const { return _id; }
  const std::string& Hotel::name() const { return _name; }
  const std::vector<HotelRoom> &Hotel::rooms() const { return _rooms; }

  void Hotel::addRoomCategory(RoomCategory category)
  {
    auto existingCategory = getCategoryById(category.id());
    if (existingCategory != _categories.end())
      std::cerr << "Category already registered! Category id: " << category.id() << std::endl;
    else
      _categories.push_back(category);
  }

  void Hotel::addRoom(HotelRoom room)
  {
    auto category = getCategoryById(room.categoryId());
    if (category == _categories.end())
    {
      std::cerr << "Unknown category " << room.categoryId() << std::endl;
      return;
    }

    _rooms.push_back(room);
  }

  std::vector<RoomCategory>::iterator Hotel::getCategoryById(const std::string& categoryId)
  {
    return std::find_if(_categories.begin(), _categories.end(),
      [&](const RoomCategory& category) {
        return category.id() == categoryId;
      });
  }


} // namespace hotel
