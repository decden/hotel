#include <QApplication>
#include <QGridLayout>
#include <QWindow>
#include <QPushButton>
#include <QScrollBar>

#include "gui/planningboardwidget.h"
#include "gui/roomlistwidget.h"
#include "gui/datebarwidget.h"

#include "hotel/persistence/sqlitestorage.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  auto storage = std::make_unique<hotel::persistence::SqliteStorage>("test.db");
  auto hotels = storage->loadHotels();
  std::vector<int> roomIds;
  for (auto& hotel : hotels)
    for (auto& room : hotel->rooms())
      roomIds.push_back(room->id());
  auto planning = storage->loadPlanning(roomIds);

  auto layout = new QGridLayout();
  auto verticalScrollbar = new QScrollBar(Qt::Vertical);
  auto horizontalScrollbar = new QScrollBar(Qt::Horizontal);
  auto planningBoard = new gui::PlanningBoardWidget(planning.get(), &hotels);
  auto roomList = new gui::RoomListWidget(&hotels);
  auto dateBar = new gui::DateBarWidget();

  layout->setSpacing(0);
  layout->addWidget(dateBar, 0, 1);
  layout->addWidget(roomList, 1, 0);
  layout->addWidget(planningBoard, 1, 1);
  layout->addWidget(verticalScrollbar, 1, 2);
  layout->addWidget(horizontalScrollbar, 2, 1);

  planningBoard->setVerticalScrollBar(verticalScrollbar);
  planningBoard->setHorizontalScrollBar(horizontalScrollbar);
  roomList->setVerticalScrollBar(verticalScrollbar);
  dateBar->setHorizontalScrollBar(horizontalScrollbar);

  QWidget window;
  window.setLayout(layout);
  window.show();

  return app.exec();
}
