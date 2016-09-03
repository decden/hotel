#ifndef HOTEL_HOTEL_H
#define HOTEL_HOTEL_H

#include <string>
#include <vector>
#include <memory>

#include "hotel/persistentobject.h"

namespace hotel {

  /**
   * @brief The RoomCategory class contains information shared by a set of rooms.
   */
  class RoomCategory : public PersistentObject
  {
  public:
    RoomCategory(const std::string& shortCode, const std::string& name);

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

    const std::string& name() const;
    const std::vector<std::unique_ptr<HotelRoom>>& rooms() const;
    const std::vector<std::unique_ptr<RoomCategory>>& categories() const;

    void addRoomCategory(std::unique_ptr<RoomCategory> category);
    void addRoom(std::unique_ptr<HotelRoom> room, const std::string& categoryShortCode);

  private:
    std::vector<std::unique_ptr<RoomCategory>>::iterator getCategoryById(int id);
    std::vector<std::unique_ptr<RoomCategory>>::iterator getCategoryByShortCode(const std::string& shortCode);

    std::string _name;
    std::vector<std::unique_ptr<RoomCategory>> _categories;
    std::vector<std::unique_ptr<HotelRoom>> _rooms;
  };

} // namespace hotel

#endif // HOTEL_HOTEL_H
