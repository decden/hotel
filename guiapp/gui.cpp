#include "guiapp/testdata.h"

#include "persistence/net/netclientbackend.h"
#include "persistence/sqlite/sqlitebackend.h"

#include "gui/dialogs/editreservation.h"
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

  persistence::net::NetClientBackend backend("localhost", 46835);
  //persistence::sqlite::SqliteBackend backend("test.db");

  if (app.arguments().contains("--createTestData"))
    guiapp::createTestData(backend);

  gui::PlanningWidget widget(backend);
  widget.registerTool("new-reservation", std::make_unique<gui::planningwidget::NewReservationTool>());
  widget.activateTool("new-reservation");

  QObject::connect(&widget, &gui::PlanningWidget::reservationDoubleClicked, [&](const hotel::Reservation &res)
  {
    if (res.status() != hotel::Reservation::Temporary)
    {
      auto dialog = new gui::dialogs::EditReservationDialog(backend, res.id());
      dialog->show();
    }
  });

  widget.showMaximized();

  gui::ChangeIntegrator integrator(&backend);

  return app.exec();
}
