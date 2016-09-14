#include "gui/planningboardlayout.h"

namespace gui
{

  PlanningBoardRowGeometry::PlanningBoardRowGeometry(PlanningBoardRowGeometry::RowType type, int top, int height,
                                                     bool even, int id)
      : _type(type), _id(id), _top(top), _height(height), _isEven(even)
  {
  }

  PlanningBoardLayout::PlanningBoardLayout()
  {
    _originDate = boost::gregorian::day_clock::local_day();
    _roomRowHeight = 22;
    _dateColumnWidth = 26;
  }

  void PlanningBoardLayout::updateRoomGeometry(hotel::HotelCollection& hotels)
  {
    const int hotelSeparatorHeight = 20;

    _rows.clear();
    _rows.push_back(PlanningBoardRowGeometry{PlanningBoardRowGeometry::SeparatorRow, 0, hotelSeparatorHeight});
    int y = hotelSeparatorHeight;
    for (auto& hotel : hotels.hotels())
    {
      bool isEven = true;
      for (auto& room : hotel->rooms())
      {
        _rows.push_back(
            PlanningBoardRowGeometry{PlanningBoardRowGeometry::RoomRow, y, _roomRowHeight, isEven, room->id()});
        y += _roomRowHeight;
        isEven = !isEven;
      }

      _rows.push_back(PlanningBoardRowGeometry{PlanningBoardRowGeometry::SeparatorRow, y, hotelSeparatorHeight});
      y += hotelSeparatorHeight;
    }

    _sceneRect.setHeight(getHeight());
  }

  QRectF PlanningBoardLayout::getAtomRect(int roomId, boost::gregorian::date_period dateRange) const
  {
    auto pos = getDatePositionX(dateRange.begin());
    auto width = dateRange.length().days() * _dateColumnWidth - 1;

    auto row = getRowGeometryForRoom(roomId);
    int y = row != nullptr ? row->top() : 0;
    return QRectF(pos, y, width, _roomRowHeight);
  }

  int PlanningBoardLayout::getDatePositionX(boost::gregorian::date date) const
  {
    return (date - _originDate).days() * _dateColumnWidth;
  }

  const PlanningBoardRowGeometry* PlanningBoardLayout::getRowGeometryForRoom(int roomId) const
  {
    for (auto& row : _rows)
    {
      if (row.rowType() == PlanningBoardRowGeometry::RoomRow && row.id() == roomId)
        return &row;
    }
    return nullptr;
  }

  std::pair<boost::gregorian::date, int> PlanningBoardLayout::getNearestDatePosition(int positionX) const
  {
    auto dateIndex = (positionX + _dateColumnWidth / 2) / _dateColumnWidth;
    return std::make_pair(_originDate + boost::gregorian::days(dateIndex), dateIndex * _dateColumnWidth);
  }

  int PlanningBoardLayout::getHeight() const
  {
    if (_rows.empty())
      return 0;
    else
      return _rows.back().bottom();
  }

  void PlanningBoardAppearance::drawRowBackground(QPainter* painter, const PlanningBoardRowGeometry& row,
                                                  const QRect& rect) const
  {
    if (row.rowType() == PlanningBoardRowGeometry::SeparatorRow)
    {
      QLinearGradient grad(0, rect.top(), 0, rect.bottom());
      grad.setColorAt(0.0, boardSeparatorColor.darker(200));
      grad.setColorAt(0.5, boardSeparatorColor);

      painter->fillRect(rect, grad);
      painter->setPen(boardEvenRowColor);
      painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
    }
    else if (row.rowType() == PlanningBoardRowGeometry::RoomRow)
    {
      auto color = row.isRowEven() ? boardEvenRowColor : boardOddRowColor;
      painter->fillRect(rect, color);
    }
  }

  const QString ShortDaysOfWeek[7] = {"su", "mo", "tu", "we", "th", "fr", "sa"};
  const QString& PlanningBoardAppearance::getShortWeekdayName(int dayOfWeek) const
  {
    assert(dayOfWeek >= 0 && dayOfWeek < 7);
    return ShortDaysOfWeek[dayOfWeek];
  }

  const QString MonthNames[12] = {"January", "February", "March",     "April",   "May",       "June",
                                  "July",    "August",   "September", "October", "Novermber", "December"};
  const QString& PlanningBoardAppearance::getMonthName(int month) const
  {
    assert(month >= 1 && month <= 12);
    return MonthNames[month - 1];
  }

} // namespace gui
