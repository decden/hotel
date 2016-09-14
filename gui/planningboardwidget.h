#ifndef GUI_PLANNINGBOARDWIDGET_H
#define GUI_PLANNINGBOARDWIDGET_H

#include "gui/planningboardlayout.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/persistence/sqlitestorage.h"
#include "hotel/planning.h"
#include "hotel/reservation.h"

#include <QGraphicsView>

#include <boost/date_time.hpp>

#include <map>
#include <memory>
#include <string>

namespace gui
{
  /**
   * @brief The PlanningBoardWidget class is a widget showing the reservations onto a virtual planning board
   *
   * @see PlanningBoardReservationItem
   */
  class PlanningBoardWidget : public QGraphicsView
  {
  public:
    PlanningBoardWidget(const PlanningBoardLayout* layout);
    void addReservations(const std::vector<std::unique_ptr<hotel::Reservation>>& reservations);

  protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
    virtual void drawForeground(QPainter* painter, const QRectF& rect) override;

  private:
    hotel::PlanningBoard* _planning;
    hotel::HotelCollection* _hotels;

    QGraphicsScene* _scene;
    const PlanningBoardLayout* _layout;
  };

} // namespace gui

#endif // GUI_PLANNINGBOARDWIDGET_H
