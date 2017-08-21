#ifndef GUIAPP_DIALOGS_EDITRESERVATION_H
#define GUIAPP_DIALOGS_EDITRESERVATION_H

#include "gui/datastreamobserveradapter.h"
#include "gui/statusbar.h"

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
    class TaskObserverAdapter : public persistence::TaskObserver
    {
    public:
      void connect(persistence::Backend& backend, persistence::op::Operations ops) { _taskHandle = backend.queueOperations(std::move(ops), this); }
      void connect(persistence::Backend& backend, persistence::op::Operation op) { _taskHandle = backend.queueOperation(std::move(op), this); }

      virtual void setResults(const std::vector<persistence::TaskResult>& results) override { resultsSetSignal(results); }

      // Public signals
      boost::signals2::signal<void(const std::vector<persistence::TaskResult>&)> resultsSetSignal;

    private:
      persistence::UniqueTaskHandle _taskHandle;
    };

    class Form
    {
    public:
      typedef boost::variant<int, std::string, hotel::Reservation::ReservationStatus> Value;
      typedef std::array<Value, 4> Tuple;

      Form()
      {
        _cbxStatus = new QComboBox();
        _cbxStatus->addItems({"New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"});
        _txtDescription = new QLineEdit();
        _spbNumberOfAdults = new QSpinBox();
        _spbNumberOfAdults->setRange(0, 10);
        _spbNumberOfChildren = new QSpinBox();
        _spbNumberOfChildren->setRange(0, 10);
      }

      Tuple formValues()
      {
        auto reservationStatus = static_cast<hotel::Reservation::ReservationStatus>(_cbxStatus->currentIndex() + 2);
        return Tuple {{
          Value{reservationStatus},
          Value{_txtDescription->text().toStdString()},
          Value{_spbNumberOfAdults->value()},
          Value{_spbNumberOfChildren->value()}
        }};
      }

      void setFormValues(const Tuple& tuple)
      {
        _cbxStatus->setCurrentIndex(boost::get<hotel::Reservation::ReservationStatus>(tuple[0]) - 2);
        _txtDescription->setText(QString::fromStdString(boost::get<std::string>(tuple[1])));
        _spbNumberOfAdults->setValue(boost::get<int>(tuple[2]));
        _spbNumberOfChildren->setValue(boost::get<int>(tuple[3]));
      }

      std::vector<std::pair<const char*, QWidget*>> getWidgets()
      {
        std::vector<std::pair<const char*, QWidget*>> result;
        result.emplace_back("Status:", _cbxStatus);
        result.emplace_back("Description:", _txtDescription);
        result.emplace_back("Number of adults:", _spbNumberOfAdults);
        result.emplace_back("Number of children:", _spbNumberOfChildren);
        return result;
      }

      // Converter methods for items

      static Tuple ItemToTuple(const hotel::Reservation& item)
      {
        return {{item.status(), item.description(), item.numberOfAdults(), item.numberOfChildren()}};
      }

      static void SetItemFromTuple(hotel::Reservation& item, const Tuple& tuple)
      {
        item.setStatus(boost::get<hotel::Reservation::ReservationStatus>(tuple[0]));
        item.setDescription(boost::get<std::string>(tuple[1]));
        item.setNumberOfAdults(boost::get<int>(tuple[2]));
        item.setNumberOfChildren(boost::get<int>(tuple[3]));
      }

    private:
      QComboBox* _cbxStatus;
      QLineEdit* _txtDescription;
      QSpinBox* _spbNumberOfAdults;
      QSpinBox* _spbNumberOfChildren;
    };

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
      EditReservationDialog(persistence::Backend& backend, int objectId);

    private slots:
      void saveClicked();
      void mergeChangesClicked();

    private:
      void reservationsInitialized();
      void reservationsAdded(const std::vector<hotel::Reservation>& reservations);
      void reservationsUpdated(const std::vector<hotel::Reservation>& reservations);
      void reservationsRemoved(const std::vector<int>& ids);
      void allReservationsRemoved();

      void saveTaskUpdated(const std::vector<persistence::TaskResult>& results);

      void updateUI();

      enum class Status { NotInitialized, Ready, Removed, Saving };
      Status _status = Status::NotInitialized;

      persistence::Backend& _backend;

      // The reference version to which we are currently editing against
      boost::optional<hotel::Reservation> _referenceVersion;
      boost::optional<hotel::Reservation> _newestVersion;

      gui::DataStreamObserverAdapter<hotel::Reservation> _reservationStreamHandle;

      boost::signals2::scoped_connection _saveTaskUpdatedConnection;
      TaskObserverAdapter _saveTask;

      Form _form;
      StatusBar* _statusBar;

      QPushButton* _btnSave;
      QPushButton* _btnMergeChanges;
    };
  } // namespace dialogs
} // namespaec gui

#endif // GUIAPP_DIALOGS_EDITRESERVATION_H``
