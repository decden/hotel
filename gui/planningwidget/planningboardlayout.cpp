#include "gui/planningwidget/planningboardlayout.h"

namespace gui
{
  namespace planningwidget
  {

    PlanningBoardRowGeometry::PlanningBoardRowGeometry(PlanningBoardRowGeometry::RowType type, int top, int height,
                                                       bool even, int id)
        : _type(type), _id(id), _top(top), _height(height), _isEven(even)
    {
    }

    PlanningBoardLayout::PlanningBoardLayout()
    {
      _originDate = boost::gregorian::day_clock::local_day();
      _pivotDate = _originDate;
      _roomRowHeight = 22;
      _dateColumnWidth = 26;
    }

    void PlanningBoardLayout::initializeLayout(const std::vector<std::unique_ptr<hotel::Hotel>>& hotels, LayoutType layoutType)
    {
      const int hotelSeparatorHeight = 20;
      const int categorySeparatorHeight = 10;

      _rows.clear();
      if (layoutType == GroupedByHotel)
      {
        appendSeparatorRow(hotelSeparatorHeight);
        for (auto& hotel : hotels)
        {
          bool isEven = true;
          for (auto& room : hotel->rooms())
          {
            appendRoomRow(isEven, room->id());
            isEven = !isEven;
          }
          appendSeparatorRow(hotelSeparatorHeight);
        }
      }
      else if (layoutType == GroupedByRoomCategory)
      {
        appendSeparatorRow(hotelSeparatorHeight);
        for (auto& hotel : hotels)
        {
          for (auto& category : hotel->categories())
          {
            bool isEven = true;

            std::vector<hotel::HotelRoom*> roomsInCategory;
            for (auto& hotelInner : hotels)
              for (auto& room : hotelInner->rooms())
                if (room->category()->id() == category->id())
                  roomsInCategory.push_back(room.get());

            for (auto& room : roomsInCategory)
            {
              appendRoomRow(isEven, room->id());
              isEven = !isEven;
            }
            if (!roomsInCategory.empty())
              appendSeparatorRow(categorySeparatorHeight);
          }
          if (!_rows.empty())
            _rows.pop_back();
          appendSeparatorRow(hotelSeparatorHeight);
        }
      }

      _sceneRect.setHeight(getHeight());
    }

    QRectF PlanningBoardLayout::getAtomRect(int roomId, boost::gregorian::date_period dateRange) const
    {
      auto pos = getDatePositionX(dateRange.begin());
      auto width = dateRange.length().days() * _dateColumnWidth - 1;

      auto row = getRowGeometryForRoom(roomId);
      int y = row != nullptr ? row->top() : 0;
      int height = row != nullptr ? row->height() : 0;
      return QRectF(pos, y, width, height);
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

    const PlanningBoardRowGeometry *PlanningBoardLayout::getRowGeometryAtPosition(int posY) const
    {
      for (auto& row : _rows)
      {
        if (posY >= row.top() && posY < row.bottom())
          return &row;
      }
      return nullptr;
    }

    std::pair<boost::gregorian::date, int> PlanningBoardLayout::getNearestDatePosition(int positionX) const
    {
      auto dateIndex = static_cast<int>(std::floor((positionX + _dateColumnWidth / 2.0) / _dateColumnWidth));
      return std::make_pair(_originDate + boost::gregorian::days(dateIndex), dateIndex * _dateColumnWidth);
    }

    int PlanningBoardLayout::getHeight() const
    {
      if (_rows.empty())
        return 0;

      return _rows.back().bottom();
    }

    void PlanningBoardLayout::appendRoomRow(bool isEven, int roomId)
    {
      auto type = PlanningBoardRowGeometry::RoomRow;
      _rows.emplace_back(type, getHeight(), _roomRowHeight, isEven, roomId);
    }

    void PlanningBoardLayout::appendSeparatorRow(int separatorHeight)
    {
      auto type = PlanningBoardRowGeometry::SeparatorRow;
      _rows.emplace_back(type, getHeight(), separatorHeight);
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

  } // namespace planningwidget
} // namespace gui
