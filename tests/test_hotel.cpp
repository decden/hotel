#include "gtest/gtest.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/person.h"
#include "hotel/reservation.h"

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
  ASSERT_EQ(0u, hotel.rooms().size());
  ASSERT_EQ(0u, hotel.categories().size());
  ASSERT_EQ(nullptr, hotel.getCategoryById(1));
  ASSERT_EQ(nullptr, hotel.getCategoryByShortCode("CODE"));

  // Add a category
  hotel.addRoomCategory(std::make_unique<hotel::RoomCategory>("CODE", "My Category"));
  ASSERT_EQ(1u, hotel.categories().size());
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
  ASSERT_EQ(1u, hotel.rooms().size());

  // Add a room with a non existing category
  ASSERT_ANY_THROW(hotel.addRoom(std::make_unique<hotel::HotelRoom>("Room 1"), "IDONOTEXIST"));
  ASSERT_EQ(1u, hotel.rooms().size());

  // Add a nullptr room
  ASSERT_ANY_THROW(hotel.addRoom(nullptr, "CODE"));

  // Copy construction
  hotel::Hotel copy = hotel;
  ASSERT_EQ("Hotel X", hotel.name());
  ASSERT_EQ(1u, copy.rooms().size());
  ASSERT_EQ(1u, copy.categories().size());
  // Make sure copy is not shallow
  ASSERT_NE(hotel.getCategoryById(1), copy.getCategoryById(1));
  ASSERT_NE(hotel.getCategoryByShortCode("CODE"), copy.getCategoryByShortCode("CODE"));
  ASSERT_NE(hotel.rooms()[0], copy.rooms()[0]);
  // Make sure copied data is correct
  ASSERT_EQ("CODE", copy.getCategoryById(1)->shortCode());
  ASSERT_EQ(1, copy.getCategoryByShortCode("CODE")->id());
  ASSERT_EQ("Room 1", copy.rooms()[0]->name());
}

TEST(Hotel, HotelCollection)
{
  hotel::HotelCollection emptyCollection;
  ASSERT_EQ(0u, emptyCollection.allRoomIDs().size());
  ASSERT_EQ(0u, emptyCollection.allCategoryIDs().size());
  ASSERT_EQ(nullptr, emptyCollection.findRoomById(1));
  ASSERT_EQ(0u, emptyCollection.allRooms().size());
  ASSERT_EQ(0u, emptyCollection.allRoomsByCategory(1).size());

  // Build a collection with one hotel
  std::vector<std::unique_ptr<hotel::Hotel>> hotels;
  auto hotel = std::make_unique<hotel::Hotel>("Hotel");
  hotel->addRoomCategory(std::make_unique<hotel::RoomCategory>("CAT", "Category"));
  hotel->getCategoryByShortCode("CAT")->setId(1);
  hotel->addRoom(std::make_unique<hotel::HotelRoom>("Room"), "CAT");
  hotel->rooms()[0]->setId(2);
  hotels.push_back(std::move(hotel));

  hotel::HotelCollection collection(std::move(hotels));
  ASSERT_EQ(1u, collection.allRoomIDs().size());
  ASSERT_EQ(1u, collection.allCategoryIDs().size());
  ASSERT_EQ(2, collection.allRoomIDs()[0]);
  ASSERT_EQ(1, collection.allCategoryIDs()[0]);
  ASSERT_EQ(nullptr, collection.findRoomById(1));
  ASSERT_NE(nullptr, collection.findRoomById(2));
  ASSERT_EQ(1u, collection.allRooms().size());
  ASSERT_EQ(1u, collection.allRoomsByCategory(1).size());
  ASSERT_EQ("Room", collection.allRooms()[0]->name());
  ASSERT_EQ("Room", collection.allRoomsByCategory(1)[0]->name());

  hotel::HotelCollection copy = collection;
  ASSERT_EQ(1u, copy.allRoomIDs().size());
  ASSERT_EQ(1u, copy.allCategoryIDs().size());
  ASSERT_EQ(2, copy.allRoomIDs()[0]);
  ASSERT_EQ(1, copy.allCategoryIDs()[0]);
  ASSERT_EQ(nullptr, copy.findRoomById(1));
  ASSERT_EQ(1u, copy.allRooms().size());
  ASSERT_EQ(1u, copy.allRoomsByCategory(1).size());
  ASSERT_EQ("Room", copy.allRooms()[0]->name());
  ASSERT_EQ("Room", copy.allRoomsByCategory(1)[0]->name());
}

