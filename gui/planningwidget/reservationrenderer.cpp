#include "gui/planningwidget/reservationrenderer.h"

#include "gui/planningwidget/planningboardlayout.h"

namespace gui
{
  namespace planningwidget
  {

  QColor mix(const QColor& c1, const QColor &c2)
  {
    return QColor(
      (c1.red() + c2.red() * 3) / 4,
      (c1.green() + c2.green() * 3) / 4,
      (c1.blue() + c2.blue() * 3) / 4,
      (c1.alpha() + c2.alpha() * 3) / 4
    );
  }

  void ReservationRenderer::paintAtom(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const
  {
    drawAtomBackground(painter, layout, atom, atomRect, isSelected);
    drawAtomText(painter, layout, atom, atomRect, isSelected);
  }

  void ReservationRenderer::paintReservationConnections(QPainter *painter, const PlanningBoardLayout &layout, const std::vector<QRectF> &atomRects, bool isSelected) const
  {
    if (isSelected)
    {
      // Draw the connection links between items
      auto& appearance = layout.appearance();
      if (atomRects.size() > 1)
      {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(appearance.selectionColor, 2));

        for (auto i = 0; i < static_cast<int>(atomRects.size()) - 1; ++i)
        {
          auto previousBox = atomRects[i];
          auto nextBox = atomRects[i + 1];

          // Calculate the two points
          auto x1 = previousBox.right();
          auto y1 = previousBox.top() + previousBox.height() / 2;
          auto x2 = nextBox.left();
          auto y2 = nextBox.top() + nextBox.height() / 2;

          // Draw the two rectangular handles
          const int handleSize = appearance.atomConnectionHandleSize;
          const int linkOverhang = appearance.atomConnectionOverhang;
          const QColor& handleColor = appearance.selectionColor;
          painter->fillRect(QRect(x1 - handleSize, y1 - handleSize, handleSize * 2, handleSize * 2), handleColor);
          painter->fillRect(QRect(x2 - handleSize, y2 - handleSize, handleSize * 2, handleSize * 2), handleColor);

          // Draw the zig-yag line between handles
          QPointF points[] = {QPoint(x1, y1), QPoint(x1 + linkOverhang, y1), QPoint(x2 - linkOverhang, y2),
                              QPoint(x2, y2)};
          painter->drawPolyline(points, 4);
        }

        painter->restore();
      }
    }
  }

  void ReservationRenderer::drawAtomBackground(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const
  {
    auto itemColor = getAtomBackgroundColor(layout, atom, isSelected);
    const int cornerRadius = 5;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(itemColor);
    painter->setPen(itemColor.darker(200));

    // Construct a rounded rectangle. Not all of the corners are rounded. Only the parts corresponding to the
    // beginning
    // or end of a reservation are rounded
    auto borderRect = atomRect.adjusted(1, 1, 0, -1).adjusted(-0.5, -0.5, -0.5, -0.5);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRoundedRect(borderRect, cornerRadius, cornerRadius, Qt::AbsoluteSize);
    if (!atom.isFirst())
      path.addRect(borderRect.adjusted(0, 0, -cornerRadius, 0));
    if (!atom.isLast())
      path.addRect(borderRect.adjusted(cornerRadius, 0, 0, 0));
    painter->drawPath(path.simplified());
    painter->restore();
  }

  void ReservationRenderer::drawAtomText(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const
    {
      painter->save();
      painter->setClipRect(atomRect);
      painter->setPen(getAtomTextColor(layout, atom, isSelected));
      painter->setFont(layout.appearance().atomTextFont);
      painter->drawText(atomRect.adjusted(5, 2, -2, -2), Qt::AlignLeft | Qt::AlignVCenter, getAtomText(atom, isSelected));
      painter->restore();
    }

    QColor ReservationRenderer::getAtomBackgroundColor(const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, bool isSelected) const
    {
      using ReservationStatus = hotel::Reservation::ReservationStatus;

      auto& appearance = layout.appearance();
      if (isSelected)
      {
        if (atom.reservation()->status() == ReservationStatus::CheckedOut ||
            atom.reservation()->status() == ReservationStatus::Archived)
          return appearance.atomArchivedSelectedColor;
        else
          return appearance.atomSelectedColor;
      }
      else
      {
        if (atom.reservation()->status() == ReservationStatus::Archived)
          return appearance.atomArchivedColor;

        if (atom.reservation()->status() == ReservationStatus::CheckedOut)
          return appearance.atomCheckedOutColor;

        if (atom.reservation()->status() == ReservationStatus::CheckedIn)
          return appearance.atomCheckedInColor;

        if (atom.reservation()->status() == ReservationStatus::New)
          return appearance.atomUnconfirmedColor;

        return appearance.atomDefaultColor;
      }
    }

    QColor ReservationRenderer::getAtomTextColor(const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, bool isSelected) const
    {
      auto& appearance = layout.appearance();
      auto itemColor = getAtomBackgroundColor(layout, atom, isSelected);
      if (itemColor.lightness() > 200)
        return appearance.atomDarkTextColor;
      else
        return appearance.atomLightTextColor;
    }

    QString ReservationRenderer::getAtomText(const hotel::ReservationAtom &atom, bool isSelected) const
    {
      auto description = QString::fromStdString(atom.reservation()->description());
      
      auto text = QString("%1+%2 %3 (%4)").arg(atom.reservation()->numberOfAdults()).arg(atom.reservation()->numberOfChildren())
                                          .arg(description).arg(atom.reservation()->length());

      if (!atom.isFirst())
        text = "\xE2\x96\xB8 " + text;
      return text;
    }

    void PrivacyReservationRenderer::paintAtom(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const
    {
      drawAtomBackground(painter, layout, atom, atomRect, isSelected);
      if (isSelected)
        drawAtomText(painter, layout, atom, atomRect, isSelected);
    }

    QColor PrivacyReservationRenderer::getAtomBackgroundColor(const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, bool isSelected) const
    {
      auto& appearance = layout.appearance();
      return isSelected ? appearance.atomSelectedColor : appearance.atomDefaultColor;
    }

    void PrivacyReservationRenderer::drawAtomBackground(QPainter *painter, const PlanningBoardLayout &layout, const hotel::ReservationAtom &atom, const QRectF &atomRect, bool isSelected) const
    {
      QColor backgroundColor = getAtomBackgroundColor(layout, atom, isSelected);
      painter->fillRect(atomRect.adjusted(0, 0, 0, -1), backgroundColor);
    }

    QColor HighlightArrivalsRenderer::getAtomBackgroundColor(const PlanningBoardLayout& layout, const hotel::ReservationAtom& atom, bool isSelected) const
    {
      auto color = ReservationRenderer::getAtomBackgroundColor(layout, atom, isSelected);
      color = mix(color, layout.appearance().atomDefaultColor);

      bool isHighlighted = atom.dateRange().begin() == layout.pivotDate();
      if (isHighlighted)
        color = color.lighter(120);
      if (isSelected)
        color = layout.appearance().atomSelectedColor;

      return color;
    }

} // namespace planningwidget
} // namespace gui
