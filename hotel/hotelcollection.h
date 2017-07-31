#ifndef HOTEL_HOTELCOLLECTION_H
#define HOTEL_HOTELCOLLECTION_H

#include "hotel/hotel.h"

#include <vector>

namespace hotel
{
  /**
   * @brief The HotelCollection class holds a list of hotels.
   *
   * The class also provides utility functions to iterate over the whole collection.
   */
  class HotelCollection
  {
  public:
    HotelCollection();
    explicit HotelCollection(std::vector<std::unique_ptr<hotel::Hotel>> hotel);
    HotelCollection(const HotelCollection& that);
    HotelCollection& operator=(const HotelCollection& that);
    HotelCollection& operator=(HotelCollection&& that);

    void addHotel(std::unique_ptr<hotel::Hotel> hotel);
    void clear();

    const std::vector<std::unique_ptr<Hotel>> &hotels() const;

    std::vector<int> allRoomIDs() const;
    std::vector<int> allCategoryIDs() const;

    hotel::HotelRoom* findRoomById(int id);

    std::vector<hotel::HotelRoom*> allRooms();
    std::vector<hotel::HotelRoom*> allRoomsByCategory(int categoryId);

  private:
    std::vector<std::unique_ptr<hotel::Hotel>> _hotels;
  };

} // namespace hotel

#endif // HOTEL_HOTELCOLLECTION_H
