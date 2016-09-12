#include "gui/planningboardwidget.h"

#include "gui/planningboardreservationitem.h"

#include <QPainter>

namespace gui
{
  PlanningBoardWidget::PlanningBoardWidget(hotel::PlanningBoard* planning,
                                           std::vector<std::unique_ptr<hotel::Hotel>>* hotels)
      : QGraphicsView(), _planning(planning), _hotels(hotels)
  {
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFrameStyle(QFrame::Plain);
    setCacheMode(QGraphicsView::CacheBackground);
    setScene(&_scene);

    _layout.updateRoomGeometry(*hotels);

    // Add the reservations
    addReservations(_planning->reservations());

    // Compute the size of the scen
    auto dateRange = _planning->getPlanningExtent();
    auto left = _layout.getDatePositionX(dateRange.begin()) - 10;
    auto right = _layout.getDatePositionX(dateRange.end()) + 10;
    _scene.setSceneRect(QRectF(left, 0, right - left, _layout.getHeight()));
  }

  void PlanningBoardWidget::drawBackground(QPainter* painter, const QRectF& rect)
  {
    auto& appearance = _layout.appearance();
    painter->fillRect(rect, appearance.widgetBackground);

    // Draw the horizontal alternating rows
    for (auto& row : _layout.rowGeometries())
    {
      if (row.rowType() == PlanningBoardRowGeometry::RoomRow)
        appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
    }

    // Get the horizontal date range
    auto leftDatePos = _layout.getNearestDatePosition(rect.left() - _layout.dateColumnWidth());

    // Draw the vertical day lines
    auto posX = leftDatePos.second;
    auto dayOfWeek = leftDatePos.first.day_of_week();
    painter->setPen(appearance.boardWeekdayColumnColor);
    while (posX < rect.right() + _layout.dateColumnWidth())
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
      posX += _layout.dateColumnWidth();
      dayOfWeek = (dayOfWeek + 1) % 7;
    }

    // Fill in the separator rows (we do not want to have vertical lines in there)
    for (auto row : _layout.rowGeometries())
    {
      if (row.rowType() == PlanningBoardRowGeometry::SeparatorRow)
        appearance.drawRowBackground(painter, row, QRect(rect.left(), row.top(), rect.width(), row.height()));
    }
  }

  void PlanningBoardWidget::drawForeground(QPainter* painter, const QRectF& rect)
  {
    // Draw the bar indicating the current day
    auto today = boost::gregorian::day_clock::local_day();
    auto x = _layout.getDatePositionX(today);
    auto todayRect = QRect(x - 2, rect.top(), 3, rect.height());
    painter->fillRect(todayRect, _layout.appearance().boardTodayBar);
  }

  void PlanningBoardWidget::addReservations(const std::vector<std::unique_ptr<hotel::Reservation>>& reservations)
  {
    for (auto& reservation : reservations)
    {
      auto item = new PlanningBoardReservationItem(&_layout, reservation.get());
      _scene.addItem(item);
    }
  }

} // namespace gui
