#include "gtest/gtest.h"

#include "hotel/hotel.h"
#include "hotel/person.h"

TEST(Hotel, Person)
{
  hotel::Person person("First", "Last");
  ASSERT_EQ("First", person.firstName());
  ASSERT_EQ("Last", person.lastName());

  hotel::Person copy = person;
  ASSERT_EQ("First", copy.firstName());
  ASSERT_EQ("Last", copy.lastName());
}

TEST(Hotel, RoomCategory)
{
  hotel::RoomCategory category("CODE", "My Category");
  ASSERT_EQ("CODE", category.shortCode());
  ASSERT_EQ("My Category", category.name());

  hotel::RoomCategory copy = category;
  ASSERT_EQ("CODE", copy.shortCode());
  ASSERT_EQ("My Category", copy.name());
}

TEST(Hotel, HotelRoom)
{
  hotel::HotelRoom room("Room 1");
  ASSERT_EQ("Room 1", room.name());
  ASSERT_EQ(nullptr, room.category());

  hotel::RoomCategory category("CODE", "My Category");
  room.setCategory(&category);
  ASSERT_EQ(&category, room.category());

  hotel::HotelRoom copy = room;
  ASSERT_EQ("Room 1", copy.name());
  ASSERT_EQ(&category, copy.category());
}

TEST(Hotel, Hotel)
{
  // Construction
  hotel::Hotel hotel("Hotel X");
  ASSERT_EQ("Hotel X", hotel.name());
  ASSERT_EQ(0, hotel.rooms().size());
  ASSERT_EQ(0, hotel.categories().size());
  ASSERT_EQ(nullptr, hotel.getCategoryById(1));
  ASSERT_EQ(nullptr, hotel.getCategoryByShortCode("CODE"));

  // Add a category
  hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>("CODE", "My Category"));
  ASSERT_EQ(1, hotel.categories().size());
  ASSERT_EQ(nullptr, hotel.getCategoryById(1));
  ASSERT_NE(nullptr, hotel.getCategoryByShortCode("CODE"));
  hotel.categories()[0]->setId(1);
  ASSERT_NE(nullptr, hotel.getCategoryById(1));

  // Add a category whose short code already exist
  ASSERT_ANY_THROW(hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>("CODE", "My Other Category")));
  ASSERT_NE(nullptr, hotel.getCategoryByShortCode("CODE"));
  ASSERT_EQ("My Category", hotel.getCategoryByShortCode("CODE")->name());

  // Try adding a nullptr category
  ASSERT_ANY_THROW(hotel.addRoomCategory(nullptr));

  // Add a room
  hotel.addRoom(std::make_unique<hotel::HotelRoom>("Room 1"), "CODE");
  ASSERT_EQ(1, hotel.rooms().size());

  // Add a room with a non existing category
  ASSERT_ANY_THROW(hotel.addRoom(std::make_unique<hotel::HotelRoom>("Room 1"), "IDONOTEXIST"));
  ASSERT_EQ(1, hotel.rooms().size());

  // Add a nullptr room
  ASSERT_ANY_THROW(hotel.addRoom(nullptr, "CODE"));

  // Copy construction
  hotel::Hotel copy = hotel;
  ASSERT_EQ("Hotel X", hotel.name());
  ASSERT_EQ(1, copy.rooms().size());
  ASSERT_EQ(1, copy.categories().size());
  // Make sure copy is not shallow
  ASSERT_NE(hotel.getCategoryById(1), copy.getCategoryById(1));
  ASSERT_NE(hotel.getCategoryByShortCode("CODE"), copy.getCategoryByShortCode("CODE"));
  ASSERT_NE(hotel.rooms()[0], copy.rooms()[0]);
  // Make sure copied data is correct
  ASSERT_EQ("CODE", copy.getCategoryById(1)->shortCode());
  ASSERT_EQ(1, copy.getCategoryByShortCode("CODE")->id());
  ASSERT_EQ("Room 1", copy.rooms()[0]->name());
}
