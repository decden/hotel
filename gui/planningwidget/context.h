#ifndef GUI_PLANNINGWIDGET_CONTEXT_H
#define GUI_PLANNINGWIDGET_CONTEXT_H

#include "gui/planningwidget/planningboardlayout.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include <QGraphicsScene>
#include <QRect>

#include <map>
#include <memory>
#include <string>

namespace gui
{
  namespace planningwidget
  {
    class Tool;

    /**
     * @brief The Context class stores shared state information between all of the planning widget subwidgets.
     */
    class Context
    {
    public:
      Context();
      virtual ~Context() = default;

      void setHotelCollection(hotel::HotelCollection* hotelCollection);
      void setDateBarScene(QGraphicsScene* scene);
      void setRoomListScene(QGraphicsScene* scene);
      void setPlanningBoardScene(QGraphicsScene* scene);

      hotel::HotelCollection* hotelCollection();
      const hotel::HotelCollection* hotelCollection() const;
      PlanningBoardLayout& layout();
      const PlanningBoardLayout& layout() const;
      PlanningBoardAppearance& appearance();
      const PlanningBoardAppearance& appearance() const;

      QGraphicsScene* dateBarScene();
      QGraphicsScene* roomListScene();
      QGraphicsScene* planningBoardScene();

      Tool* activeTool() { return _activeTool; }
      const Tool* activeTool() const { return _activeTool; }

      void setPivotDate(const boost::gregorian::date date);
      void initializeLayout(PlanningBoardLayout::LayoutType layoutType);

      void registerTool(const std::string& toolName, std::unique_ptr<gui::planningwidget::Tool> tool);
      void activateTool(const std::string& toolName);

    private:
      // Hotel data
      hotel::HotelCollection* _hotelCollection;

      // Object with information on how to layout the individual elements in the planning widget, such as e.g.
      // reservations and rooms.
      PlanningBoardLayout _layout;
      PlanningBoardAppearance _appearance;

      // Graphics scenes of the individual subwidgets. This information is used by the tools to add custom graphics to
      // the different scenes.
      QGraphicsScene* _dateBarScene;
      QGraphicsScene* _roomListScene;
      QGraphicsScene* _planningBoardScene;

      // List of available tools and currently selected tool
      using ToolPtr = std::unique_ptr<Tool>;
      std::map<std::string, ToolPtr> _availableTools;
      Tool* _activeTool = nullptr;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_CONTEXT_H
