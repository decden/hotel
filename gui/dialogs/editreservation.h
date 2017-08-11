#ifndef GUIAPP_DIALOGS_EDITRESERVATION_H
#define GUIAPP_DIALOGS_EDITRESERVATION_H

#include "gui/datastreamobserveradapter.h"

#include "persistence/datasource.h"

#include "boost/optional.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

namespace gui
{
  namespace dialogs
  {
    /**
     * @brief The EditReservationDialog allows users to edit reservation details
     */
    class EditReservationDialog : public QDialog
    {
      Q_OBJECT
    public:
      /**
       * @brief Creates a new EditReservationDialog
       * @param ds datasource to use for getting the data and modifying the data
       * @param objectId the id of the reservation to edit
       */
      EditReservationDialog(persistence::DataSource& ds, int objectId);

    private slots:
      void saveClicked();

    private:
      void reservationsAdded(const std::vector<hotel::Reservation>& reservations);
      void reservationsRemoved(const std::vector<int>& ids);
      void allReservationsRemoved();
      void saveTaskUpdated();

      persistence::DataSource& _dataSource;

      boost::optional<hotel::Reservation> _reservation;
      gui::DataStreamObserverAdapter<hotel::Reservation> _reservationStreamHandle;

      boost::signals2::scoped_connection _saveTaskUpdatedConnection;
      boost::optional<persistence::op::Task<persistence::op::OperationResults>> _saveTask;

      QComboBox* _cbxStatus;
      QLineEdit* _txtDescription;
      QSpinBox* _spbNumberOfAdults;
      QSpinBox* _spbNumberOfChildren;

      QPushButton* _btnSave;
    };
  } // namespace dialogs
} // namespaec gui

#endif // GUIAPP_DIALOGS_EDITRESERVATION_H``
