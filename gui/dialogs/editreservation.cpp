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
    template <class T>
    class Resolver {
    public:
      struct Resolution {
        T merged;
        std::vector<int> conflicts;
      };

      static Resolution merge(const T &base, const T &ours, const T &theirs)
      {
        Resolution resolution;
        resolution.merged = base;
        for (int i = 0; i < static_cast<int>(base.size()); ++i)
        {
          bool oursChanged = base[i] != ours[i];
          bool theirsChanged = base[i] != theirs[i];
          if (ours[i] == theirs[i])
            resolution.merged[i] = ours[i];
          else if (oursChanged && !theirsChanged)
            resolution.merged[i] = ours[i];
          else if (!oursChanged && theirsChanged)
            resolution.merged[i] = theirs[i];
          else if (oursChanged && theirsChanged)
            resolution.conflicts.push_back(i);
        }
        return resolution;
      }
    };

    template <class T>
    std::string valueToStr(const T& val)
    {
      return std::to_string(val);
    }
    template <>
    std::string valueToStr(const std::string& val)
    {
      return val;
    }
    template <>
    std::string valueToStr(const hotel::Reservation::ReservationStatus& val)
    {
      const char* names[] = {"Unknown", "Temporary", "New", "Confirmed", "CheckedIn", "CheckedOut", "Archived"};
      return names[static_cast<int>(val)];
    }


    EditReservationDialog::EditReservationDialog(persistence::Backend &backend, int objectId)
        : QDialog(nullptr), _backend(backend)
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

      _saveTask.resultsSetSignal.connect(boost::bind(&EditReservationDialog::saveTaskUpdated, this, boost::placeholders::_1));

      // Connect to data stream
      nlohmann::json options;
      options["id"] = objectId;
      _reservationStreamHandle.connect(backend, "reservation.by_id", options);

      auto layout = new QGridLayout();
      auto buttonsLayout = new QHBoxLayout();
      auto outerLayout = new QVBoxLayout();
      outerLayout->setMargin(0);
      layout->setMargin(10);
      buttonsLayout->setMargin(10);
      _statusBar = new StatusBar();
      _btnSave = new QPushButton(tr("Save"));
      _btnMergeChanges = new QPushButton(tr("Resolve conflicts"));

      outerLayout->addWidget(_statusBar);
      outerLayout->addLayout(layout);
      outerLayout->addStretch();
      outerLayout->addLayout(buttonsLayout);

      int row = 0;
      for (auto& field : _form.getWidgets())
      {
        layout->addWidget(new QLabel(tr(field.first)), row, 0);
        layout->addWidget(field.second, row, 1);
        row++;
      }

      buttonsLayout->addStretch();
      buttonsLayout->addWidget(_btnMergeChanges);
      buttonsLayout->addWidget(_btnSave);

      setLayout(outerLayout);

      // Connect events
      connect(_btnSave, SIGNAL(clicked(bool)), this, SLOT(saveClicked()));
      connect(_btnMergeChanges, SIGNAL(clicked(bool)), this, SLOT(mergeChangesClicked()));

      updateUI();
    }

    void EditReservationDialog::saveClicked()
    {
      assert(_status == Status::Ready);
      assert(_referenceVersion != boost::none);

      _status = Status::Saving;

      auto updatedReservation = std::make_unique<hotel::Reservation>(*_referenceVersion);
      Form::SetItemFromTuple(*updatedReservation, _form.formValues());
      _saveTask.connect(_backend, persistence::op::UpdateReservation{std::move(updatedReservation)});

      updateUI();
    }

    void EditReservationDialog::mergeChangesClicked()
    {
      if (_newestVersion != boost::none)
      {
        auto base = Form::ItemToTuple(*_referenceVersion);
        auto ours = _form.formValues();
        auto theirs = Form::ItemToTuple(*_newestVersion);

        auto resolution = Resolver<Form::Tuple>::merge(base, ours, theirs);
        if (!resolution.conflicts.empty())
        {
          for (auto conflict : resolution.conflicts)
          {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Merge conflict"));
            auto baseStr = boost::apply_visitor([](auto& value){ return valueToStr(value); }, base[conflict]);
            auto ourStr = boost::apply_visitor([](auto& value){ return valueToStr(value); }, ours[conflict]);
            auto theirStr = boost::apply_visitor([](auto& value){ return valueToStr(value); }, theirs[conflict]);

            msgBox.setText(tr("Failed to automatically merge a field. Please choose which value to keep:\n\n%1\nBase version: \t%2\nOur version: \t%3\nTheir version: \t%4")
                           .arg(_form.getWidgets()[conflict].first)
                           .arg(QString::fromStdString(baseStr))
                           .arg(QString::fromStdString(ourStr))
                           .arg(QString::fromStdString(theirStr)));
            msgBox.addButton(tr("Base version"), QMessageBox::RejectRole);
            auto ourVersionButton = msgBox.addButton(tr("Our version"), QMessageBox::AcceptRole);
            auto theirVersionButton = msgBox.addButton(tr("Their version"), QMessageBox::RejectRole);
            msgBox.exec();
            if (msgBox.clickedButton() == ourVersionButton)
              resolution.merged[conflict] = ours[conflict];
            if (msgBox.clickedButton() == theirVersionButton)
              resolution.merged[conflict] = theirs[conflict];
          }
        }

        _form.setFormValues(resolution.merged);
      }
      _referenceVersion = _newestVersion;
      _newestVersion = boost::none;
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
      _form.setFormValues(Form::ItemToTuple(*_referenceVersion));
    }

    void EditReservationDialog::reservationsUpdated(const std::vector<hotel::Reservation> &reservations)
    {
      assert(reservations.size() == 1);
      assert(_referenceVersion != boost::none);

      auto newVersion = reservations[0];
      assert(newVersion.revision() > _referenceVersion->revision());
      _newestVersion = newVersion;

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

    void EditReservationDialog::saveTaskUpdated(const std::vector<persistence::TaskResult> &results)
    {
      auto result = results[0];
      if (result.status == persistence::TaskResultStatus::Successful)
        close();
      else
      {
        _status = Status::Ready;
        updateUI();
        _statusBar->showMessage(tr("Saving was unsuccessful (%1)").arg(QString::fromStdString(result.result["message"])), StatusBar::Error);
      }
    }

    void EditReservationDialog::updateUI()
    {
      setEnabled(_status == Status::Ready && _referenceVersion != boost::none);
      _btnSave->setEnabled(_status == Status::Ready && _newestVersion == boost::none);
      _btnMergeChanges->setVisible(_status == Status::Ready && _newestVersion != boost::none);

      if (_status == Status::Ready && _newestVersion != boost::none)
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
