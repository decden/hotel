#ifndef HOTEL_RESERVATION_H
#define HOTEL_RESERVATION_H

#include "hotel/persistentobject.h"
#include "hotel/reservation.h"

#include <boost/date_time.hpp>
#include <boost/optional.hpp>

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
      Temporary, // Used for temporary selections in the planning board
      New,
      Confirmed,
      CheckedIn,
      CheckedOut,
      Archived
    };

    Reservation(const std::string& description);
    Reservation(const std::string& description, int roomId, boost::gregorian::date_period dateRange);
    Reservation(Reservation&& that);

    void setStatus(ReservationStatus status);
    void setDescription(const std::string& newDescription);
    void setNumberOfAdults(int adults);
    void setNumberOfChildren(int adults);
    void setReservationOwnerPerson(boost::optional<int> personId);

    ReservationAtom* addAtom(int room, boost::gregorian::date_period dateRange);
    ReservationAtom* addContinuation(int room, boost::gregorian::date date);

    ReservationStatus status() const;
    const std::string& description() const;
    int numberOfAdults() const;
    int numberOfChildren() const;
    boost::optional<int> reservationOwnerPersonId();

    const std::vector<std::unique_ptr<ReservationAtom>>& atoms() const;
    const ReservationAtom* firstAtom() const;
    const ReservationAtom* lastAtom() const;

    boost::gregorian::date_period dateRange() const;

    //! @brief Returns true if the reservation contains at least one atom, and all of the periods are continuous
    const bool isValid() const;
    const int length() const;

  private:
    ReservationStatus _status;
    std::string _description;

    boost::optional<int> _reservationOwnerPersonId;

    int _adults;
    int _children;
    std::vector<std::unique_ptr<ReservationAtom>> _atoms;
  };

  /**
   * @brief The ReservationAtom class represents one single reserved room over a given date period.
   */
  class ReservationAtom : public PersistentObject
  {
  public:
    ReservationAtom(const int room, boost::gregorian::date_period dateRange);
    ReservationAtom(const ReservationAtom& that) = default;

    int roomId() const { return _roomId; }
    boost::gregorian::date_period dateRange() const { return _dateRange; }

    void setDateRange(boost::gregorian::date_period dateRange) { _dateRange = dateRange; }
    void setRoomId(int id) { _roomId = id; }

  private:
    int _roomId;
    boost::gregorian::date_period _dateRange;
  };

  bool operator==(const ReservationAtom& a, const ReservationAtom& b);
  bool operator!=(const ReservationAtom& a, const ReservationAtom& b);

} // namespace hotel

#endif // HOTEL_RESERVATION_H
