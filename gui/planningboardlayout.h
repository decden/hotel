#ifndef GUI_PLANNINGBOARDLAYOUT_H
#define GUI_PLANNINGBOARDLAYOUT_H

#include "hotel/hotel.h"

#include <boost/date_time.hpp>

#include <QColor>
#include <QFont>
#include <QRect>

namespace gui
{
  /**
   * @brief The PlanningBoardRowGeometry class holds the goemetry and appearance of one row in the planning board
   */
  class PlanningBoardRowGeometry
  {
  public:
    enum RowType
    {
      RoomRow,
      SeparatorRow
    };

    PlanningBoardRowGeometry(RowType type, int top, int height, int id = 0);
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
   * @brief The PlanningBoardAppearance struct holds colors and constants defining the appearance of the planning board
   */
  struct PlanningBoardAppearance
  {
    // General colors
    QColor widgetBackground = QColor(0xCCCCCC);
    QColor selectionColor = QColor(0xD33682);

    // Atom constants
    int atomConnectionHandleSize = 3;
    int atomConnectionOverhang = 10;
    // Atom colors
    QColor atomDefaultColor = QColor(0xE0D5B3);
    QColor atomCheckedInColor = QColor(0x9EC2A9);
    QColor atomCheckedOutColor = QColor(0xC0BFBB);
    QColor atomUnconfirmedColor = QColor(0xD4885C);
    QColor atomArchivedColor = QColor(0xECEBE8);
    QColor atomSelectedColor = QColor(0xD33682);
    QColor atomArchivedSelectedColor = QColor(0xF8DEEB);
    QColor atomDarkTextColor = QColor(0x586E75);
    QColor atomLightTextColor = QColor(0xffffff);
    // Atom fonts
    QFont atomTextFont = QFont("Arial", 9);

    // Planning board constants
    int boardSaturdayColumnWidth = 4;
    int boardSundayColumnWidth = 6;
    // Planning board colors
    QColor boardEvenRowColor = QColor(0xFDF6E3);
    QColor boardOddRowColor = QColor(0xEEE8D5);
    QColor boardWeekdayColumnColor = QColor(0xE3D9BA);
    QColor boardSaturdayColumnColor = QColor(0xE3D097);
    QColor boardSundayColumnColor = QColor(0xE3D097);
    QColor boardTodayBar = QColor(0x26, 0x8B, 0xD2, 0xA0);
    QColor boardSeparatorColor = QColor(0x353F41);
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

    /**
     * @brief getNearestDate returns the nearest date to the given position, together with the actual x-position of the
     *        found date.
     */
    std::pair<boost::gregorian::date, int> getNearestDatePosition(int positionX) const;

    //! @brief getHeight returns the total heiht of the planning board, according to the current layout.
    int getHeight() const;
    int dateColumnWidth() const { return _dateColumnWidth; }

    const std::vector<PlanningBoardRowGeometry>& rowGeometries() { return _rows; }
    const PlanningBoardAppearance& appearance() const { return _appearance; }

  private:
    int _roomRowHeight;
    int _dateColumnWidth;

    PlanningBoardAppearance _appearance;

    // List of ordered, non-overlapping row definitions
    std::vector<PlanningBoardRowGeometry> _rows;

    // The date which will correspond to the x=0 line
    boost::gregorian::date _originDate;
  };

} // namespace gui

#endif // GUI_PLANNINGBOARDLAYOUT_H
