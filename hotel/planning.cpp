#include "hotel/planning.h"

namespace hotel
{
  PlanningBoard& PlanningBoard::operator=(const PlanningBoard& that)
  {
    assert(this != &that);
    if (this == &that) return *this;

    clear();

    // Copy reservations
    for (auto& reservation : that._reservations)
      addReservation(std::make_unique<Reservation>(*reservation));

    return *this;
  }

  PlanningBoard& PlanningBoard::operator=(PlanningBoard&& that)
  {
    assert(this != &that);

    clear();
    _rooms = std::move(that._rooms);
    _reservations = std::move(that._reservations);
    that.clear();

    return *this;
  }

  Reservation* PlanningBoard::addReservation(std::unique_ptr<Reservation> reservation)
  {
    if (reservation == nullptr)
      throw std::invalid_argument("cannot add nullptr reservation to planning board");

    if (!canAddReservation(*reservation))
      throw std::logic_error("cannot add reservation " + reservation->description());

    // Insert reservation and atoms
    for (auto& atom : reservation->atoms())
      insertAtom(&atom);
    auto reservationPtr = reservation.get();
    _reservations.push_back(std::move(reservation));

    return reservationPtr;
  }

  void PlanningBoard::removeReservation(const Reservation* reservation)
  {
    if (reservation == nullptr)
      throw std::invalid_argument("cannot remove nullptr reservation from planning board");

    // Remove the atoms
    for (auto& atom : reservation->atoms())
    {
      auto& roomAtoms = _rooms[atom.roomId()];
      auto atomIt = std::find(roomAtoms.begin(), roomAtoms.end(), &atom);
      if (atomIt != roomAtoms.end())
        roomAtoms.erase(atomIt);
    }

    // Find and remove the reservation, then notify the observers
    auto reservationIt =
        std::find_if(_reservations.begin(), _reservations.end(), [=](auto& x) { return x.get() == reservation; });
    if (reservationIt != _reservations.end())
      _reservations.erase(reservationIt);
  }

  void PlanningBoard::removeReservation(int reservationId)
  {
    // Find the reservation
    auto reservationIt = std::find_if(_reservations.begin(), _reservations.end(), [=](auto& x) { return x->id() == reservationId; });
    if (reservationIt != _reservations.end())
    {
      // First, remove the atoms
      for (auto& atom : (*reservationIt)->atoms())
      {
        auto& roomAtoms = _rooms[atom.roomId()];
        auto atomIt = std::find(roomAtoms.begin(), roomAtoms.end(), &atom);
        if (atomIt != roomAtoms.end())
          roomAtoms.erase(atomIt);
      }

      // Then, remove the reservation
      _reservations.erase(reservationIt);
    }
  }

  void PlanningBoard::clear()
  {
    _reservations.clear();
    _rooms.clear();
  }

  bool PlanningBoard::canAddReservation(const Reservation& reservation) const
  {
    if (!reservation.isValid())
      return false;

    auto& atoms = reservation.atoms();
    return std::all_of(atoms.begin(), atoms.end(),
                       [this](auto& atom) { return this->isFree(atom.roomId(), atom.dateRange()); });
  }

  bool PlanningBoard::isFree(int roomId, boost::gregorian::date_period period) const
  {
    auto roomIt = _rooms.find(roomId);
    if (roomIt == _rooms.end())
      return true;

    // TODO: This can be made more efficient! log(n) instead of linear
    auto& roomAtoms = roomIt->second;
    return std::none_of(roomAtoms.begin(), roomAtoms.end(),
                        [&period](auto x) { return x->dateRange().intersects(period); });
  }

  int PlanningBoard::getAvailableDaysFrom(int roomId, boost::gregorian::date date) const
  {
    auto roomIt = _rooms.find(roomId);
    if (roomIt == _rooms.end())
      return std::numeric_limits<int>::max();

    // Find the first element which would influence the number of available days: i.e.
    // atom.period.end > date
    auto& roomAtoms = roomIt->second;
    auto it = std::upper_bound(roomAtoms.begin(), roomAtoms.end(), date,
                               [](auto date, auto& x) { return date < x->dateRange().end(); });

    if (it == roomAtoms.end())
      return std::numeric_limits<int>::max();
    else
      return std::max<int>(0, ((*it)->dateRange().begin() - date).days());
  }

  std::vector<Reservation*> PlanningBoard::reservations()
  {
    std::vector<Reservation*> result;
    result.reserve(_reservations.size());
    for (auto& reservation : _reservations)
      result.push_back(reservation.get());
    return result;
  }

  std::vector<const Reservation*> PlanningBoard::reservations() const
  {
    std::vector<const Reservation*> result;
    result.reserve(_reservations.size());
    for (auto& reservation : _reservations)
      result.push_back(reservation.get());
    return result;
  }

  std::vector<Reservation*> PlanningBoard::getReservationsInPeriod(boost::gregorian::date_period period)
  {
    std::vector<Reservation*> result;
    for (auto& reservation : _reservations)
      if (reservation->dateRange().intersects(period))
        result.push_back(reservation.get());
    return result;
  }

  std::vector<const Reservation*> PlanningBoard::getReservationsInPeriod(boost::gregorian::date_period period) const
  {
    std::vector<const Reservation*> result;
    for (auto& reservation : _reservations)
      if (reservation->dateRange().intersects(period))
        result.push_back(reservation.get());
    return result;
  }

  const Reservation *PlanningBoard::getReservationById(int id) const
  {
    auto it = std::find_if(_reservations.begin(), _reservations.end(), [id](auto& x) { return x->id() == id; });
    if (it != _reservations.end())
      return it->get();
    else
      return nullptr;
  }

  boost::gregorian::date_period PlanningBoard::getPlanningExtent() const
  {
    using namespace boost::gregorian;
    if (_reservations.empty())
    {
      auto today = day_clock::local_day();
      return date_period(today, today);
    }
    else
    {
      date from(pos_infin);
      date to(neg_infin);
      for (auto& roomRow : _rooms)
      {
        if (!roomRow.second.empty())
        {
          auto& atoms = roomRow.second;
          from = std::min(from, atoms.front()->dateRange().begin());
          to = std::max(to, atoms.back()->dateRange().end());
        }
      }
      assert(!from.is_special() && !to.is_special() && from < to);
      return date_period(from, to);
    }
  }

  void PlanningBoard::insertAtom(const ReservationAtom* atom)
  {
    // TODO: This can be made more efficient! Linear instead of n*log(n)
    auto& roomAtoms = _rooms[atom->roomId()];
    roomAtoms.push_back(atom);
    std::sort(roomAtoms.begin(), roomAtoms.end(),
              [](auto x, auto y) { return x->dateRange().begin() < y->dateRange().begin(); });
  }

} // namespace hotel
