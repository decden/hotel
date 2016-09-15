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
  gui::PlanningWidget widget(std::move(storage));
  widget.show();

  return app.exec();
}
