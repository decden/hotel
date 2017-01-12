#include "hotelcollection.h"

#include <cassert>

namespace hotel
{
  HotelCollection::HotelCollection() : _hotels() {}

  HotelCollection::HotelCollection(std::vector<std::unique_ptr<Hotel>> hotel) : _hotels(std::move(hotel)) {}

  HotelCollection::HotelCollection(const HotelCollection &that)
  {
    *this = that;
  }

  HotelCollection& HotelCollection::operator=(const HotelCollection& that)
  {
    assert(this != &that);
    if (this == &that) return *this;

    _hotels.clear();

    // Deep copy all hotels
    for (auto& hotel : that._hotels)
      _hotels.push_back(std::make_unique<Hotel>(*hotel));

    std::vector<const Hotel*> hotels;
    for (auto& hotel : _hotels)
      hotels.push_back(hotel.get());
    _observableCollection.foreachObserver([=](auto& observer){
      observer.itemsAdded(hotels);
    });

    return *this;
  }

  HotelCollection& HotelCollection::operator=(HotelCollection&& that)
  {
    assert(!that._observableCollection.hasObservers());
    _hotels = std::move(that._hotels);

    std::vector<const Hotel*> hotels;
    for (auto& hotel : _hotels)
      hotels.push_back(hotel.get());
    _observableCollection.foreachObserver([=](auto& observer) {
      observer.itemsAdded(hotels);
    });

    return *this;
  }

  void HotelCollection::addHotel(std::unique_ptr<Hotel> hotel)
  {
    auto hotelPtr = hotel.get();
    _hotels.push_back(std::move(hotel));

    _observableCollection.foreachObserver([=](auto& observer) {
      observer.itemsAdded({ hotelPtr });
    });
  }

  void HotelCollection::clear()
  {
    _hotels.clear();
    _observableCollection.foreachObserver([](auto& observer) {
      observer.allItemsRemoved();
    });
  }

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

  HotelRoom *HotelCollection::findRoomById(int id)
  {
    for (auto& hotel : _hotels)
      for (auto& room : hotel->rooms())
        if (room->id() == id)
          return room.get();
    return nullptr;
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

  void HotelCollection::addObserver(HotelCollectionObserver *observer)
  {
    _observableCollection.addObserver(observer);
    std::vector<const Hotel*> hotels;
    for (auto& hotel : _hotels)
      hotels.push_back(hotel.get());
    observer->itemsAdded(hotels);
  }

  void HotelCollection::removeObserver(HotelCollectionObserver *observer)
  {
    _observableCollection.removeObserver(observer);
  }

} // namespace hotel
