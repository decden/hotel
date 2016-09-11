#ifndef GUI_PLANNINGBOARDLAYOUT_H
#define GUI_PLANNINGBOARDLAYOUT_H

#include "hotel/hotel.h"

#include <boost/date_time.hpp>

#include <QColor>
#include <QRect>

namespace gui
{
  class PlanningBoardRowGeometry
  {
  public:
    enum RowType
    {
      RoomRow,
      SeparatorRow
    };

    PlanningBoardRowGeometry(RowType type, int top, int height, int id = 0)
        : _type(type), _id(id), _top(top), _height(height)
    {
    }

    void setId(int id) { _id = id; }

    RowType rowType() const { return _type; }
    int id() const { return _id; }
    int top() const { return _top; }
    int bottom() const { return _top + _height; }
    int height() const { return _height; }

  private:
    RowType _type;
    int _id;
    int _top;
    int _height;
  };

  /**
   * @brief The PlanningBoardLayout class holds the geometry and appearance of the PlanningBoardWidget
   *
   * One of the responsibilities of this class is to transofrm room ids and positions to coordinates in the virtual
   * planning board.
   */
  class PlanningBoardLayout
  {
  public:
    PlanningBoardLayout();

    void updateRoomGeometry(std::vector<std::unique_ptr<hotel::Hotel>>& hotels);

    //! @brief getAtomRect produces the rectangle for the given room and date on the virtual planning board
    QRectF getAtomRect(int roomId, boost::gregorian::date_period dateRange);

    //! @brief getPositionX returns the x coordiante associated to the given date, w.r.t. the layout's origin date
    int getDatePositionX(boost::gregorian::date date) const;

    //! @brief getHeight returns the total heiht of the planning board, according to the current layout.
    int getHeight() const;

    const std::vector<PlanningBoardRowGeometry>& rowGeometries() { return _rows; }

    QColor widgetBackground = QColor(0xcccccc);
    QColor selectionColor = QColor(0xD33682);
    QColor evenBackgroundRow = QColor(0xfdf6e3);
    QColor oddBackgroundRow = QColor(0xeee8d5);

  private:
    int _roomRowHeight;
    int _dateColumnWidth;

    // List of ordered, non-overlapping row definitions
    std::vector<PlanningBoardRowGeometry> _rows;

    // The date which will correspond to the x=0 line
    boost::gregorian::date _originDate;
  };

} // namespace gui

#endif // GUI_PLANNINGBOARDLAYOUT_H
