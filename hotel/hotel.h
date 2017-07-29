#ifndef HOTEL_HOTEL_H
#define HOTEL_HOTEL_H

#include <memory>
#include <string>
#include <vector>

#include "hotel/persistentobject.h"

namespace hotel
{

  class Hotel;

  /**
   * @brief The RoomCategory class contains information shared by a set of rooms.
   */
  class RoomCategory : public PersistentObject
  {
  public:
    RoomCategory(const std::string& shortCode, const std::string& name);

    bool operator==(const RoomCategory& that) const;
    bool operator!=(const RoomCategory& that) const;

    const std::string& shortCode() const;
    const std::string& name() const;

  private:
    std::string _shortCode;
    std::string _name;
  };

  class HotelRoom : public PersistentObject
  {
  public:
    HotelRoom(const std::string& name);

    bool operator==(const HotelRoom& that) const;
    bool operator!=(const HotelRoom& that) const;

    void setCategory(const RoomCategory* category);

    const RoomCategory* category() const;
    const std::string& name() const;

  private:
    const RoomCategory* _category;
    std::string _name;
  };

  class Hotel : public PersistentObject
  {
  public:
    Hotel(const std::string& name);
    //! The copy constructor performs a deep copy of the object
    Hotel(const Hotel& that);
    Hotel& operator=(const Hotel& that);

    bool operator==(const Hotel& that) const;
    bool operator!=(const Hotel& that) const;

    const std::string& name() const;
    const std::vector<std::unique_ptr<HotelRoom>>& rooms() const;
    const std::vector<std::unique_ptr<RoomCategory>>& categories() const;

    void addRoomCategory(std::unique_ptr<RoomCategory> category);
    void addRoom(std::unique_ptr<HotelRoom> room, const std::string& categoryShortCode);

    RoomCategory* getCategoryById(int id);
    RoomCategory* getCategoryByShortCode(const std::string& shortCode);

  private:
    std::string _name;
    std::vector<std::unique_ptr<RoomCategory>> _categories;
    std::vector<std::unique_ptr<HotelRoom>> _rooms;
  };

} // namespace hotel

#endif // HOTEL_HOTEL_H
