#include <QApplication>
#include <QWindow>

#include "gui/planningboardwidget.h"

#include "hotel/persistence/sqlitestorage.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  auto storage = std::make_unique<hotel::persistence::SqliteStorage>("test.db");
  gui::PlanningBoardWidget planningWidget(std::move(storage));
  planningWidget.show();

  return app.exec();
}
