#include "hotel/hotel.h"

#include <algorithm>
#include <iostream>

namespace hotel {

RoomCategory::RoomCategory(const std::string &shortCode, const std::string &name)
  : _hotel(nullptr)
  , _shortCode(shortCode)
  , _name(name)
{
}

void RoomCategory::setHotel(const Hotel* hotel) { _hotel = hotel; }
const std::string &RoomCategory::shortCode() const { return _shortCode; }
const std::string &RoomCategory::name() const { return _name; }

HotelRoom::HotelRoom(const std::string &name)
  : _category(nullptr)
  , _name(name)
{
}

void HotelRoom::setCategory(const RoomCategory *category) { _category = category; }
const RoomCategory *HotelRoom::category() const { return _category; }
const std::string &HotelRoom::name() const { return _name; }

Hotel::Hotel(const std::string& name)
  : _name(name)
{
}

const std::string& Hotel::name() const { return _name; }
const std::vector<std::unique_ptr<HotelRoom>> &Hotel::rooms() const { return _rooms; }
const std::vector<std::unique_ptr<RoomCategory>> &Hotel::categories() const { return _categories; }

void Hotel::addRoomCategory(std::unique_ptr<RoomCategory> category)
{
  auto existingCategory = getCategoryByShortCode(category->shortCode());
  if (existingCategory != _categories.end())
    std::cerr << "Category already registered! Category short code: " << category->shortCode() << ", name: " << (*existingCategory)->name() << std::endl;
  else
  {
    category->setHotel(this);
    _categories.push_back(std::move(category));
  }
}

void Hotel::addRoom(std::unique_ptr<HotelRoom> room, const std::string &categoryShortCode)
{
  auto category = getCategoryByShortCode(categoryShortCode);
  if (category == _categories.end())
  {
    std::cerr << "Unknown category " << categoryShortCode << std::endl;
    return;
  }

  room->setCategory(category->get());
  _rooms.push_back(std::move(room));
}

RoomCategory* Hotel::getCategoryById(int id)
{
  auto it = std::find_if(_categories.begin(), _categories.end(),
    [id](const std::unique_ptr<RoomCategory>& category) {
      return category->id() == id;
  });
  if (it != _categories.end())
    return it->get();
  else
    return nullptr;
}

std::vector<std::unique_ptr<RoomCategory>>::iterator Hotel::getCategoryByShortCode(const std::string &shortCode)
{
  return std::find_if(_categories.begin(), _categories.end(),
    [shortCode](const std::unique_ptr<RoomCategory>& category) {
      return category->shortCode() == shortCode;
  });
}


} // namespace hotel
