#include "gui/planningwidget/planningboardwidget.h"

#include "gui/planningwidget/planningboardreservationitem.h"

#include <QPainter>

namespace gui
{
  namespace planningwidget
  {

    PlanningBoardWidget::PlanningBoardWidget(const PlanningBoardLayout* layout) : QGraphicsView(), _layout(layout)
    {
      setAlignment(Qt::AlignLeft | Qt::AlignTop);
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      setFrameStyle(QFrame::Plain);
      setCacheMode(QGraphicsView::CacheBackground);

      _scene = new QGraphicsScene;
      _scene->setSceneRect(_layout->sceneRect());
      setScene(_scene);

      // Init tools
      if (_tool)
      {
        _tool->init(layout, _scene);
        _tool->load();
      }

      // No scrollbars
      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void PlanningBoardWidget::drawBackground(QPainter* painter, const QRectF& rect)
    {
      auto& appearance = _layout->appearance();
      painter->fillRect(rect, appearance.widgetBackground);

      // Draw the horizontal alternating rows
      for (auto& row : _layout->rowGeometries())
      {
        if (row.rowType() == PlanningBoardRowGeometry::RoomRow)
          appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
      }

      // Get the horizontal date range
      auto leftDatePos = _layout->getNearestDatePosition(rect.left() - _layout->dateColumnWidth());

      // Draw the vertical day lines
      auto posX = leftDatePos.second;
      auto dayOfWeek = leftDatePos.first.day_of_week();
      painter->setPen(appearance.boardWeekdayColumnColor);
      while (posX < rect.right() + _layout->dateColumnWidth())
      {
        if (dayOfWeek == boost::gregorian::Sunday)
        {
          int w = appearance.boardSundayColumnWidth;
          painter->fillRect(QRectF(posX - w / 2, rect.top(), w, rect.height()),
                            QColor(appearance.boardSundayColumnColor));
        }
        else if (dayOfWeek == boost::gregorian::Saturday)
        {
          int w = appearance.boardSaturdayColumnWidth;
          painter->fillRect(QRectF(posX - w / 2, rect.top(), w, rect.height()),
                            QColor(appearance.boardSaturdayColumnColor));
        }
        else
          painter->drawLine(posX - 1, rect.top(), posX - 1, rect.bottom());
        posX += _layout->dateColumnWidth();
        dayOfWeek = (dayOfWeek + 1) % 7;
      }

      // Fill in the separator rows (we do not want to have vertical lines in there)
      for (auto row : _layout->rowGeometries())
      {
        if (row.rowType() == PlanningBoardRowGeometry::SeparatorRow)
          appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
      }
    }

    void PlanningBoardWidget::drawForeground(QPainter* painter, const QRectF& rect)
    {
      // Draw the bar indicating the pivot day
      auto pivotDay = _layout->pivotDate();
      auto x = _layout->getDatePositionX(pivotDay);
      auto todayRect = QRect(x - 2, rect.top(), 3, rect.height());
      auto lineColor = _layout->appearance().boardPivotTodayColor;
      lineColor.setAlpha(0xA0);
      painter->fillRect(todayRect, lineColor);
    }

    void PlanningBoardWidget::addReservations(const std::vector<const hotel::Reservation*>& reservations)
    {
      for (auto& reservation : reservations)
      {
        auto item = new PlanningBoardReservationItem(_layout, reservation);
        _scene->addItem(item);
      }
    }

    void PlanningBoardWidget::removeReservations(const std::vector<const hotel::Reservation*>& reservations)
    {
      // Make a set out of the list
      std::set<const hotel::Reservation*> reservationSet;
      for (auto& reservation : reservations)
        reservationSet.insert(reservation);

      // Remove all of the corresponiding reservations
      for (auto item : _scene->items())
      {
        auto reservationItem = dynamic_cast<PlanningBoardReservationItem*>(item);
        if (reservationItem != nullptr)
        {
          if (reservationSet.find(reservationItem->reservation()) != reservationSet.end())
            _scene->removeItem(reservationItem);
        }
      }
    }

    void PlanningBoardWidget::removeAllReservations()
    {
      // Remove all of the corresponiding reservations
      for (auto item : _scene->items())
      {
        auto reservationItem = dynamic_cast<PlanningBoardReservationItem*>(item);
        if (reservationItem != nullptr)
          _scene->removeItem(reservationItem);
      }
    }

    void PlanningBoardWidget::updateLayout()
    {
      for (auto item : _scene->items())
      {
        // TODO: We should maintain a list of reservation which are currently diplayed, instead of resorting do
        // dynamic_casting things
        auto reservationItem = dynamic_cast<PlanningBoardReservationItem*>(item);
        if (reservationItem != nullptr)
          reservationItem->updateLayout();
      }

      _scene->setSceneRect(_layout->sceneRect());
      invalidateBackground();
      invalidateForeground();
    }

    void PlanningBoardWidget::invalidateBackground()
    {
      _scene->invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
      update();
    }

    void PlanningBoardWidget::invalidateForeground()
    {
      _scene->invalidate(sceneRect(), QGraphicsScene::ForegroundLayer);
      update();
    }

  } // namespace planningwidget
} // namespace gui
