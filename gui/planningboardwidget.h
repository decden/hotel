#ifndef GUI_PLANNINGBOARDWIDGET_H
#define GUI_PLANNINGBOARDWIDGET_H

#include "gui/planningboardlayout.h"

#include "hotel/hotel.h"
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
    PlanningBoardWidget(hotel::PlanningBoard* planning, std::vector<std::unique_ptr<hotel::Hotel>>* hotels);

  protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
    virtual void drawForeground(QPainter* painter, const QRectF& rect) override;

  private:
    void addReservations(const std::vector<std::unique_ptr<hotel::Reservation>>& reservations);

    hotel::PlanningBoard* _planning;
    std::vector<std::unique_ptr<hotel::Hotel>>* _hotels;

    QGraphicsScene _scene;
    PlanningBoardLayout _layout;
  };

} // namespace gui

#endif // GUI_PLANNINGBOARDWIDGET_H
