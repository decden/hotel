#ifndef HOTEL_HOTELCOLLECTION_H
#define HOTEL_HOTELCOLLECTION_H

#include "hotel/hotel.h"
#include "hotel/observablecollection.h"

#include <vector>

namespace hotel
{
  typedef ObservableCollection<const hotel::Hotel*> ObservableHotelCollection;
  typedef CollectionObserver<const hotel::Hotel*> HotelCollectionObserver;

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

    void addObserver(HotelCollectionObserver* observer);
    void removeObserver(HotelCollectionObserver* observer);

  private:
    std::vector<std::unique_ptr<hotel::Hotel>> _hotels;

    ObservableHotelCollection _observableCollection;
  };

} // namespace hotel

#endif // HOTEL_HOTELCOLLECTION_H
