#ifndef GUI_PLANNINGWIDGET_RESERVATIONRENDERER_H
#define GUI_PLANNINGWIDGET_RESERVATIONRENDERER_H

#include "hotel/reservation.h"

#include <QPainter>

#include <vector>

namespace gui
{
  namespace planningwidget
  {
    class PlanningBoardLayout;

    /**
     * @brief The ReservationRenderer class is an interface for classes implementing the rendering of reservations and atoms
     *
     * The purpose of this interface is to have the possibility to have multiple visualizations for atoms and reservations.
     * One might for example wish to have a custom reservation renderer which does not display persional information for
     * when the operator is working at the front desk.
     */
    class ReservationRenderer
    {
    public:
      virtual ~ReservationRenderer() {}

      // Paint functions called from the outside
      virtual void paintAtom(QPainter* painter, const PlanningBoardLayout& layout, const hotel::ReservationAtom& atom,
                             const QRectF& atomRect, bool isSelected) const;
      virtual void paintReservationConnections(QPainter* painter, const PlanningBoardLayout& layout, const std::vector<QRectF>& atomRects, bool isSelected) const;

    protected:
      // Utility drawing functions
      virtual void drawAtomBackground(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom,
                                      const QRectF &atomRect, bool isSelected) const;
      virtual void drawAtomText(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom,
                                const QRectF &atomRect, bool isSelected) const;

      // Utility functions
      virtual QString getAtomText(const hotel::ReservationAtom& atom, bool isSelected) const;
      virtual QColor getAtomBackgroundColor(const PlanningBoardLayout& layout, const hotel::ReservationAtom& atom, bool isSelected) const;
      virtual QColor getAtomTextColor(const PlanningBoardLayout& layout, const hotel::ReservationAtom& atom, bool isSelected) const;

    };

    class PrivacyReservationRenderer : public ReservationRenderer
    {
    public:
      virtual void paintAtom(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const override;

    protected:
      virtual QColor getAtomBackgroundColor(const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, bool isSelected) const override;
      virtual void drawAtomBackground(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const override;
    };

    class HighlightArrivalsRenderer : public ReservationRenderer
    {
    protected:
      virtual QColor getAtomBackgroundColor(const PlanningBoardLayout& layout, const hotel::ReservationAtom& atom, bool isSelected) const override;
    };

  } // namespace planningwidget
} // namespace gui

#endif // GUI_PLANNINGWIDGET_RESERVATIONRENDERER_H
