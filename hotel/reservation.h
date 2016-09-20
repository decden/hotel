#ifndef HOTEL_RESERVATION_H
#define HOTEL_RESERVATION_H

#include "hotel/persistentobject.h"
#include "hotel/reservation.h"

#include <boost/date_time.hpp>

#include <memory>
#include <string>
#include <vector>

namespace hotel
{

  class ReservationAtom;

  /**
   * @brief The Reservation class represents a single reservation over a given date period
   *
   * One reservation supports changes of room, during the period of stay. To represent this, a Reservation is composed
   * of one or more ReservationAtom.
   *
   * This class does not store extensive information about the reservation, like owner and prices. This class is
   * primarely meant for display.
   *
   * More detailed information is stored in the DetailedReservation class.
   *
   * @see ReservationAtom
   * @see DetailedReservation
   */
  class Reservation : public PersistentObject
  {
  public:
    enum ReservationStatus
    {
      Unknown,
      New,
      Confirmed,
      CheckedIn,
      CheckedOut,
      Archived
    };

    Reservation(const std::string& description);
    Reservation(const std::string& description, int roomId, boost::gregorian::date_period dateRange);

    void setStatus(ReservationStatus status);
    void setDescription(const std::string& newDescription);

    ReservationAtom* addAtom(int room, boost::gregorian::date_period dateRange);
    ReservationAtom* addContinuation(int room, boost::gregorian::date date);

    ReservationStatus status() const;
    const std::string& description() const;
    const std::vector<std::unique_ptr<ReservationAtom>>& atoms() const;

    boost::gregorian::date_period dateRange() const;

    //! @brief Returns true if the reservation contains at least one atom, and all of the periods are continuous
    const bool isValid() const;
    const int length() const;

  private:
    ReservationStatus _status;
    std::string _description;
    std::vector<std::unique_ptr<ReservationAtom>> _atoms;
  };

  /**
   * @brief The ReservationAtom class represents one single reserved room over a given date period
   *
   * Objects of this class are owned by a Reservation. Each Atom keeps a back pointer to the parent reservation.
   */
  class ReservationAtom : public PersistentObject
  {
  public:
    ReservationAtom(Reservation* reservation, const int room, boost::gregorian::date_period dateRange);

    const Reservation* reservation() const { return _reservation; }
    int roomId() const { return _roomId; }
    boost::gregorian::date_period dateRange() const { return _dateRange; }

    bool isFirst() const { return _reservation && this == _reservation->atoms().begin()->get(); }
    bool isLast() const { return _reservation && this == _reservation->atoms().rbegin()->get(); }

  public: // TODO: Public for now...
    Reservation* _reservation;
    int _roomId;
    boost::gregorian::date_period _dateRange;
  };

} // namespace hotel

#endif // HOTEL_RESERVATION_H
