#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "hotel/planning.h"

class HotelPlanning : public testing::Test
{
public:
  boost::gregorian::date makeDate(int day)
  {
    using namespace boost::gregorian;
    return date(2017, 1, 1) + days(day);
  }

  hotel::Reservation makeReservation(int room, int from, int to)
  {
    using namespace boost::gregorian;
    return hotel::Reservation("", room, date_period(makeDate(from), makeDate(to)));
  }
};

class MockPlanningBoardObserver : public hotel::PlanningBoardObserver
{
public:
  MOCK_METHOD1(reservationsAdded, void(const std::vector<const hotel::Reservation*>& reservations));
  MOCK_METHOD1(reservationsRemoved, void(const std::vector<const hotel::Reservation*>& reservations));
  MOCK_METHOD1(initialUpdate, void(const hotel::PlanningBoard&));
  MOCK_METHOD0(allReservationsRemoved, void());
};

TEST_F(HotelPlanning, Planning)
{
  using namespace boost::gregorian;
  auto reservation1 = makeReservation(1, 1, 3);
  auto reservation2 = makeReservation(1, 3, 5);
  auto reservation3 = makeReservation(1, 4, 5);
  auto reservation4 = makeReservation(3, 4, 9);
  auto reservation5 = makeReservation(3, 2, 2);

  hotel::PlanningBoard board;
  ASSERT_EQ(0, board.reservations().size());
  ASSERT_EQ(0, board.getReservationsInPeriod(date_period(date(2016, 1, 1), date(2018, 1, 1))).size());
  ASSERT_TRUE(board.getPlanningExtent().is_null());

  // No rooms
  ASSERT_FALSE(board.hasRoom(1));
  ASSERT_FALSE(board.isFree(1, reservation1.dateRange()));
  ASSERT_FALSE(board.canAddReservation(reservation1));
  ASSERT_EQ(0, board.getAvailableDaysFrom(1, date(2016, 1, 1)));

  // Adding rooms
  board.addRoomId(1);
  board.addRoomId(2);
  ASSERT_TRUE(board.hasRoom(1));
  ASSERT_TRUE(board.hasRoom(2));
  ASSERT_FALSE(board.hasRoom(3));
  ASSERT_TRUE(board.isFree(1, reservation1.dateRange()));
  ASSERT_TRUE(board.canAddReservation(reservation1));
  ASSERT_EQ(std::numeric_limits<int>::max(), board.getAvailableDaysFrom(1, makeDate(-100)));

  // Adding reservations
  board.addReservation(std::make_unique<hotel::Reservation>(reservation1));
  board.addReservation(std::make_unique<hotel::Reservation>(reservation2));
  ASSERT_ANY_THROW(board.addReservation(nullptr));
  ASSERT_ANY_THROW(board.addReservation(std::make_unique<hotel::Reservation>(reservation2)));
  ASSERT_ANY_THROW(board.addReservation(std::make_unique<hotel::Reservation>(reservation3)));
  ASSERT_ANY_THROW(board.addReservation(std::make_unique<hotel::Reservation>(reservation4)));
  ASSERT_ANY_THROW(board.addReservation(std::make_unique<hotel::Reservation>(reservation5)));
  ASSERT_EQ(2, board.reservations().size());
  ASSERT_EQ(date_period(makeDate(1), makeDate(5)), board.getPlanningExtent());

  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 2, 4)));
  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 6, 8)));
  ASSERT_EQ(4, board.reservations().size());
  ASSERT_EQ(date_period(makeDate(1), makeDate(8)), board.getPlanningExtent());
  // After the above insertions we have the following situation:
  // Room   1 2 3 4 5 6 7 8 9
  //    1   [###|###]
  //    2     [###]   [###]
  ASSERT_EQ(11, board.getAvailableDaysFrom(1, makeDate(-10)));
  ASSERT_EQ(1, board.getAvailableDaysFrom(1, makeDate(0)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(1, makeDate(1)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(1, makeDate(3)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(1, makeDate(4)));
  ASSERT_EQ(std::numeric_limits<int>::max(), board.getAvailableDaysFrom(1, makeDate(5)));
  ASSERT_EQ(12, board.getAvailableDaysFrom(2, makeDate(-10)));
  ASSERT_EQ(2, board.getAvailableDaysFrom(2, makeDate(0)));
  ASSERT_EQ(1, board.getAvailableDaysFrom(2, makeDate(1)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(2, makeDate(3)));
  ASSERT_EQ(2, board.getAvailableDaysFrom(2, makeDate(4)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(2, makeDate(6)));
  ASSERT_EQ(0, board.getAvailableDaysFrom(2, makeDate(7)));
  ASSERT_EQ(std::numeric_limits<int>::max(), board.getAvailableDaysFrom(2, makeDate(8)));
  ASSERT_TRUE(board.canAddReservation(makeReservation(1, 0, 1)));
  ASSERT_FALSE(board.canAddReservation(makeReservation(1, 0, 2)));
  ASSERT_TRUE(board.canAddReservation(makeReservation(1, 5, 7)));
  ASSERT_TRUE(board.canAddReservation(makeReservation(2, 4, 5)));
  ASSERT_TRUE(board.canAddReservation(makeReservation(2, 4, 6)));
  ASSERT_FALSE(board.canAddReservation(makeReservation(2, 4, 7)));
  ASSERT_FALSE(board.canAddReservation(makeReservation(2, 10, 10)));

  ASSERT_EQ(0, board.getReservationsInPeriod(date_period(makeDate(0), makeDate(1))).size());
  ASSERT_EQ(1, board.getReservationsInPeriod(date_period(makeDate(1), makeDate(2))).size());
  ASSERT_EQ(2, board.getReservationsInPeriod(date_period(makeDate(1), makeDate(3))).size());
  ASSERT_EQ(3, board.getReservationsInPeriod(date_period(makeDate(1), makeDate(4))).size());
  ASSERT_EQ(4, board.getReservationsInPeriod(date_period(makeDate(1), makeDate(7))).size());
  ASSERT_EQ(1, board.getReservationsInPeriod(date_period(makeDate(5), makeDate(7))).size());

  // Test removal of reservations
  auto allReservations = board.getReservationsInPeriod(board.getPlanningExtent());
  ASSERT_EQ(4, allReservations.size());
  board.removeReservation(allReservations[0]);
  ASSERT_EQ(3, board.getReservationsInPeriod(board.getPlanningExtent()).size());
  board.removeReservation(allReservations[1]);
  ASSERT_EQ(2, board.getReservationsInPeriod(board.getPlanningExtent()).size());
  board.removeReservation(allReservations[2]);
  ASSERT_EQ(1, board.getReservationsInPeriod(board.getPlanningExtent()).size());
  board.removeReservation(allReservations[3]);
  ASSERT_EQ(0, board.getReservationsInPeriod(board.getPlanningExtent()).size());
  ASSERT_ANY_THROW(board.removeReservation(nullptr));
}

TEST_F(HotelPlanning, PlanningBoardObserver)
{
  hotel::PlanningBoard board;
  board.addRoomId(2);
  MockPlanningBoardObserver observer;

  EXPECT_CALL(observer, initialUpdate(testing::_)).Times(1);
  observer.setObservedPlanningBoard(&board);
  testing::Mock::VerifyAndClear(&observer);

  // Add reservations
  EXPECT_CALL(observer, reservationsAdded(testing::_)).Times(2);
  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 2, 4)));
  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 6, 8)));
  testing::Mock::VerifyAndClear(&observer);

  // Remove reservations
  EXPECT_CALL(observer, reservationsRemoved(testing::_)).Times(1);
  auto allReservations = board.getReservationsInPeriod(board.getPlanningExtent());
  board.removeReservation(allReservations[0]);
  testing::Mock::VerifyAndClear(&observer);

  // Reset observed board to nullptr
  EXPECT_CALL(observer, allReservationsRemoved()).Times(1);
  observer.setObservedPlanningBoard(nullptr);
  testing::Mock::VerifyAndClear(&observer);

  // Add reservations while no observer attached
  EXPECT_CALL(observer, initialUpdate(testing::_)).Times(1);
  EXPECT_CALL(observer, reservationsAdded(testing::_)).Times(0);
  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 10, 11)));
  board.addReservation(std::make_unique<hotel::Reservation>(makeReservation(2, 11, 12)));
  observer.setObservedPlanningBoard(&board);
  testing::Mock::VerifyAndClear(&observer);
}
