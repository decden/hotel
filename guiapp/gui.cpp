#include "hotel/persistence/sqlitestorage.h"

#include "gui/planningwidget.h"
#include "gui/planningwidget/newreservationtool.h"

#include <QApplication>
#include <QGridLayout>
#include <QWindow>
#include <QPushButton>
#include <QScrollBar>

#include <memory>

int main(int argc, char** argv)
{
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);

  auto storage = std::make_unique<hotel::persistence::SqliteStorage>("test.db");
  auto hotelCollection = storage->loadHotels();
  auto planning = storage->loadPlanning(hotelCollection->allRoomIDs());

  gui::PlanningWidget widget(hotelCollection.get());
  widget.registerTool("new-reservation", std::make_unique<gui::planningwidget::NewReservationTool>());
  widget.activateTool("new-reservation");

  widget.setObservedPlanningBoard(planning.get());
  widget.show();

  return app.exec();
}
