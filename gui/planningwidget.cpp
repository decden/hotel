#include "planningwidget.h"

#include <QGridLayout>
#include <QKeyEvent>

namespace gui
{
  PlanningWidget::PlanningWidget(hotel::HotelCollection* hotelCollection)
  {
    // Assign the data
    _hotelCollection = hotelCollection;

    // Initialize the layout object with the above data
    _layout.setPivotDate(boost::gregorian::day_clock::local_day());
    _layout.initializeLayout(*_hotelCollection, planningwidget::PlanningBoardLayout::GroupedByRoomCategory);

    updateDateRange();

    // Create UI
    auto grid = new QGridLayout;
    grid->setMargin(0);
    grid->setSpacing(0);

    _verticalScrollbar = new QScrollBar(Qt::Vertical);
    _horizontalScrollbar = new QScrollBar(Qt::Horizontal);
    _planningBoard = new planningwidget::PlanningBoardWidget(&_layout);
    _roomList = new planningwidget::RoomListWidget(&_layout);
    _dateBar = new planningwidget::DateBarWidget(&_layout);

    // Wire up the scroll bars
    _planningBoard->setHorizontalScrollBar(_horizontalScrollbar);
    _planningBoard->setVerticalScrollBar(_verticalScrollbar);
    _roomList->setVerticalScrollBar(_verticalScrollbar);
    _dateBar->setHorizontalScrollBar(_horizontalScrollbar);

    // Wire up events
    connect(_dateBar, SIGNAL(dateClicked(boost::gregorian::date)), this, SLOT(setPivotDate(boost::gregorian::date)));

    grid->addWidget(_dateBar, 0, 1);
    grid->addWidget(_roomList, 1, 0);
    grid->addWidget(_planningBoard, 1, 1);
    grid->addWidget(_verticalScrollbar, 1, 2);
    grid->addWidget(_horizontalScrollbar, 2, 1);
    setLayout(grid);

    // Add data to the sub-widgets
    for (auto room : _hotelCollection->allRooms())
      _roomList->addRoomItem(room);
  }

  void PlanningWidget::setPivotDate(boost::gregorian::date pivotDate)
  {
    // TODO: This should not update the whole layout!
    _layout.setPivotDate(pivotDate);
    updateLayout();
  }

  void PlanningWidget::keyPressEvent(QKeyEvent* event)
  {
    if (event->key() == Qt::Key_F1)
      _layout.initializeLayout(*_hotelCollection, planningwidget::PlanningBoardLayout::GroupedByHotel);
    if (event->key() == Qt::Key_F2)
      _layout.initializeLayout(*_hotelCollection, planningwidget::PlanningBoardLayout::GroupedByRoomCategory);

    if (event->key() == Qt::Key_F1 || event->key() == Qt::Key_F2)
    {
      updateLayout();
    }
  }

  void PlanningWidget::reservationsAdded(const std::vector<const hotel::Reservation*>& reservations)
  {
    _planningBoard->addReservations(reservations);
  }

  void PlanningWidget::reservationsRemoved(const std::vector<const hotel::Reservation*>& reservations)
  {
    _planningBoard->removeReservations(reservations);
  }

  void PlanningWidget::initialUpdate(const hotel::PlanningBoard& board)
  {
    _planningBoard->addReservations(board.reservations());
    updateDateRange();
    updateLayout();
  }

  void PlanningWidget::allReservationsRemoved() { _planningBoard->removeAllReservations(); }

  void PlanningWidget::updateDateRange()
  {
    auto planningExtent = boost::gregorian::date_period(_layout.pivotDate(), _layout.pivotDate());
    auto planningBoard = observedPlanningBoard();
    if (planningBoard != nullptr)
      planningExtent = planningBoard->getPlanningExtent();

    // Extend the date range so that it starts one week prior to the first reservation and extends for at least one year
    auto dateRange = planningExtent.merge(boost::gregorian::date_period(
        planningExtent.begin() + boost::gregorian::days(-7), planningExtent.begin() + boost::gregorian::days(365)));

    // Apply the scene size
    auto left = _layout.getDatePositionX(dateRange.begin()) - 10;
    auto right = _layout.getDatePositionX(dateRange.end()) + 10;
    _layout.setSceneRect(QRectF(left, 0, right - left, _layout.getHeight()));
  }

  void PlanningWidget::updateLayout()
  {
    _planningBoard->updateLayout();
    _roomList->updateLayout();
    _dateBar->updateLayout();
  }

} // namespace gui
