#include "gui/dialogs/editreservation.h"

#include <QDate>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QMessageBox>

namespace gui
{
  namespace dialogs
  {
    EditReservationDialog::EditReservationDialog(persistence::DataSource& ds, int objectId)
        : QDialog(nullptr), _dataSource(ds)
    {
      setAttribute(Qt::WA_DeleteOnClose);
      setMinimumWidth(500);
      setWindowTitle(tr("Edit Reservation"));

      // Connect events
      _reservationStreamHandle.initializedSignal.connect(boost::bind(&EditReservationDialog::reservationsInitialized, this));
      _reservationStreamHandle.itemsAddedSignal.connect(boost::bind(&EditReservationDialog::reservationsAdded, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsUpdatedSignal.connect(boost::bind(&EditReservationDialog::reservationsUpdated, this, boost::placeholders::_1));
      _reservationStreamHandle.itemsRemovedSignal.connect(boost::bind(&EditReservationDialog::reservationsRemoved, this, boost::placeholders::_1));
      _reservationStreamHandle.allItemsRemovedSignal.connect(boost::bind(&EditReservationDialog::allReservationsRemoved, this));

      // Connect to data stream
      nlohmann::json options;
      options["id"] = objectId;
      _reservationStreamHandle.connect(ds, "reservation.by_id", options);

      auto layout = new QGridLayout();
      auto buttonsLayout = new QHBoxLayout();
      auto outerLayout = new QVBoxLayout();
      outerLayout->setMargin(0);
      layout->setMargin(10);
      buttonsLayout->setMargin(10);
      _statusBar = new StatusBar();
      _cbxStatus = new QComboBox();
      _cbxStatus->addItems({"New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"});
      _txtDescription = new QLineEdit();
      _spbNumberOfAdults = new QSpinBox();
      _spbNumberOfAdults->setRange(0, 10);
      _spbNumberOfChildren = new QSpinBox();
      _spbNumberOfChildren->setRange(0, 10);
      _btnSave = new QPushButton(tr("Save"));

      outerLayout->addWidget(_statusBar);
      outerLayout->addLayout(layout);
      outerLayout->addStretch();
      outerLayout->addLayout(buttonsLayout);

      layout->addWidget(new QLabel(tr("Status:")), 0, 0);
      layout->addWidget(new QLabel(tr("Description:")), 1, 0);
      layout->addWidget(new QLabel(tr("Number of adults:")), 2, 0);
      layout->addWidget(new QLabel(tr("Number of children:")), 3, 0);
      layout->addWidget(_cbxStatus, 0, 1);
      layout->addWidget(_txtDescription, 1, 1);
      layout->addWidget(_spbNumberOfAdults, 2, 1);
      layout->addWidget(_spbNumberOfChildren, 3, 1);

      buttonsLayout->addStretch();
      buttonsLayout->addWidget(_btnSave);

      setLayout(outerLayout);

      // Connect events
      connect(_btnSave, SIGNAL(clicked(bool)), this, SLOT(saveClicked()));

      updateUI();
    }

    void EditReservationDialog::saveClicked()
    {
      assert(_status == Status::Ready);
      assert(_referenceVersion != boost::none);

      _status = Status::Saving;

      auto updatedReservation = std::make_unique<hotel::Reservation>(*_referenceVersion);
      updatedReservation->setStatus(static_cast<hotel::Reservation::ReservationStatus>(_cbxStatus->currentIndex() + 2));
      updatedReservation->setDescription(_txtDescription->text().toStdString());
      updatedReservation->setNumberOfAdults(_spbNumberOfAdults->value());
      updatedReservation->setNumberOfChildren(_spbNumberOfChildren->value());

      _saveTask = _dataSource.queueOperation(persistence::op::UpdateReservation{std::move(updatedReservation)});
      _saveTaskUpdatedConnection = _saveTask->connectToChangedSignal(boost::bind(&EditReservationDialog::saveTaskUpdated, this));

      updateUI();
    }

    void EditReservationDialog::reservationsInitialized()
    {
      assert(_status == Status::NotInitialized);
      _status = Status::Ready;

      updateUI();
    }

    void EditReservationDialog::reservationsAdded(const std::vector<hotel::Reservation> &reservations)
    {      
      assert(_status == Status::NotInitialized);
      assert(reservations.size() == 1);
      assert(_referenceVersion == boost::none);

      _referenceVersion = reservations[0];
      _cbxStatus->setCurrentIndex(_referenceVersion->status() - 2);
      _txtDescription->setText(QString::fromStdString(_referenceVersion->description()));
      _spbNumberOfAdults->setValue(_referenceVersion->numberOfAdults());
      _spbNumberOfChildren->setValue(_referenceVersion->numberOfChildren());
    }

    void EditReservationDialog::reservationsUpdated(const std::vector<hotel::Reservation> &reservations)
    {
      assert(reservations.size() == 1);
      assert(_referenceVersion != boost::none);

      auto newVersion = reservations[0];
      assert(newVersion.revision() > _referenceVersion->revision());
      _newerVersions.push_back(std::move(newVersion));

      updateUI();
    }

    void EditReservationDialog::reservationsRemoved(const std::vector<int> &ids)
    {
      assert(ids.size() == 1);
      assert(_referenceVersion != boost::none);
      assert(_referenceVersion->id() == ids[0]);
      _referenceVersion = boost::none;
      _status = Status::Removed;
      updateUI();
    }

    void EditReservationDialog::allReservationsRemoved()
    {
      _referenceVersion = boost::none;
      _status = Status::Removed;
      updateUI();
    }

    void EditReservationDialog::saveTaskUpdated()
    {
      if (_saveTask != boost::none && _saveTask->completed())
      {
        auto result = _saveTask->results()[0];
        if (result.status == persistence::op::OperationResultStatus::Successful)
          close();
        else
        {
          _status = Status::Ready;
          updateUI();
          _statusBar->showMessage(tr("Saving was unsuccessful (%1)").arg(QString::fromStdString(result.message)), StatusBar::Error);
        }
      }
    }

    void EditReservationDialog::updateUI()
    {
      setEnabled(_status == Status::Ready && _referenceVersion != boost::none);
      _btnSave->setEnabled(_status == Status::Ready && _newerVersions.empty());

      if (_status == Status::Ready && !_newerVersions.empty())
      {
        _statusBar->showMessage(tr("Newer versions of this item are available"), gui::StatusBar::Error);
      }
      else if (_status == Status::Ready && _referenceVersion != boost::none)
      {
        auto description = QString::fromStdString(_referenceVersion->description());
        auto fromDate = _referenceVersion->dateRange().begin();
        auto toDate = _referenceVersion->dateRange().end();
        auto fromDateText = QDate(fromDate.year(), fromDate.month(), fromDate.day()).toString("dd-MM-yyyy");
        auto toDateText = QDate(toDate.year(), toDate.month(), toDate.day()).toString("dd-MM-yyyy");
        _statusBar->showMessage(tr("%1 - From %2 to %3").arg(description, fromDateText, toDateText), gui::StatusBar::Success);
      }
      else if (_status == Status::Ready && _referenceVersion == boost::none)
        _statusBar->showMessage(tr("Could not load data"), gui::StatusBar::Error);
      else if (_status == Status::NotInitialized)
        _statusBar->showMessage(tr("Loading..."), gui::StatusBar::Info);
      else if (_status == Status::Removed)
        _statusBar->showMessage(tr("The reservation was deleted while you were editing it"), gui::StatusBar::Error);
      else if (_status == Status::Saving)
        _statusBar->showMessage(tr("Saving..."), gui::StatusBar::Info);
    }
  } // namespace dialogs
} // namespace gui
