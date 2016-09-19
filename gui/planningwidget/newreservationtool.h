#ifndef GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
#define GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H

#include "gui/planningwidget/tool.h"
#include "gui/planningwidget/planningboardlayout.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>

#include <boost/date_time.hpp>

#include <vector>

namespace gui
{
  namespace planningwidget
  {

    class ReservationGhostItem : public QGraphicsRectItem
    {
    public:
      ReservationGhostItem(const PlanningBoardLayout* layout) : _layout(layout) {}

      // QGraphicsRectItem interface
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

      void updateLayout()
      {
        auto rect = _layout->getAtomRect(roomId, boost::gregorian::date_period(startDate, endDate));
        setRect(rect);
      }

      int roomId;
      boost::gregorian::date startDate;
      boost::gregorian::date endDate;

    private:
      const PlanningBoardLayout* _layout;
    };



    /**
     * @brief The NewReservationTool class is a tool allowing new reservations to be created on the board
     */
    class NewReservationTool : public Tool
    {
    public:
      NewReservationTool();

      virtual void init(const PlanningBoardLayout *layout, QGraphicsScene *boardScene) override;

      virtual void load() override;
      virtual void unload() override;
      virtual void updateLayout() override;

      virtual void mousePressEvent(QMouseEvent *event, const QPointF &position) override;
      virtual void mouseReleaseEvent(QMouseEvent *event, const QPointF &position) override;
      virtual void mouseMoveEvent(QMouseEvent *event, const QPointF& position) override;

    private:
      //! Information needed to draw on the planning widget
      const PlanningBoardLayout* _layout;
      QGraphicsScene* _boardScene;

      //! The list of temporary reservation items in the scene
      std::vector<ReservationGhostItem*> _ghosts;
      ReservationGhostItem* _currentGhost;

    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_NEWRESERVATIONTOOL_H
