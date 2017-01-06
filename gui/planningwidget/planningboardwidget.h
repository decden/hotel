#ifndef GUI_PLANNINGBOARDWIDGET_H
#define GUI_PLANNINGBOARDWIDGET_H

#include "gui/planningwidget/context.h"
#include "gui/planningwidget/newreservationtool.h"
#include "gui/planningwidget/planningboardlayout.h"

#include "persistence/sqlite/sqlitestorage.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"
#include "hotel/reservation.h"

#include <QGraphicsView>

#include <boost/date_time.hpp>

#include <map>
#include <memory>
#include <string>

namespace gui
{
  namespace planningwidget
  {

    /**
     * @brief The PlanningBoardWidget class is a widget showing the reservations onto a virtual planning board
     *
     * @see PlanningBoardReservationItem
     */
    class PlanningBoardWidget : public QGraphicsView
    {
    public:
      PlanningBoardWidget(Context* context);
      void addReservations(const std::vector<const hotel::Reservation*>& reservations);
      void removeReservations(const std::vector<const hotel::Reservation*>& reservations);
      void removeAllReservations();

      //! When the layout changes, call this methods to update the scene.
      void updateLayout();

    protected:
      virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
      virtual void drawForeground(QPainter* painter, const QRectF& rect) override;

      virtual void mousePressEvent(QMouseEvent* event) override;
      virtual void mouseReleaseEvent(QMouseEvent* event) override;
      virtual void mouseMoveEvent(QMouseEvent* event) override;

    private:
      Context* _context;
      QGraphicsScene* _scene;

      void invalidateBackground();
      void invalidateForeground();
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGBOARDWIDGET_H
