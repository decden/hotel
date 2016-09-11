#ifndef GUI_PLANNINGBOARDLAYOUT_H
#define GUI_PLANNINGBOARDLAYOUT_H

#include "hotel/hotel.h"

#include <boost/date_time.hpp>

#include <QColor>
#include <QRect>

namespace gui
{
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

    QColor widgetBackground = QColor(0xcccccc);
    QColor selectionColor = QColor(0xC3AB08);

  private:
    enum RowType
    {
      RoomRow,
      SeparatorRow
    };

    struct RowDefinition
    {
      RowType rowType;
      int id;
      int top;
      int height;

      int bottom() const { return top + height; }
    };

    // List of ordered, non-overlapping row definitions
    std::vector<RowDefinition> _rows;

    // The date which will correspond to the x=0 line
    boost::gregorian::date _originDate;
  };

} // namespace gui

#endif // GUI_PLANNINGBOARDLAYOUT_H
