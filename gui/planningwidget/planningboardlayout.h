#ifndef GUI_PLANNINGBOARDLAYOUT_H
#define GUI_PLANNINGBOARDLAYOUT_H

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"

#include "gui/planningwidget/reservationrenderer.h"

#include <boost/date_time.hpp>

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QRect>

namespace gui
{
  namespace planningwidget
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

      PlanningBoardRowGeometry(RowType type, int top, int height, bool even = true, int id = 0);

      void setId(int id) { _id = id; }
      void setRowEven(bool even) { _isEven = even; }

      RowType rowType() const { return _type; }
      int id() const { return _id; }
      int top() const { return _top; }
      int bottom() const { return _top + _height; }
      int height() const { return _height; }
      bool isRowEven() const { return _isEven; }

    private:
      RowType _type;
      int _id;
      int _top;
      int _height;
      bool _isEven;
    };

    /**
     * @brief The PlanningBoardAppearance struct holds colors and constants defining the appearance of the planning
     * board
     */
    struct PlanningBoardAppearance
    {
      // General colors
      QColor widgetBackground = QColor(0xCCCCCC);
      QColor selectionColor = QColor(0xD33682);
      QFont headerFont = QFont("Arial", 10);
      QFont boldHeaderFont = QFont("Arial", 10, QFont::Bold);

      // Reservation renderers
      ReservationRenderer reservationRendererDefault;
      PrivacyReservationRenderer reservationRendererPrivacy;
      HighlightArrivalsRenderer reservationRendererArrivals;

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
      QColor boardPivotTodayColor = QColor(0x268BD2);
      QColor boardPivotColor = QColor(0x26D2AC);
      QColor boardSeparatorColor = QColor(0x353F41);

      // Room list constants
      int roomListWidth = 100;
      int monthBarHeight = 20;
      int daysBarHeight = 40;
      // Room list fonts
      QFont roomListCategoryFont = QFont("Arial", 8);

      // Utility functions for rendering
      const ReservationRenderer* reservationRenderer() const { return &reservationRendererArrivals; }
      void drawRowBackground(QPainter* painter, const PlanningBoardRowGeometry& row, const QRect& rect) const;

      // Utility functions for text
      const QString& getShortWeekdayName(int dayOfWeek) const;
      const QString& getMonthName(int month) const;
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

      enum LayoutType
      {
        GroupedByHotel,
        GroupedByRoomCategory
      };

      /**
       * @brief initializeLayout initializes the rows contained in layout, given the provided hotels and rooms
       * @param hotels the list of hotels to include in the layout
       * @param layoutType indicates how to position the rows
       */
      void initializeLayout(hotel::HotelCollection& hotels, LayoutType layoutType);

      void setSceneRect(const QRectF& rect) { _sceneRect = rect; }
      QRectF sceneRect() const { return _sceneRect; }

      void setPivotDate(const boost::gregorian::date date) { _pivotDate = date; }
      boost::gregorian::date pivotDate() const { return _pivotDate; }

      //! @brief getAtomRect produces the rectangle for the given room and date on the virtual planning board
      QRectF getAtomRect(int roomId, boost::gregorian::date_period dateRange) const;

      //! @brief getPositionX returns the x coordiante associated to the given date, w.r.t. the layout's origin date
      int getDatePositionX(boost::gregorian::date date) const;

      const PlanningBoardRowGeometry* getRowGeometryForRoom(int roomId) const;

      /**
       * @brief getNearestDate returns the nearest date to the given position, together with the actual x-position of
       * the found date.
       */
      std::pair<boost::gregorian::date, int> getNearestDatePosition(int positionX) const;

      //! @brief getHeight returns the total heiht of the planning board, according to the current layout.
      int getHeight() const;
      int dateColumnWidth() const { return _dateColumnWidth; }

      const std::vector<PlanningBoardRowGeometry>& rowGeometries() const { return _rows; }
      const PlanningBoardAppearance& appearance() const { return _appearance; }

    private:
      int _roomRowHeight;
      int _dateColumnWidth;

      PlanningBoardAppearance _appearance;

      // Scene rect
      QRectF _sceneRect;

      // List of ordered, non-overlapping row definitions
      std::vector<PlanningBoardRowGeometry> _rows;

      // The currently selected date
      boost::gregorian::date _pivotDate;

      // The date which will correspond to the x=0 line. This value does not represent the currently selected date, it
      // is only used for setting a coordinate system, in which to place the reservations.
      boost::gregorian::date _originDate;

      void appendRoomRow(bool isEven, int roomId);
      void appendSeparatorRow(int separatorHeight);
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGBOARDLAYOUT_H
