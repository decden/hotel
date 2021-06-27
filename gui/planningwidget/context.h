#ifndef GUI_PLANNINGWIDGET_CONTEXT_H
#define GUI_PLANNINGWIDGET_CONTEXT_H

#include "gui/planningwidget/planningboardlayout.h"

#include "persistence/backend.h"

#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include <QtCore/QRect>
#include <QtWidgets/QGraphicsScene>

#include <boost/signals2.hpp>

#include <map>
#include <memory>
#include <string>
#include <unordered_set>

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

      // Setup
      void setDataBackend(persistence::Backend* backend);
      void setDateBarScene(QGraphicsScene* scene);
      void setRoomListScene(QGraphicsScene* scene);
      void setPlanningBoardScene(QGraphicsScene* scene);

      // Selection
      const std::unordered_set<const hotel::Reservation*> selectedReservations() const;
      void addSelectedReservation(const hotel::Reservation* reservation);
      void removeSelectedReservation(const hotel::Reservation* reservation);

      // Event handling
      boost::signals2::signal<void(const hotel::Reservation&)>& reservationDoubleClickedSignal() { return _reservationDoubleClickedSignal; }
      void emitReservationDoubleClicked(const hotel::Reservation& reservation) const { _reservationDoubleClickedSignal(reservation); }
      boost::signals2::signal<void()>& selectedReservationsChangedSignal() { return _selectedReservationsChangedSignal; }
      boost::signals2::signal<void()>& highlightedPeriodsChangedSignal() { return _highlightedPeriodsChangedSignal; }

      // Modifying data calls
      void addHotel(const hotel::Hotel& hotel);
      const hotel::Reservation* addReservation(const hotel::Reservation& reservation);
      void removeHotel(int hotelId);
      void removeReservation(int reservationId);

      // Reading data calls
      const std::vector<std::unique_ptr<hotel::Hotel>>& hotels() const;
      const hotel::PlanningBoard& planning() const;

      // Getters
      persistence::Backend& dataBackend();
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
      void setHighlightedPeriods(const std::vector<boost::gregorian::date_period>& periods);
      void initializeLayout(PlanningBoardLayout::LayoutType layoutType);

      void registerTool(const std::string& toolName, std::unique_ptr<gui::planningwidget::Tool> tool);
      void activateTool(const std::string& toolName);

    private:
      // Hotel data
      persistence::Backend* _backend = nullptr;

      // Data from streams
      std::vector<std::unique_ptr<hotel::Hotel>> _hotels;
      hotel::PlanningBoard _reservations;

      std::unordered_set<const hotel::Reservation*> _selectedReservations;

      // Events
      boost::signals2::signal<void(const hotel::Reservation&)> _reservationDoubleClickedSignal;
      boost::signals2::signal<void()> _selectedReservationsChangedSignal;
      boost::signals2::signal<void()> _highlightedPeriodsChangedSignal;

      // Object with information on how to layout the individual elements in the planning widget, such as e.g.
      // reservations and rooms.
      PlanningBoardLayout _layout;
      PlanningBoardAppearance _appearance;

      // Graphics scenes of the individual subwidgets. This information is used by the tools to add custom graphics to
      // the different scenes.
      QGraphicsScene* _dateBarScene = nullptr;
      QGraphicsScene* _roomListScene = nullptr;
      QGraphicsScene* _planningBoardScene = nullptr;

      // List of available tools and currently selected tool
      using ToolPtr = std::unique_ptr<Tool>;
      std::map<std::string, ToolPtr> _availableTools;
      Tool* _activeTool = nullptr;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_CONTEXT_H
