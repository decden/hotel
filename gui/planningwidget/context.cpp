#include "gui/planningwidget/context.h"

#include "gui/planningwidget/tool.h"

namespace gui
{
  namespace planningwidget
  {
    Context::Context() = default;

    void Context::setDataBackend(persistence::Backend* backend) { _backend = backend; }
    void Context::setDateBarScene(QGraphicsScene* scene) { _dateBarScene = scene; }
    void Context::setRoomListScene(QGraphicsScene* scene) { _roomListScene = scene; }
    void Context::setPlanningBoardScene(QGraphicsScene* scene) { _planningBoardScene = scene; }

    const std::unordered_set<const hotel::Reservation*> Context::selectedReservations() const
    {
      return _selectedReservations;
    }
    void Context::addSelectedReservation(const hotel::Reservation* reservation)
    {
      _selectedReservations.insert(reservation);
      _selectedReservationsChangedSignal();
    }
    void Context::removeSelectedReservation(const hotel::Reservation* reservation)
    {
      _selectedReservations.erase(reservation);
      _selectedReservationsChangedSignal();
    }

    void Context::addHotel(const hotel::Hotel& hotel)
    {
      assert(hotel.id() != 0);
      _hotels.push_back(std::make_unique<hotel::Hotel>(hotel));
    }

    const hotel::Reservation* Context::addReservation(const hotel::Reservation& reservation)
    {
      assert(reservation.id() != 0);
      if (_activeTool)
        _activeTool->reservationAdded(reservation);

      return _reservations.addReservation(std::make_unique<hotel::Reservation>(reservation));
    }

    void Context::removeHotel(int hotelId)
    {
      _hotels.erase(std::remove_if(_hotels.begin(), _hotels.end(), [=](auto& hotel) { return hotel->id() == hotelId; }),
                    _hotels.end());
    }

    void Context::removeReservation(int reservationId)
    {
      if (_activeTool)
        _activeTool->reservationRemoved(reservationId);
      _reservations.removeReservation(reservationId);
    }

    const std::vector<std::unique_ptr<hotel::Hotel>>& Context::hotels() const { return _hotels; }

    persistence::Backend& Context::dataBackend() { return *_backend; }
    void Context::setPivotDate(const boost::gregorian::date date) { _layout.setPivotDate(date); }
    void Context::setHighlightedPeriods(const std::vector<boost::gregorian::date_period>& periods)
    {
      _layout.setHighlightedPeriods(periods);
      _highlightedPeriodsChangedSignal();
    }

    void Context::initializeLayout(PlanningBoardLayout::LayoutType layoutType)
    {
      assert(_backend != nullptr);
      if (_backend)
        _layout.initializeLayout(hotels(), layoutType);
    }

    const hotel::PlanningBoard& Context::planning() const { return _reservations; }
    PlanningBoardLayout& Context::layout() { return _layout; }
    const PlanningBoardLayout& Context::layout() const { return _layout; }
    PlanningBoardAppearance& Context::appearance() { return _appearance; }
    const PlanningBoardAppearance& Context::appearance() const { return _appearance; }
    QGraphicsScene* Context::dateBarScene() { return _dateBarScene; }
    QGraphicsScene* Context::roomListScene() { return _roomListScene; }
    QGraphicsScene* Context::planningBoardScene() { return _planningBoardScene; }

    void Context::registerTool(const std::string& toolName, std::unique_ptr<Tool> tool)
    {
      if (toolName.empty())
      {
        std::cerr << "registerTool(): cannot register tool with empty name." << std::endl;
        return;
      }

      if (_availableTools.find(toolName) != _availableTools.end())
      {
        std::cerr << "registerTool(): a tool with the name " << toolName << " has already been registered."
                  << std::endl;
        return;
      }

      tool->init(*this);
      _availableTools[toolName] = std::move(tool);
    }

    void Context::activateTool(const std::string& toolName)
    {
      auto it = _availableTools.find(toolName);

      gui::planningwidget::Tool* newTool = nullptr;
      if (it != _availableTools.end())
        newTool = it->second.get();
      else if (!toolName.empty())
        std::cerr << "activateTool(): the tool " << toolName << " has not been registered." << std::endl;

      if (_activeTool != newTool)
      {
        if (_activeTool)
          _activeTool->unload();
        _activeTool = newTool;
        if (_activeTool)
          _activeTool->load();
      }
    }

  } // namespace planningwidget
} // namespace gui
