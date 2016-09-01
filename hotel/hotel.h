#ifndef HOTEL_HOTEL_H
#define HOTEL_HOTEL_H

#include <string>
#include <vector>

namespace hotel {

  class RoomCategory
  {
  public:
    RoomCategory(const std::string& id, const std::string& name);
    ~RoomCategory() = default;

    const std::string& id() const;
    const std::string& name() const;

  private:
    std::string _id;
    std::string _name;
  };

  class HotelRoom
  {
  public:
    HotelRoom(const std::string& id, const std::string& categoryId, const std::string& name);
    ~HotelRoom() = default;

    const std::string& id() const;
    const std::string& categoryId() const;
    const std::string& name() const;

  private:
    std::string _id;
    std::string _categoryId;
    std::string _name;
  };

  class Hotel
  {
  public:
    Hotel(const std::string& id, const std::string& name);
    Hotel(const Hotel& that) = delete;
    Hotel& operator=(const Hotel& that) = delete;
    ~Hotel() = default;

    const std::string& id() const;
    const std::string& name() const;
    const std::vector<HotelRoom>& rooms() const;

    void addRoomCategory(RoomCategory category);
    void addRoom(HotelRoom room);



  private:
    std::vector<RoomCategory>::iterator getCategoryById(const std::string& categoryId);

    std::string _id;
    std::string _name;

    std::vector<RoomCategory> _categories;
    std::vector<HotelRoom> _rooms;
  };

} // namespace hotel

#endif // HOTEL_HOTEL_H
