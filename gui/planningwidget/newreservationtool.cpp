#include "gui/planningwidget/newreservationtool.h"

#include "gui/planningwidget/context.h"

#include "persistence/op/operations.h"

#include <QInputDialog>
#include <QGraphicsScene>
#include <QPainter>

namespace gui
{
  namespace planningwidget
  {
    using namespace boost::gregorian;

    NewReservationTool::NewReservationTool()
        : _context(nullptr), _ghosts(), _currentGhost(boost::none)
    {
    }
    void NewReservationTool::init(Context& context) { _context = &context; }
    void NewReservationTool::load() {}

    void NewReservationTool::unload()
    {
      _ghosts.clear();
      _currentGhost.reset();
    }

    void NewReservationTool::updateLayout()
    {
      for (auto& ghost : _ghosts)
        ghost.item->updateLayout();
      if (_currentGhost)
        _currentGhost->item->updateLayout();
    }

    void NewReservationTool::reservationAdded(const hotel::Reservation& item)
    {
      // A reservation was just added. Make sure it does not overlap with any ghost
      for (auto& ghost : _ghosts)
      {
        if (item.intersectsWith(*ghost.temporaryReservation))
          ghost.item = nullptr;
      }

      if (_currentGhost && item.intersectsWith(*_currentGhost->temporaryReservation))
        _currentGhost = boost::none;

      _ghosts.erase(std::remove_if(_ghosts.begin(), _ghosts.end(), [](auto& ghost) { return ghost.item == nullptr; }),
                    _ghosts.end());
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
      auto maximumDate = computeMaximumDate(row->id(), date);
      if (maximumDate == date)
        return;

      // Create a new reservation atom (note: the associated reservation is initially empty!)
      _currentGhost = ReservationGhost{nullptr, nullptr};
      _currentGhost->temporaryReservation = std::make_unique<hotel::Reservation>("new");
      _currentGhost->temporaryReservation->setStatus(hotel::Reservation::Temporary);
      _currentGhost->item = std::make_unique<PlanningBoardReservationItem>(_context, _currentGhost->temporaryReservation.get());
      _currentGhost->startRoomId = row->id();
      _currentGhost->startDate = date;
      _currentGhost->item->updateLayout();

      _context->planningBoardScene()->addItem(_currentGhost->item.get());
    }

    void NewReservationTool::mouseReleaseEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost)
      {
        // Either delete the current ghost, or add it to the list of ghosts
        if (!_currentGhost->temporaryReservation->dateRange().is_null())
          _ghosts.push_back(std::move(*_currentGhost));
        _currentGhost = boost::none;
      }
    }

    void NewReservationTool::mouseMoveEvent(QMouseEvent* event, const QPointF& position)
    {
      if (_currentGhost != boost::none)
      {
        date currentDate;
        int dateXPos;
        std::tie(currentDate, dateXPos) = _context->layout().getNearestDatePosition(position.x());

        // We modify the last atom in the reservation if it has the same room as the current one
        auto lastAtom = _currentGhost->temporaryReservation->lastAtom();
        auto currentRoomId = lastAtom ? lastAtom->roomId() : _currentGhost->startRoomId;

        bool modifyLastAtom = lastAtom != nullptr && lastAtom->roomId() == currentRoomId;
        auto currentStartDate = modifyLastAtom ? lastAtom->dateRange().begin() : _currentGhost->startDate;
        auto currentEndDate = modifyLastAtom ? lastAtom->dateRange().end() : _currentGhost->startDate;

        // Allow switching to a new room when pressing shift
        if (event->modifiers() & Qt::ShiftModifier)
        {
          auto& layout = _context->layout();
          auto row = layout.getRowGeometryAtPosition(position.y());
          if (row != nullptr && row->rowType() == PlanningBoardRowGeometry::RoomRow && row->id() != currentRoomId)
          {
            currentRoomId = row->id();
            modifyLastAtom = lastAtom != nullptr && lastAtom->roomId() == currentRoomId;

            if (modifyLastAtom)
              currentStartDate = lastAtom->dateRange().begin();
            else if (lastAtom != nullptr)
              currentStartDate = lastAtom->dateRange().end();
            else
              currentStartDate = _currentGhost->startDate;
          }
        }

        // Dates cannot be extended in the past by dragging. Neither can they be extended in the future
        auto currentMaximumDate = computeMaximumDate(currentRoomId, currentStartDate);
        if (currentDate < currentStartDate)
          currentDate = currentStartDate;
        if (currentDate > currentMaximumDate)
          currentDate = currentMaximumDate;

        auto newPeriod = date_period(currentStartDate, currentDate);

        // If we are allowed to modify the last atom in the reservation we first remove it, and we later add it back
        // with updated data (if the date range is not empty)
        if (modifyLastAtom)
          _currentGhost->temporaryReservation->removeLastAtom();
        if (!newPeriod.is_null())
          _currentGhost->temporaryReservation->addAtom(currentRoomId, newPeriod);

        _currentGhost->item->updateLayout();
      }
    }

    void NewReservationTool::keyPressEvent(QKeyEvent *event)
    {
      // Ignore key events while manipulating a ghost
      if (_currentGhost != boost::none)
        return;

      if (event->key() == Qt::Key_Escape)
      {
        unload();
        load();
      }
      else if (event->key() == Qt::Key_Backspace)
      {
        if (!_ghosts.empty())
          _ghosts.pop_back();
      }
      else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
      {
        if (!_ghosts.empty())
        {
          // Ask the user about the title
          auto reservationName = QInputDialog::getText(nullptr, "Reservation Description", "");
          if (reservationName.isEmpty())
            return;

          // Gather reservations
          std::vector<std::unique_ptr<hotel::Reservation>> reservations;
          for (auto& ghost : _ghosts)
          {
            ghost.item.reset();\
            ghost.temporaryReservation->setStatus(hotel::Reservation::New);
            ghost.temporaryReservation->setDescription(reservationName.toStdString());
            reservations.push_back(std::move(ghost.temporaryReservation));
          }
          _ghosts.clear();

          namespace op = persistence::op;
          for (auto& reservation : reservations)
            _context->dataSource().queueOperation(op::StoreNewReservation{std::move(reservation)});
        }
      }
    }

    date NewReservationTool::computeMaximumDate(int roomId, date fromDate)
    {
      auto& planning = _context->planning();
      auto availableDays = planning.getAvailableDaysFrom(roomId, fromDate);
      auto maximumDate = fromDate + boost::gregorian::days(availableDays);

      // Narrow down the availability by also considering the ghosts, such that the user cannot create two overlapping
      // temporary reservation atoms
      for (auto& ghost : _ghosts)
      {
        for (auto& ghostAtom : ghost.temporaryReservation->atoms())
        {
          if (ghostAtom.roomId() == roomId)
          {
            if (ghostAtom.dateRange().contains(fromDate))
              return fromDate;
            auto atomBeginDate = ghostAtom.dateRange().begin();
            if (atomBeginDate >= fromDate && atomBeginDate < maximumDate)
              maximumDate = atomBeginDate;
          }
        }
      }

      return maximumDate;
    }

  } // namespace planningwidget
} // namespace gui
