#include "gui/planningwidget/newreservationtool.h"

#include "gui/planningwidget/context.h"

#include <QGraphicsScene>
#include <QPainter>

namespace gui
{
  namespace planningwidget
  {

    void ReservationGhostItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      painter->fillRect(rect(), QColor(0xff00ff));
    }

    NewReservationTool::NewReservationTool() : _context(nullptr), _ghosts(), _currentGhost(nullptr)
    {
    }

    void NewReservationTool::init(Context &context)
    {
      _context = &context;
    }

    void NewReservationTool::load() {}

    void NewReservationTool::unload()
    {
      auto boardScene = _context->planningBoardScene();

      for (auto ghost : _ghosts)
        boardScene->removeItem(ghost);
      _ghosts.clear();

      if (_currentGhost != nullptr)
        boardScene->removeItem(_currentGhost);
      _currentGhost = nullptr;
    }

    void NewReservationTool::updateLayout()
    {
      // TODO update layout of ghosts
    }

    void NewReservationTool::mousePressEvent(QMouseEvent* event, const QPointF& position)
    {
      unload();
      load();

      auto& layout = _context->layout();

      // Get date and row
      boost::gregorian::date date;
      int dateXPos;
      std::tie(date, dateXPos) = layout.getNearestDatePosition(position.x());
      auto row = layout.getRowGeometryAtPosition(position.y());
      if (row == nullptr || row->rowType() != PlanningBoardRowGeometry::RoomRow)
        return;

      _currentGhost = new ReservationGhostItem(*_context);
      _currentGhost->startDate = date;
      _currentGhost->endDate = date;
      _currentGhost->roomId = row->id();
      _currentGhost->updateLayout();
      _context->planningBoardScene()->addItem(_currentGhost);
    }

    void NewReservationTool::mouseReleaseEvent(QMouseEvent* event, const QPointF& position) {}

    void NewReservationTool::mouseMoveEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost != nullptr)
      {
        boost::gregorian::date date;
        int dateXPos;
        std::tie(date, dateXPos) = _context->layout().getNearestDatePosition(position.x());

        if (date < _currentGhost->startDate)
          date = _currentGhost->startDate;
        _currentGhost->endDate = date;

        _currentGhost->updateLayout();
      }
    }

  } // namespace planningwidget
} // namespace gui
