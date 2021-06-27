#include "gui/planningwidget/movereservationtool.h"

#include "gui/planningwidget/context.h"

#include "persistence/op/operations.h"

#include <QGuiApplication>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QInputDialog>

namespace gui
{
  namespace planningwidget
  {
    using namespace boost::gregorian;

    MoveReservationTool::MoveReservationTool() : _context(nullptr), _currentGhost(std::nullopt) {}
    void MoveReservationTool::init(Context& context) { _context = &context; }
    void MoveReservationTool::load() {}

    void MoveReservationTool::unload() { _currentGhost.reset(); }

    void MoveReservationTool::updateLayout()
    {
      if (_currentGhost)
        _currentGhost->item->updateLayout();
    }

    void MoveReservationTool::reservationAdded(const hotel::Reservation& item)
    {
      // A reservation was just added. Make sure it does not overlap with the ghost
      if (_currentGhost && item.intersectsWith(*_currentGhost->temporaryReservation))
        _currentGhost = std::nullopt;
    }

    void MoveReservationTool::atomMousePressEvent(const hotel::Reservation& reservation,
                                                  const hotel::ReservationAtom& atom, const QPointF& /*position*/)
    {
      // Initiate a new interaction?
      if (!_currentGhost.has_value())
      {
        const auto& atoms = reservation.atoms();
        auto atomIt = std::find_if(atoms.begin(), atoms.end(), [&](const auto& x) { return &x == &atom; });
        assert(atomIt != atoms.end());

        // Create a copy of the reservation for editing
        auto ghostReservation = std::make_unique<hotel::Reservation>(reservation);
        ghostReservation->setStatus(hotel::Reservation::Temporary);
        auto ghostItem = std::make_unique<PlanningBoardReservationItem>(_context, ghostReservation.get());
        ghostItem->setZValue(4);
        int atomIndex = std::distance(atoms.begin(), atomIt);

        _currentGhost = ReservationGhost{std::move(ghostItem), std::move(ghostReservation), reservation.status(),
                                         atomIt->roomId(), atomIndex};
        _currentGhost->item->updateLayout();
        _context->planningBoardScene()->addItem(_currentGhost->item.get());
      }
    }

    void MoveReservationTool::mouseReleaseEvent(QMouseEvent* /*event*/, const QPointF& /*position*/)
    {
      if (_currentGhost)
      {
        _currentGhost->temporaryReservation->setStatus(_currentGhost->originalStatus);
        _currentGhost->temporaryReservation->joinAdjacentAtoms();

        // Commit change
        namespace op = persistence::op;
        _context->dataBackend().queueOperation(op::Update{std::move(_currentGhost->temporaryReservation)});
      }
      _currentGhost.reset();
    }

    void MoveReservationTool::mouseMoveEvent(QMouseEvent* /*event*/, const QPointF& position)
    {
      if (_currentGhost.has_value())
      {
        const auto& layout = _context->layout();
        const auto row = layout.getRowGeometryAtPosition(static_cast<int>(position.y()));

        // Move the atom to the given row
        if (row != nullptr && row->rowType() == PlanningBoardRowGeometry::RoomRow)
        {
          auto id = row->id();
          auto& atom = _currentGhost->temporaryReservation->atoms()[_currentGhost->atomIndex];

          // Check if room is still available
          if (id == _currentGhost->originalRoomId || _context->planning().isFree(id, atom.dateRange()))
          {
            atom.setRoomId(id);
            _currentGhost->item->updateLayout();
          }
        }
      }
    }

    void MoveReservationTool::keyPressEvent(QKeyEvent* /*event*/) {}

  } // namespace planningwidget
} // namespace gui
