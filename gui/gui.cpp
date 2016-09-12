#include <QApplication>
#include <QGridLayout>
#include <QWindow>
#include <QPushButton>

#include "gui/planningboardwidget.h"
#include "gui/roomlistwidget.h"

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
  layout->setSpacing(0);
  layout->addWidget(new gui::PlanningBoardWidget(planning.get(), &hotels), 1, 1);
  layout->addWidget(new gui::RoomListWidget(&hotels), 1, 0);

  QWidget window;
  window.setLayout(layout);
  window.show();

  return app.exec();
}
