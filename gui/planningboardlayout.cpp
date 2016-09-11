#include "gui/planningboardlayout.h"

namespace gui
{
  PlanningBoardLayout::PlanningBoardLayout() { _originDate = boost::gregorian::day_clock::local_day(); }

  void PlanningBoardLayout::updateRoomGeometry(std::vector<std::unique_ptr<hotel::Hotel>>& hotels)
  {
    _rows.clear();
    int y = 0;
    for (auto& hotel : hotels)
    {
      for (auto& room : hotel->rooms())
      {
        _rows.push_back(RowDefinition{RoomRow, room->id(), y, 20});
        y += 20;
      }

      _rows.push_back(RowDefinition{SeparatorRow, 0, y, 10});
      y += 10;
    }
  }

  QRectF PlanningBoardLayout::getAtomRect(int roomId, boost::gregorian::date_period dateRange)
  {
    auto pos = getDatePositionX(dateRange.begin());
    auto width = dateRange.length().days() * 24 - 1;

    int y = 0;
    for (auto& row : _rows)
    {
      if (row.rowType == RoomRow && row.id == roomId)
      {
        y = row.top;
        break;
      }
    }

    return QRectF(pos, y, width, 20);
  }

  int PlanningBoardLayout::getDatePositionX(boost::gregorian::date date) const
  {
    return (date - _originDate).days() * 24;
  }

  int PlanningBoardLayout::getHeight() const
  {
    if (_rows.empty())
      return 0;
    else
      return _rows.back().bottom();
  }

} // namespace gui
