#include "planningwidget.h"

#include <QGridLayout>

namespace gui
{

  PlanningWidget::PlanningWidget(std::unique_ptr<hotel::persistence::SqliteStorage> storage)
  {
    // Load the Data
    _hotelsData = storage->loadHotels();
    std::vector<int> roomIds;
    for (auto& hotel : _hotelsData)
      for (auto& room : hotel->rooms())
        roomIds.push_back(room->id());
    _planningData = storage->loadPlanning(roomIds);

    // Initialize the layout object with the above data
    _layout.updateRoomGeometry(_hotelsData);

    // Initialize scene geometry
    // TODO: This should be combined together with a fixed minimum width
    auto dateRange = _planningData->getPlanningExtent();
    auto left = _layout.getDatePositionX(dateRange.begin()) - 10;
    auto right = _layout.getDatePositionX(dateRange.end()) + 10;
    _layout.setSceneRect(QRectF(left, 0, right - left, _layout.getHeight()));

    // Create UI
    auto grid = new QGridLayout;
    grid->setMargin(0);
    grid->setSpacing(0);

    _verticalScrollbar = new QScrollBar(Qt::Vertical);
    _horizontalScrollbar = new QScrollBar(Qt::Horizontal);
    _planningBoard = new gui::PlanningBoardWidget(&_layout);
    _roomList = new gui::RoomListWidget(&_layout);
    _dateBar = new gui::DateBarWidget(&_layout);

    grid->addWidget(_dateBar, 0, 1);
    grid->addWidget(_roomList, 1, 0);
    grid->addWidget(_planningBoard, 1, 1);
    grid->addWidget(_verticalScrollbar, 1, 2);
    grid->addWidget(_horizontalScrollbar, 2, 1);
    setLayout(grid);

    // Wire up the scroll bars
    _planningBoard->setHorizontalScrollBar(_horizontalScrollbar);
    _planningBoard->setVerticalScrollBar(_verticalScrollbar);
    _roomList->setVerticalScrollBar(_verticalScrollbar);
    _dateBar->setHorizontalScrollBar(_horizontalScrollbar);

    // Add data to the sub-widgets
    for (auto& hotel : _hotelsData)
      for (auto& room : hotel->rooms())
        _roomList->addRoomItem(room.get());
    _planningBoard->addReservations(_planningData->reservations());
  }

} // namespace gui
