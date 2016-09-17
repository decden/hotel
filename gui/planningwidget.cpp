#include "planningwidget.h"

#include <QGridLayout>
#include <QKeyEvent>

namespace gui
{
  PlanningWidget::PlanningWidget(std::unique_ptr<hotel::persistence::SqliteStorage> storage)
  {
    // Load the Data
    _hotelsData = storage->loadHotels();
    _planningData = storage->loadPlanning(_hotelsData->allRoomIDs());

    // Initialize the layout object with the above data
    _layout.initializeLayout(*_hotelsData, planningwidget::PlanningBoardLayout::GroupedByRoomCategory);
    _layout.setPivotDate(boost::gregorian::day_clock::local_day());

    // Initialize scene geometry
    // TODO: This should be combined together with a fixed minimum width
    auto planningExtent = _planningData->getPlanningExtent();
    // Extend the date range so that it starts one week prior to the first reservation and extends for at least one year
    auto dateRange = planningExtent.merge(boost::gregorian::date_period(
          planningExtent.begin() + boost::gregorian::days(-7),
          planningExtent.begin() + boost::gregorian::days(365)));

    auto left = _layout.getDatePositionX(dateRange.begin()) - 10;
    auto right = _layout.getDatePositionX(dateRange.end()) + 10;
    _layout.setSceneRect(QRectF(left, 0, right - left, _layout.getHeight()));

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
    for (auto room : _hotelsData->allRooms())
      _roomList->addRoomItem(room);
    _planningBoard->addReservations(_planningData->reservations());
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
      _layout.initializeLayout(*_hotelsData, planningwidget::PlanningBoardLayout::GroupedByHotel);
    if (event->key() == Qt::Key_F2)
      _layout.initializeLayout(*_hotelsData, planningwidget::PlanningBoardLayout::GroupedByRoomCategory);

    if (event->key() == Qt::Key_F1 || event->key() == Qt::Key_F2)
    {
      updateLayout();
    }
  }

  void PlanningWidget::updateLayout()
  {
    _planningBoard->updateLayout();
    _roomList->updateLayout();
    _dateBar->updateLayout();
  }

} // namespace gui
