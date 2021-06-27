#ifndef HOTEL_RESERVATION_H
#define HOTEL_RESERVATION_H

#include "hotel/persistentobject.h"
#include "hotel/reservation.h"

#include <boost/date_time.hpp>

#include <memory>
#include <optional>
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
    Reservation(const Reservation& that) = default;
    Reservation(Reservation&& that) = default;
    Reservation& operator=(const Reservation& that) = default;
    Reservation& operator=(Reservation&& that) = default;

    void setStatus(ReservationStatus status);
    void setDescription(const std::string& newDescription);
    void setNumberOfAdults(int adults);
    void setNumberOfChildren(int adults);
    void setReservationOwnerPerson(std::optional<int> personId);

    void addAtom(int room, boost::gregorian::date_period dateRange);
    void addAtom(const ReservationAtom& atom);
    void addContinuation(int room, boost::gregorian::date date);
    void joinAdjacentAtoms(); // Joins adjacent atoms on the same room
    void removeLastAtom();
    void removeAllAtoms();

    ReservationStatus status() const;
    const std::string& description() const;
    int numberOfAdults() const;
    int numberOfChildren() const;
    std::optional<int> reservationOwnerPersonId() const;

    const std::vector<ReservationAtom>& atoms() const;
    std::vector<ReservationAtom>& atoms();
    const ReservationAtom* atomAtIndex(int i) const;
    const ReservationAtom* firstAtom() const;
    const ReservationAtom* lastAtom() const;

    boost::gregorian::date_period dateRange() const;
    bool intersectsWith(const Reservation& other) const;

    //! @brief Returns true if the reservation contains at least one atom, and all of the periods are continuous
    bool isValid() const;
    int length() const;

  private:
    ReservationStatus _status;
    std::string _description;

    std::optional<int> _reservationOwnerPersonId;

    int _adults;
    int _children;
    std::vector<ReservationAtom> _atoms;
  };

  bool operator==(const Reservation& a, const Reservation& b);
  bool operator!=(const Reservation& a, const Reservation& b);

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

    //! Returns true if two items overlap
    bool intersectsWith(const ReservationAtom& other) const;

  private:
    int _roomId;
    boost::gregorian::date_period _dateRange;
  };

  bool operator==(const ReservationAtom& a, const ReservationAtom& b);
  bool operator!=(const ReservationAtom& a, const ReservationAtom& b);

} // namespace hotel

#endif // HOTEL_RESERVATION_H