TEST(Hotel, ReservationAtom)
{
  using namespace boost::gregorian;
  hotel::ReservationAtom atom1(10, date_period(date(2017, 1, 1), date(2017, 1, 10)));
  hotel::ReservationAtom atom2(12, date_period(date(2017, 1, 1), date(2017, 1, 10)));
  hotel::ReservationAtom atom3(10, date_period(date(2017, 1, 10), date(2017, 1, 20)));
  ASSERT_EQ(10, atom1.roomId());
  ASSERT_EQ(12, atom2.roomId());
  ASSERT_EQ(10, atom3.roomId());
  ASSERT_EQ(date_period(date(2017, 1, 1), date(2017, 1, 10)), atom1.dateRange());
  ASSERT_EQ(date_period(date(2017, 1, 1), date(2017, 1, 10)), atom2.dateRange());
  ASSERT_EQ(date_period(date(2017, 1, 10), date(2017, 1, 20)), atom3.dateRange());
  ASSERT_NE(atom1, atom2);
  ASSERT_NE(atom1, atom3);
  ASSERT_NE(atom2, atom3);

  auto atom1Copy = atom1;
  ASSERT_EQ(10, atom1Copy.roomId());
  ASSERT_EQ(date_period(date(2017, 1, 1), date(2017, 1, 10)), atom1Copy.dateRange());
  ASSERT_EQ(atom1Copy, atom1);
}

