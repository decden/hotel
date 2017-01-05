#include "hotel/reservation.h"

namespace hotel
{

  Reservation::Reservation(const std::string& description)
      : _status(Unknown), _description(description), _reservationOwnerPersonId(), _adults(0), _children(0), _atoms()
  {
  }

  hotel::Reservation::Reservation(const std::string& description, int roomId, boost::gregorian::date_period dateRange)
      : _status(Unknown), _description(description), _reservationOwnerPersonId(), _adults(0), _children(0), _atoms()
  {
    _atoms.push_back(std::make_unique<ReservationAtom>(roomId, dateRange));
  }

  Reservation::Reservation(Reservation&& that)
      : _status(that._status), _description(std::move(that._description)),
        _reservationOwnerPersonId(that._reservationOwnerPersonId), _adults(that._adults), _children(that._children),
        _atoms(std::move(that._atoms))
  {
    that._status = Unknown;
    that._description = "";
    that._reservationOwnerPersonId = boost::optional<int>();
    that._adults = 0;
    that._children = 0;
    that._atoms.clear();
  }

  void Reservation::setStatus(Reservation::ReservationStatus status) { _status = status; }
  void Reservation::setDescription(const std::string& newDescription) { _description = newDescription; }
  void Reservation::setNumberOfAdults(int adults) { _adults = adults; }
  void Reservation::setNumberOfChildren(int children) { _children = children; }
  void Reservation::setReservationOwnerPerson(boost::optional<int> personId) { _reservationOwnerPersonId = personId; }
  ReservationAtom* Reservation::addAtom(int room, boost::gregorian::date_period dateRange)
  {
    if (dateRange.is_null())
      throw std::logic_error("Cannot add atom because its date range is invalid");

    if (_atoms.empty() || lastAtom()->dateRange().end() == dateRange.begin())
    {
      auto atom = new ReservationAtom(room, dateRange);
      _atoms.push_back(std::unique_ptr<ReservationAtom>(atom));
      return atom;
    }
    else
      throw std::logic_error("Cannot add atom because the date range is not contiguous to the previous atom");
  }

  ReservationAtom* Reservation::addContinuation(int room, boost::gregorian::date date)
  {
    if (_atoms.empty())
      throw std::logic_error("Cannot add continuation, because reservation has no atom yet");

    if (lastAtom()->dateRange().end() >= date)
      throw std::logic_error(
          "Cannot create reservation continuation... The given date preceeds the end date of the last atom");

    auto atom = new ReservationAtom(room, boost::gregorian::date_period(lastAtom()->dateRange().end(), date));
    _atoms.push_back(std::unique_ptr<ReservationAtom>(atom));
    return atom;
  }

  Reservation::ReservationStatus Reservation::status() const { return _status; }
  const std::string& Reservation::description() const { return _description; }
  int Reservation::numberOfAdults() const { return _adults; }
  int Reservation::numberOfChildren() const { return _children; }
  boost::optional<int> Reservation::reservationOwnerPersonId() { return _reservationOwnerPersonId; }
  const std::vector<std::unique_ptr<ReservationAtom>>& Reservation::atoms() const { return _atoms; }
  const ReservationAtom *Reservation::firstAtom() const { return _atoms.empty() ? nullptr : _atoms[0].get(); }
  const ReservationAtom *Reservation::lastAtom() const { return _atoms.empty() ? nullptr : _atoms[_atoms.size() - 1].get(); }

  boost::gregorian::date_period Reservation::dateRange() const
  {
    using namespace boost::gregorian;
    if (!isValid())
      return date_period(date(), date());
    return date_period(_atoms.front()->dateRange().begin(), _atoms.back()->dateRange().end());
  }

  const bool Reservation::isValid() const
  {
    if (_atoms.empty())
      return false;

    if (_atoms[0]->dateRange().is_null())
      return false;

    // Check that all of the atoms are contiguous and have valid date ranges
    for (size_t i = 1; i < _atoms.size(); ++i)
    {
      if (_atoms[i]->dateRange().is_null())
        return false;
      if (_atoms[i - 1]->dateRange().end() != _atoms[i]->dateRange().begin())
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

  ReservationAtom::ReservationAtom(const int room, boost::gregorian::date_period dateRange)
      : _roomId(room), _dateRange(dateRange)
  {
  }

  bool operator==(const ReservationAtom &a, const ReservationAtom &b)
  {
    return a.roomId() == b.roomId() && a.dateRange() == b.dateRange();
  }

  bool operator!=(const ReservationAtom &a, const ReservationAtom &b)
  {
    return !(a == b);
  }

} // namespace hotel
