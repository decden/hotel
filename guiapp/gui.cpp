#include <QApplication>
#include <QGridLayout>
#include <QWindow>
#include <QPushButton>
#include <QScrollBar>

#include "gui/planningwidget.h"

#include "hotel/persistence/sqlitestorage.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  auto storage = std::make_unique<hotel::persistence::SqliteStorage>("test.db");
  auto hotelCollection = storage->loadHotels();
  auto planning = storage->loadPlanning(hotelCollection->allRoomIDs());

  gui::PlanningWidget widget(hotelCollection.get());
  widget.setObservedPlanningBoard(planning.get());
  widget.show();

  gui::PlanningWidget widget2(hotelCollection.get());
  widget2.setObservedPlanningBoard(planning.get());
  widget2.show();

  return app.exec();
}
