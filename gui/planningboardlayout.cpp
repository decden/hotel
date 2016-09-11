#include "gui/planningboardlayout.h"

namespace gui
{
  PlanningBoardLayout::PlanningBoardLayout()
  {
    _originDate = boost::gregorian::day_clock::local_day();
    _roomRowHeight = 24;
    _dateColumnWidth = 30;
  }

  void PlanningBoardLayout::updateRoomGeometry(std::vector<std::unique_ptr<hotel::Hotel>>& hotels)
  {
    const int hotelSeparatorHeight = 20;

    _rows.clear();
    int y = 0;
    for (auto& hotel : hotels)
    {
      for (auto& room : hotel->rooms())
      {
        _rows.push_back(PlanningBoardRowGeometry{PlanningBoardRowGeometry::RoomRow, y, _roomRowHeight, room->id()});
        y += _roomRowHeight;
      }

      _rows.push_back(PlanningBoardRowGeometry{PlanningBoardRowGeometry::SeparatorRow, y, hotelSeparatorHeight});
      y += hotelSeparatorHeight;
    }
  }

  QRectF PlanningBoardLayout::getAtomRect(int roomId, boost::gregorian::date_period dateRange)
  {
    auto pos = getDatePositionX(dateRange.begin());
    auto width = dateRange.length().days() * _dateColumnWidth - 1;

    int y = 0;
    for (auto& row : _rows)
    {
      if (row.rowType() == PlanningBoardRowGeometry::RoomRow && row.id() == roomId)
      {
        y = row.top();
        break;
      }
    }

    return QRectF(pos, y, width, _roomRowHeight);
  }

  int PlanningBoardLayout::getDatePositionX(boost::gregorian::date date) const
  {
    return (date - _originDate).days() * _dateColumnWidth;
  }

  int PlanningBoardLayout::getHeight() const
  {
    if (_rows.empty())
      return 0;
    else
      return _rows.back().bottom();
  }

} // namespace gui
