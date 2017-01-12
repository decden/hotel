#ifndef HOTEL_PLANNING_H
#define HOTEL_PLANNING_H

#include "hotel/reservation.h"

#include "hotel/observablecollection.h"

#include <boost/date_time.hpp>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace hotel
{
typedef ObservableCollection<const Reservation*> ObservablePlanningBoard;
typedef CollectionObserver<const Reservation*> PlanningBoardObserver;

  /**
   * @brief The PlanningBoard class holds planning information for a given set of rooms.
   *
   * The PlanningBoard essentially holds a list of Reservation, which themselves hold a list of ReservationAtom, which
   * in turn represents a room, which is occupied over a given date period. Each atom holds additional information
   * needed
   * for display.
   *
   * @see Reservation
   * @see ReservationAtom
   */
  class PlanningBoard
  {
  public:
    PlanningBoard& operator=(const PlanningBoard& that);
    PlanningBoard& operator=(PlanningBoard&& that);

    /**
     * @brief addRoomId Adds a room on the planning board
     * @param roomId the id of the room to add
     */
    void addRoomId(int roomId);
    /**
     * @brief addReservation tries to add the given reservation to the planning board
     * @param reservation the reservation to add
     * @return a pointer to the added reservation on success, otherwise nullptr.
     */
    Reservation* addReservation(std::unique_ptr<Reservation> reservation);
    /**
     * @brief removeReservation deletes the given reservation from the planning board
     * @param reservation the reservation to delete
     */
    void removeReservation(const Reservation* reservation);

    /**
     * @brief clear deletes all reservations and rooms from the current planning board.
     * The method also notifies the observers
     */
    void clear();

    /**
     * @brief canAddReservation returns true if there is availability for the whole reservation
     */
    bool canAddReservation(const Reservation& reservation) const;

    bool isFree(int roomId, boost::gregorian::date_period period) const;
    bool hasRoom(int roomId) const;

    /**
     * @brief getAvailableDaysFrom computes the number of days in which the given room is available from the given date
     *        onwards.
     * @return the number of days for which the room is free. If the room is unavailable 0 is returend. If the room is
     *         always available max is returned.
     */
    int getAvailableDaysFrom(int roomId, boost::gregorian::date date) const;

    std::vector<Reservation*> reservations();
    std::vector<const Reservation*> reservations() const;
    std::vector<Reservation*> getReservationsInPeriod(boost::gregorian::date_period period);
    std::vector<const Reservation*> getReservationsInPeriod(boost::gregorian::date_period period) const;

    const Reservation* getReservationById(int id) const;

    /**
     * @brief getPlanningExtent Returns the date period encompassing all of the reservations
     * @return If there are no reservation, an empty period is returned, encompassing the current day.
     */
    boost::gregorian::date_period getPlanningExtent() const;

    void addObserver(PlanningBoardObserver* observer);
    void removeObserver(PlanningBoardObserver* observer);

  private:
    /**
     * @brief insertAtom Inserts a given reservation atom to the PlanningBoard.
     * @note This function does not verify constraints to avoid overlapping atoms.
     */
    void insertAtom(const ReservationAtom* atom); // TODO: Make this more efficient

    std::vector<std::unique_ptr<Reservation>> _reservations;
    std::map<int, std::vector<const ReservationAtom*>> _rooms;

    //! Used to notify observers about changes to the collection
    ObservablePlanningBoard _observableCollection;
  };

} // namespace hotel

#endif // HOTEL_PLANNING_H
