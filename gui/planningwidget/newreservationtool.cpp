#include "gui/planningwidget/newreservationtool.h"

#include "gui/planningwidget/context.h"

#include "persistence/op/operations.h"

#include <QGraphicsScene>
#include <QPainter>

namespace gui
{
  namespace planningwidget
  {
    using namespace boost::gregorian;

    void ReservationGhostItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
      // Date range is empty or invalid: no need to draw something
      if (atom.dateRange().is_null())
        return;

      auto renderer = _context.appearance().reservationRenderer();
      renderer->paintAtom(painter, _context, reservation, atom, rect(), false);
    }

    NewReservationTool::NewReservationTool()
        : _context(nullptr), _ghosts(), _currentGhost(nullptr)
    {
    }
    void NewReservationTool::init(Context& context) { _context = &context; }
    void NewReservationTool::load() {}

    void NewReservationTool::unload()
    {
      auto boardScene = _context->planningBoardScene();

      for (auto ghost : _ghosts)
        delete ghost;
      _ghosts.clear();

      if (_currentGhost != nullptr)
        boardScene->removeItem(_currentGhost);
      _currentGhost = nullptr;
    }

    void NewReservationTool::updateLayout()
    {
      for (auto ghost : _ghosts)
        ghost->updateLayout();
      if (_currentGhost)
        _currentGhost->updateLayout();
    }

    void NewReservationTool::mousePressEvent(QMouseEvent* event, const QPointF& position)
    {
      auto& layout = _context->layout();

      // Get date and row
      boost::gregorian::date date;
      int dateXPos;
      std::tie(date, dateXPos) = layout.getNearestDatePosition(position.x());
      auto row = layout.getRowGeometryAtPosition(position.y());
      if (row == nullptr || row->rowType() != PlanningBoardRowGeometry::RoomRow)
        return;

      // Compute availability
      auto& planning = _context->planning();
      auto availableDays = planning.getAvailableDaysFrom(row->id(), date);
      auto maximumDate = date + boost::gregorian::days(availableDays);

      // Narrow down the availability by also considering the ghosts, such that the user cannot create two overlapping
      // temporary reservation atoms
      for (auto ghost : _ghosts)
      {
        if (ghost->atom.roomId() == row->id())
        {
          if (ghost->atom.dateRange().contains(date))
            return;
          auto atomBeginDate = ghost->atom.dateRange().begin();
          if (atomBeginDate >= date && atomBeginDate < maximumDate)
            maximumDate = atomBeginDate;
        }
      }

      if (maximumDate == date)
        return;

      _currentGhost = new ReservationGhostItem(*_context);
      _currentGhost->maximumDate = maximumDate;
      _currentGhost->atom.setDateRange(date_period(date, date));
      _currentGhost->atom.setRoomId(row->id());
      _currentGhost->updateLayout();
      _context->planningBoardScene()->addItem(_currentGhost);
    }

    void NewReservationTool::mouseReleaseEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost)
      {
        if (_currentGhost->atom.dateRange().is_null())
          delete _currentGhost;
        else
          _ghosts.push_back(_currentGhost);
      }
      _currentGhost = nullptr;
    }

    void NewReservationTool::mouseMoveEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost != nullptr)
      {
        date currentDate;
        int dateXPos;
        std::tie(currentDate, dateXPos) = _context->layout().getNearestDatePosition(position.x());

        auto currentPeriod = _currentGhost->atom.dateRange();

        // Dates cannot be extended in the past by dragging
        if (currentDate < currentPeriod.begin())
          currentDate = currentPeriod.begin();
        if (currentDate > _currentGhost->maximumDate)
          currentDate = _currentGhost->maximumDate;

        auto newPeriod = date_period(currentPeriod.begin(), currentDate);
        _currentGhost->atom.setDateRange(newPeriod);

        _currentGhost->updateLayout();
      }
    }



    void NewReservationTool::keyPressEvent(QKeyEvent *event)
    {
      // Ignore key events while manipulating a ghost
      if (_currentGhost != nullptr)
        return;

      if (event->key() == Qt::Key_Escape)
      {
        unload();
        load();
      }
      else if (event->key() == Qt::Key_Backspace)
      {
        if (!_ghosts.empty())
        {
          delete _ghosts[_ghosts.size() - 1];
          _ghosts.pop_back();
        }
      }
      else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
      {
        // Gather reservations
        std::vector<std::unique_ptr<hotel::Reservation>> reservations;
        for(auto ghost : _ghosts)
        {
          auto reservation = std::make_unique<hotel::Reservation>(std::move(ghost->reservation));
          reservation->addAtom(ghost->atom);
          reservations.push_back(std::move(reservation));
          delete ghost;
        }
        _ghosts.clear();

        namespace op = persistence::op;
        for (auto& reservation : reservations)
          _context->dataSource().queueOperation(op::StoreNewReservation{std::move(reservation)});
      }
    }

  } // namespace planningwidget
} // namespace gui
