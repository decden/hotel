#include "hotel/hotel.h"

#include <algorithm>
#include <iostream>
#include <cassert>

namespace hotel
{

  RoomCategory::RoomCategory(const std::string& shortCode, const std::string& name)
      : _shortCode(shortCode), _name(name)
  {
  }

  const std::string& RoomCategory::shortCode() const { return _shortCode; }
  const std::string& RoomCategory::name() const { return _name; }

  HotelRoom::HotelRoom(const std::string& name) : _category(nullptr), _name(name) {}

  void HotelRoom::setCategory(const RoomCategory* category) { _category = category; }
  const RoomCategory* HotelRoom::category() const { return _category; }
  const std::string& HotelRoom::name() const { return _name; }

  Hotel::Hotel(const std::string& name) : _name(name) {}

  Hotel::Hotel(const Hotel& that) : _name(that._name)
  {
    // Clone categories
    for (auto& category : that._categories)
      _categories.push_back(std::make_unique<hotel::RoomCategory>(*category));

    // Clone rooms
    for (auto& room : that._rooms)
      addRoom(std::make_unique<hotel::HotelRoom>(*room), room->category()->shortCode());
  }

  const std::string& Hotel::name() const { return _name; }
  const std::vector<std::unique_ptr<HotelRoom>>& Hotel::rooms() const { return _rooms; }
  const std::vector<std::unique_ptr<RoomCategory>>& Hotel::categories() const { return _categories; }

  void Hotel::addRoomCategory(std::unique_ptr<RoomCategory> category)
  {
    if (category == nullptr)
      throw std::logic_error("Trying to add a nullptr category to the hotel");

    auto existingCategory = getCategoryByShortCode(category->shortCode());
    if (existingCategory != nullptr)
      throw std::logic_error("Category already registered! Category short code: " + category->shortCode() +
                             ", name: " + existingCategory->name());

    _categories.push_back(std::move(category));
  }

  void Hotel::addRoom(std::unique_ptr<HotelRoom> room, const std::string& categoryShortCode)
  {
    if (room == nullptr)
      throw std::logic_error("Trying to add a nullptr room to the hotel");

    auto existingCategory = getCategoryByShortCode(categoryShortCode);
    if (existingCategory == nullptr)
      throw std::logic_error("Trying to add a room with unknown category " + categoryShortCode);

    room->setCategory(existingCategory);
    _rooms.push_back(std::move(room));
  }

  RoomCategory* Hotel::getCategoryById(int id)
  {
    auto it = std::find_if(_categories.begin(), _categories.end(),
                           [id](const std::unique_ptr<RoomCategory>& category) { return category->id() == id; });
    return it != _categories.end() ? it->get() : nullptr;
  }

  RoomCategory* Hotel::getCategoryByShortCode(const std::string& shortCode)
  {
    auto it =
        std::find_if(_categories.begin(), _categories.end(), [shortCode](const std::unique_ptr<RoomCategory>& category) {
          return category->shortCode() == shortCode;
        });
    return it != _categories.end() ? it->get() : nullptr;
  }

} // namespace hotel
