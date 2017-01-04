#include "hotel/reservation.h"

namespace hotel
{

  Reservation::Reservation(const std::string& description) : _status(Unknown), _description(description), _adults(0), _children(0) {}

  hotel::Reservation::Reservation(const std::string& description, int roomId, boost::gregorian::date_period dateRange)
      : _status(Unknown), _description(description), _adults(0), _children(0)
  {
    _atoms.push_back(std::make_unique<ReservationAtom>(this, roomId, dateRange));
  }

  void Reservation::setStatus(Reservation::ReservationStatus status) { _status = status; }
  void Reservation::setDescription(const std::string& newDescription) { _description = newDescription; }
  void Reservation::setNumberOfAdults(int adults) { _adults = adults; }
  void Reservation::setNumberOfChildren(int children) { _children = children; }
  void Reservation::setReservationOwnerPerson(boost::optional<int> personId) { _reservationOwnerPersonId = personId; }
  ReservationAtom* Reservation::addAtom(int room, boost::gregorian::date_period dateRange)
  {
    if (_atoms.empty() || _atoms[_atoms.size() - 1]->dateRange().end() == dateRange.begin())
    {
      auto atom = new ReservationAtom(this, room, dateRange);
      _atoms.push_back(std::unique_ptr<ReservationAtom>(atom));
      return atom;
    }
    else
    {
      std::cerr << "Cannot add atom because the date range is not contiguous to the previous atom" << std::endl;
      return nullptr;
    }
  }

  ReservationAtom* Reservation::addContinuation(int room, boost::gregorian::date date)
  {
    if (_atoms.empty())
    {
      std::cerr << "Cannot add continuation, because reservation has no atom yet" << std::endl;
      return nullptr;
    }

    auto& lastAtom = *_atoms.rbegin();
    if (lastAtom->_dateRange.end() >= date)
    {
      std::cerr << "Cannot create reservation continuation... The given date preceeds the end date of the last atom"
                << std::endl;
      return nullptr;
    }

    auto atom = new ReservationAtom(this, room, boost::gregorian::date_period(lastAtom->_dateRange.end(), date));
    _atoms.push_back(std::unique_ptr<ReservationAtom>(atom));
    return atom;
  }

  Reservation::ReservationStatus Reservation::status() const { return _status; }
  const std::string& Reservation::description() const { return _description; }
  int Reservation::numberOfAdults() const { return _adults; }
  int Reservation::numberOfChildren() const { return _children; }
  boost::optional<int> Reservation::reservationOwnerPersonId() { return _reservationOwnerPersonId; }
  const std::vector<std::unique_ptr<ReservationAtom>>& Reservation::atoms() const { return _atoms; }

  boost::gregorian::date_period Reservation::dateRange() const
  {
    return boost::gregorian::date_period(_atoms.front()->dateRange().begin(), _atoms.back()->dateRange().end());
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

    return (_atoms[_atoms.size() - 1]->_dateRange.end() - _atoms[0]->_dateRange.begin()).days();
  }

} // namespace hotel
