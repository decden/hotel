#include "hotel/reservation.h"

namespace hotel
{

  Reservation::Reservation(const std::string& description)
      : PersistentObject(), _status(Unknown), _description(description), _reservationOwnerPersonId(), _adults(0),
        _children(0), _atoms()
  {
  }

  hotel::Reservation::Reservation(const std::string& description, int roomId, boost::gregorian::date_period dateRange)
      : _status(Unknown), _description(description), _reservationOwnerPersonId(), _adults(0), _children(0), _atoms()
  {
    _atoms.push_back(ReservationAtom(roomId, dateRange));
  }

  void Reservation::setStatus(Reservation::ReservationStatus status) { _status = status; }
  void Reservation::setDescription(const std::string& newDescription) { _description = newDescription; }
  void Reservation::setNumberOfAdults(int adults) { _adults = adults; }
  void Reservation::setNumberOfChildren(int children) { _children = children; }
  void Reservation::setReservationOwnerPerson(boost::optional<int> personId) { _reservationOwnerPersonId = personId; }
  const ReservationAtom* Reservation::addAtom(int room, boost::gregorian::date_period dateRange)
  {
    if (dateRange.is_null())
      throw std::logic_error("Cannot add atom because its date range is invalid");

    if (_atoms.empty() || lastAtom()->dateRange().end() == dateRange.begin())
    {
      _atoms.push_back(ReservationAtom(room, dateRange));
      return lastAtom();
    }
    else
      throw std::logic_error("Cannot add atom because the date range is not contiguous to the previous atom");
  }

  const ReservationAtom* Reservation::addAtom(const ReservationAtom& atom)
  {
    return addAtom(atom.roomId(), atom.dateRange());
  }

  const ReservationAtom* Reservation::addContinuation(int room, boost::gregorian::date date)
  {
    if (_atoms.empty())
      throw std::logic_error("Cannot add continuation, because reservation has no atom yet");

    if (lastAtom()->dateRange().end() >= date)
      throw std::logic_error(
          "Cannot create reservation continuation... The given date preceeds the end date of the last atom");

    auto atom = ReservationAtom(room, boost::gregorian::date_period(lastAtom()->dateRange().end(), date));
    _atoms.push_back(atom);
    return lastAtom();
  }

  void Reservation::removeAllAtoms()
  {
    _atoms.clear();
  }

  Reservation::ReservationStatus Reservation::status() const { return _status; }
  const std::string& Reservation::description() const { return _description; }
  int Reservation::numberOfAdults() const { return _adults; }
  int Reservation::numberOfChildren() const { return _children; }
  boost::optional<int> Reservation::reservationOwnerPersonId() const { return _reservationOwnerPersonId; }

  const std::vector<ReservationAtom>& Reservation::atoms() const { return _atoms; }
  std::vector<ReservationAtom>& Reservation::atoms() { return _atoms; }
  const ReservationAtom* Reservation::firstAtom() const { return _atoms.empty() ? nullptr : &_atoms[0]; }
  const ReservationAtom* Reservation::lastAtom() const { return _atoms.empty() ? nullptr : &_atoms[_atoms.size() - 1]; }

  boost::gregorian::date_period Reservation::dateRange() const
  {
    using namespace boost::gregorian;
    if (!isValid())
      return date_period(date(), date());
    return date_period(_atoms.front().dateRange().begin(), _atoms.back().dateRange().end());
  }

  const bool Reservation::isValid() const
  {
    if (_atoms.empty())
      return false;

    if (_atoms[0].dateRange().is_null())
      return false;

    // Check that all of the atoms are contiguous and have valid date ranges
    for (size_t i = 1; i < _atoms.size(); ++i)
    {
      if (_atoms[i].dateRange().is_null())
        return false;
      if (_atoms[i - 1].dateRange().end() != _atoms[i].dateRange().begin())
        return false;
    }

    return true;
  }

  const int Reservation::length() const
  {
    if (_atoms.empty())
      return 0;

    return (lastAtom()->dateRange().end() - firstAtom()->dateRange().begin()).days();
  }

  bool operator==(const Reservation& a, const Reservation& b)
  {
    return a.status() == b.status() && a.description() == b.description() &&
           a.reservationOwnerPersonId() == b.reservationOwnerPersonId() && a.numberOfAdults() == b.numberOfAdults() &&
           a.numberOfChildren() == b.numberOfChildren() && a.atoms() == b.atoms();
  }

  bool operator!=(const Reservation& a, const Reservation& b) { return !(a == b); }

  ReservationAtom::ReservationAtom(const int room, boost::gregorian::date_period dateRange)
      : _roomId(room), _dateRange(dateRange)
  {
  }

  bool operator==(const ReservationAtom& a, const ReservationAtom& b)
  {
    return a.roomId() == b.roomId() && a.dateRange() == b.dateRange();
  }

  bool operator!=(const ReservationAtom& a, const ReservationAtom& b) { return !(a == b); }

} // namespace hotel
