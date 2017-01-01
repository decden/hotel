#include "gui/planningwidget/context.h"

#include "gui/planningwidget/tool.h"

namespace gui
{
  namespace planningwidget
  {

    Context::Context() {}

    void Context::setHotelCollection(hotel::HotelCollection* hotelCollection) { _hotelCollection = hotelCollection; }
    void Context::setDateBarScene(QGraphicsScene* scene) { _dateBarScene = scene; }
    void Context::setRoomListScene(QGraphicsScene* scene) { _roomListScene = scene; }
    void Context::setPlanningBoardScene(QGraphicsScene* scene) { _planningBoardScene = scene; }
    void Context::setPlanning(const hotel::PlanningBoard* planning) { _planning = planning; }
    void Context::setPivotDate(const boost::gregorian::date date) { _layout.setPivotDate(date); }

    void Context::initializeLayout(PlanningBoardLayout::LayoutType layoutType)
    {
      assert(_hotelCollection != nullptr);
      if (_hotelCollection)
        _layout.initializeLayout(*_hotelCollection, layoutType);
    }

    hotel::HotelCollection* Context::hotelCollection() { return _hotelCollection; }
    const hotel::HotelCollection *Context::hotelCollection() const { return _hotelCollection; }
    const hotel::PlanningBoard *Context::planning() const { return _planning; }
    PlanningBoardLayout& Context::layout() { return _layout; }
    const PlanningBoardLayout& Context::layout() const { return _layout; }
    PlanningBoardAppearance& Context::appearance() { return _appearance; }
    const PlanningBoardAppearance& Context::appearance() const { return _appearance; }
    QGraphicsScene *Context::dateBarScene() { return _dateBarScene; }
    QGraphicsScene *Context::roomListScene() { return _roomListScene; }
    QGraphicsScene *Context::planningBoardScene() { return _planningBoardScene; }

    void Context::registerTool(const std::string& toolName, std::unique_ptr<Tool> tool)
    {
      if (toolName == "")
      {
        std::cerr << "registerTool(): cannot register tool with empty name." << std::endl;
        return;
      }
      else if (_availableTools.find(toolName) != _availableTools.end())
      {
        std::cerr << "registerTool(): a tool with the name " << toolName << " has already been registered."
                  << std::endl;
        return;
      }
      else
      {
        tool->init(*this);
        _availableTools[toolName] = std::move(tool);
      }
    }

    void Context::activateTool(const std::string& toolName)
    {
      auto it = _availableTools.find(toolName);

      gui::planningwidget::Tool* newTool = nullptr;
      if (it != _availableTools.end())
        newTool = it->second.get();
      else if (toolName != "")
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
