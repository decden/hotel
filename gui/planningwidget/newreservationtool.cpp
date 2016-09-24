#include "newreservationtool.h"

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

    NewReservationTool::NewReservationTool() : _layout(nullptr), _boardScene(nullptr), _ghosts(), _currentGhost(nullptr)
    {
    }

    void NewReservationTool::init(const PlanningBoardLayout* layout, QGraphicsScene* boardScene)
    {
      _layout = layout;
      _boardScene = boardScene;
    }

    void NewReservationTool::load() {}

    void NewReservationTool::unload()
    {
      for (auto ghost : _ghosts)
        _boardScene->removeItem(ghost);
      _ghosts.clear();

      if (_currentGhost != nullptr)
        _boardScene->removeItem(_currentGhost);
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

      // Get date and row
      boost::gregorian::date date;
      int dateXPos;
      std::tie(date, dateXPos) = _layout->getNearestDatePosition(position.x());
      auto row = _layout->getRowGeometryAtPosition(position.y());
      if (row == nullptr || row->rowType() != PlanningBoardRowGeometry::RoomRow)
        return;

      _currentGhost = new ReservationGhostItem(_layout);
      _currentGhost->startDate = date;
      _currentGhost->endDate = date;
      _currentGhost->roomId = row->id();
      _currentGhost->updateLayout();
      _boardScene->addItem(_currentGhost);
    }

    void NewReservationTool::mouseReleaseEvent(QMouseEvent* event, const QPointF& position) {}

    void NewReservationTool::mouseMoveEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost != nullptr)
      {
        boost::gregorian::date date;
        int dateXPos;
        std::tie(date, dateXPos) = _layout->getNearestDatePosition(position.x());

        if (date < _currentGhost->startDate)
          date = _currentGhost->startDate;
        _currentGhost->endDate = date;

        _currentGhost->updateLayout();
      }
    }

  } // namespace planningwidget
} // namespace gui
