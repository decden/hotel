#include "hotel/planning.h"

namespace hotel
{

ReservationAtom::ReservationAtom(Reservation *reservation, const int room, boost::gregorian::date_period dateRange)
  : _reservation(reservation)
  , _roomId(room)
  , _dateRange(dateRange)
{
}

Reservation *PlanningBoard::addReservation(std::unique_ptr<Reservation> reservation)
{
  if (!canAddReservation(*reservation))
  {
    std::cerr << "addReservation(): Cannot add reservation" << std::endl;
    return nullptr;
  }

  for (auto& atom : reservation->atoms())
    insertAtom(atom.get());

  auto reservationPtr = reservation.get();
  _reservations.push_back(std::move(reservation));
  return reservationPtr;
}

void PlanningBoard::removeReservation(const Reservation *reservation)
{
  for(auto& atom : reservation->atoms())
  {
    auto& roomAtoms = _rooms[atom->_roomId];
    auto atomIt = std::find(roomAtoms.begin(), roomAtoms.end(), atom.get());
    if (atomIt != roomAtoms.end())
      roomAtoms.erase(atomIt);
  }

  auto reservationIt = std::find_if(_reservations.begin(), _reservations.end(), [=](auto& x) {
    return x.get() == reservation;
  });
  if (reservationIt != _reservations.end())
    _reservations.erase(reservationIt);
}

void PlanningBoard::addRoomId(int roomId)
{
  // This will insert a new item "room" if it does not yet exist
  _rooms[roomId];
}

bool PlanningBoard::canAddReservation(const Reservation &reservation) const
{
  auto& atoms = reservation.atoms();
  return std::all_of(atoms.begin(), atoms.end(), [this](auto& atom) {
    return this->isFree(atom->_roomId, atom->_dateRange);
  });
}

bool PlanningBoard::isFree(int roomId, boost::gregorian::date_period period) const
{
  if (!hasRoom(roomId))
    return false;

  // TODO: This can be made more efficient! log(n) instead of linear
  auto& roomAtoms = _rooms.find(roomId)->second;
  return std::none_of(roomAtoms.begin(), roomAtoms.end(), [&period](auto x) {
    return x->_dateRange.intersects(period);
  });
}

bool PlanningBoard::hasRoom(int roomId) const
{
  return _rooms.find(roomId) != _rooms.end();
}

int PlanningBoard::getAvailableDaysFrom(int roomId, boost::gregorian::date date) const
{
  if (!hasRoom(roomId))
    return 0;

  // Find the first element which would influence the number of available days: i.e.
  // atom.period.end > date
  auto& roomAtoms = _rooms.find(roomId)->second;
  auto it = std::upper_bound(roomAtoms.begin(), roomAtoms.end(), date, [](auto date, auto& x) {
    return date < x->_dateRange.end();
  });

  if (it == roomAtoms.end())
    return std::numeric_limits<int>::max();
  else
    return std::max<int>(0, ((*it)->_dateRange.begin() - date).days());
}

const std::vector<std::unique_ptr<Reservation>> &PlanningBoard::reservations() const
{
  return _reservations;
}

void PlanningBoard::insertAtom(const ReservationAtom *atom)
{
  // TODO: This can be made more efficient! Linear instead of n*log(n)
  auto& roomAtoms = _rooms[atom->_roomId];
  roomAtoms.push_back(atom);
  std::sort(roomAtoms.begin(), roomAtoms.end(), [](auto x, auto y) {
    return x->_dateRange.begin() < y->_dateRange.begin();
  });
}

} // namespace hotel