TEST(Hotel, Reservation)
{
  using namespace boost::gregorian;

  hotel::Reservation emptyReservation("Empty");
  ASSERT_EQ(hotel::Reservation::Unknown, emptyReservation.status());
  ASSERT_EQ("Empty", emptyReservation.description());
  ASSERT_EQ(0, emptyReservation.numberOfAdults());
  ASSERT_EQ(0, emptyReservation.numberOfChildren());
  ASSERT_EQ(boost::optional<int>(), emptyReservation.reservationOwnerPersonId());
  ASSERT_EQ(0u, emptyReservation.atoms().size());
  ASSERT_EQ(nullptr, emptyReservation.firstAtom());
  ASSERT_EQ(nullptr, emptyReservation.lastAtom());
  ASSERT_TRUE(emptyReservation.dateRange().is_null());
  ASSERT_FALSE(emptyReservation.isValid());
  ASSERT_EQ(0, emptyReservation.length());

  hotel::Reservation reservation("Valid", 1, date_period(date(2017, 1, 1), date(2017, 1, 10)));
  ASSERT_EQ(hotel::Reservation::Unknown, reservation.status());
  ASSERT_EQ("Valid", reservation.description());
  ASSERT_EQ(0, reservation.numberOfAdults());
  ASSERT_EQ(0, reservation.numberOfChildren());
  ASSERT_EQ(boost::optional<int>(), reservation.reservationOwnerPersonId());
  ASSERT_EQ(1u, reservation.atoms().size());
  ASSERT_NE(nullptr, reservation.firstAtom());
  ASSERT_NE(nullptr, reservation.lastAtom());
  ASSERT_EQ(reservation.firstAtom(), reservation.lastAtom());
  ASSERT_FALSE(reservation.dateRange().is_null());
  ASSERT_TRUE(reservation.isValid());
  ASSERT_EQ(9, reservation.length());

  // Setters
  reservation.setStatus(hotel::Reservation::CheckedIn);
  reservation.setDescription("Valid Reservation");
  reservation.setNumberOfAdults(2);
  reservation.setNumberOfChildren(3);
  reservation.setReservationOwnerPerson(10);
  ASSERT_EQ(hotel::Reservation::CheckedIn, reservation.status());
  ASSERT_EQ("Valid Reservation", reservation.description());
  ASSERT_EQ(2, reservation.numberOfAdults());
  ASSERT_EQ(3, reservation.numberOfChildren());
  ASSERT_EQ(10, *reservation.reservationOwnerPersonId());


  // Add continuation
  reservation.addContinuation(1, date(2017, 1, 20));
  ASSERT_EQ(2u, reservation.atoms().size());
  ASSERT_NE(reservation.firstAtom(), reservation.lastAtom());
  ASSERT_EQ(19, reservation.length());

  // Add atom
  reservation.addAtom(10, date_period(date(2017, 1, 20), date(2017, 1, 30)));
  ASSERT_EQ(3u, reservation.atoms().size());
  ASSERT_NE(reservation.firstAtom(), reservation.lastAtom());
  ASSERT_EQ(29, reservation.length());

  // Adding invalid continuations
  // Dates are in the past
  ASSERT_ANY_THROW(reservation.addContinuation(1, date(2016, 1, 1)));
  ASSERT_ANY_THROW(reservation.addContinuation(1, date(2017, 1, 30)));
  ASSERT_ANY_THROW(reservation.addAtom(1, date_period(date(2017, 1, 1), date(2017, 1, 10))));
  // Dates are not contiguous
  ASSERT_ANY_THROW(reservation.addAtom(1, date_period(date(2017, 2, 5), date(2017, 2, 10))));
  // Date range is invalid (end date is smaller then start date)
  ASSERT_ANY_THROW(reservation.addAtom(1, date_period(date(2017, 1, 30), date(2017, 1, 10))));
  // Adding a continuation to an empty reservation is always forbidden
  ASSERT_ANY_THROW(emptyReservation.addContinuation(1, date(2016, 1, 1)));

  // Copy
  auto copy = reservation;
  ASSERT_EQ(copy, reservation);
  ASSERT_EQ(hotel::Reservation::CheckedIn, copy.status());
  ASSERT_EQ("Valid Reservation", copy.description());
  ASSERT_EQ(2, copy.numberOfAdults());
  ASSERT_EQ(3, copy.numberOfChildren());
  ASSERT_EQ(10, *copy.reservationOwnerPersonId());
  ASSERT_EQ(3u, copy.atoms().size());
  ASSERT_NE(nullptr, copy.firstAtom());
  ASSERT_NE(nullptr, copy.lastAtom());
  ASSERT_NE(copy.firstAtom(), copy.lastAtom());
  ASSERT_FALSE(copy.dateRange().is_null());
  ASSERT_TRUE(copy.isValid());
  ASSERT_EQ(29, copy.length());

  // Move
  hotel::Reservation moved(std::move(copy));
  ASSERT_FALSE(copy.isValid());
  ASSERT_EQ(moved, reservation);
  ASSERT_EQ(hotel::Reservation::CheckedIn, moved.status());
  ASSERT_EQ("Valid Reservation", moved.description());
  ASSERT_EQ(2, moved.numberOfAdults());
  ASSERT_EQ(3, moved.numberOfChildren());
  ASSERT_EQ(10, *moved.reservationOwnerPersonId());
  ASSERT_EQ(3u, moved.atoms().size());
  ASSERT_NE(nullptr, moved.firstAtom());
  ASSERT_NE(nullptr, moved.lastAtom());
  ASSERT_NE(moved.firstAtom(), moved.lastAtom());
  ASSERT_FALSE(moved.dateRange().is_null());
  ASSERT_TRUE(moved.isValid());
  ASSERT_EQ(29, moved.length());

  // Test equality
  ASSERT_EQ(reservation, reservation);
  ASSERT_NE(reservation, emptyReservation);
}
