#include "guiapp/testdata.h"

#include "persistence/datasource.h"
#include "persistence/sqlite/sqlitestorage.h"

#include "gui/datasourcechangeintegrator.h"
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

  persistence::DataSource dataSource("test.db");

  if (app.arguments().contains("--createTestData"))
    guiapp::createTestData(dataSource);

  gui::PlanningWidget widget(dataSource);
  widget.registerTool("new-reservation", std::make_unique<gui::planningwidget::NewReservationTool>());
  widget.activateTool("new-reservation");
  widget.showMaximized();

  gui::ChangeIntegrator integrator(&dataSource);

  return app.exec();
}
