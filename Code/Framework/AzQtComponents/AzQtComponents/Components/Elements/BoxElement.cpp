/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>
#include <QTabBar>
#include <QToolButton>
#include <QWidget>
#include <qdrawutil.h>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace BoxElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        constexpr int ArrowSize = 16;                   // ComboBox.qss:66-67 $size.arrow.idle.width $size.arrow.idle.height
        constexpr int ArrowMarginRight = 4;             // ComboBox.qss:68 $space.arrow.idle
        // QStyleSheetStyle::drawControl (CE_PushButtonLabel), as QPushButton::sizeHint has it $space.button.idle.icon-spacing
        constexpr int PushButtonIconSpacing = 4;
        // QStyleSheetStyle::defaultSize (a tool button's ::menu-indicator) $size.menu-indicator.idle
        constexpr QSize MenuIndicatorSize(13, 13);
        // QStyleSheetStyle::sizeFromContents (CT_ComboBox: what the base style leaves in the label) $space.field.idle.label
        constexpr int ComboLabelExtra = 2;
        // QStyleSheetStyle::drawControl (CE_ComboBoxLabel, a rule with no spacing) $space.field.idle.icon-spacing
        constexpr int ComboIconSpacing = 6;

        // ------------------------------------------------------------------------------
        // Helpers: resolution
        // ------------------------------------------------------------------------------

        //! The dynamic property that says the element took the autofill of the widget's background
        constexpr const char* AutoFillProperty = "_o3de_boxElementAutoFill";

        ElementRegistry<BoxElement>& registry()
        {
            static ElementRegistry<BoxElement> rows("_o3de_boxElement");
            return rows;
        }

        // ------------------------------------------------------------------------------
        // Helpers: the state of an option (QStyleSheetStyle::renderRule)
        // ------------------------------------------------------------------------------

        //! The States the rule is computed for (pseudoClass, qstylesheetstyle.cpp): :hover needs :enabled, :pressed is Sunken, and no
        //! option is Disabled alone (QStyleSheetStyle::renderRule). A line edit's Sunken is its frame, no press: the sheet leaves it out.
        unsigned statesOf(const QStyleOption* option)
        {
            QStyle::State state = option ? option->state : QStyle::State(QStyle::State_None);
            if (option && qobject_cast<const QLineEdit*>(option->styleObject))
            {
                state &= ~QStyle::State_Sunken;
            }
            unsigned states = 0;
            if (!state.testFlag(QStyle::State_Enabled))
            {
                states |= BoxElement::Disabled;
            }
            else if (state.testFlag(QStyle::State_MouseOver))
            {
                states |= BoxElement::Hover;
            }
            states |= state.testFlag(QStyle::State_Sunken) ? BoxElement::Pressed : 0;
            states |= state.testFlag(QStyle::State_HasFocus) ? BoxElement::Focus : 0;
            states |= state.testFlag(QStyle::State_On) ? BoxElement::Checked : 0;
            return states;
        }

        bool hasBorderImage(const BoxElement::Face& face)
        {
            return face.m_borderImage[0] != 0;
        }

        // ------------------------------------------------------------------------------
        // Helpers: the box in numbers (QRenderRule)
        // ------------------------------------------------------------------------------

        //! The margin, the radius and the spacing of a face: its own, else the element's
        QMargins marginOf(const BoxElement& element, const BoxElement::Face& face)
        {
            return face.m_margin.value_or(element.m_margin);
        }

        int radiusOf(const BoxElement& element, const BoxElement::Face& face)
        {
            return face.m_radius != BoxElement::Unset ? face.m_radius : element.m_radius;
        }

        int spacingOf(const BoxElement& element, const BoxElement::Face& face)
        {
            return face.m_spacing != BoxElement::Unset ? face.m_spacing : element.m_spacing;
        }

        //! QRenderRule::hasBox() of a face's rule: a margin or a padding, even a 0, or a spacing (ValueExtractor::extractBox)
        bool hasBox(const BoxElement& element, const BoxElement::Face& face)
        {
            return element.m_hasBox || !element.m_margin.isNull() || !element.m_padding.isNull() || element.m_spacing != BoxElement::Unset
                || face.m_margin.has_value() || face.m_spacing != BoxElement::Unset;
        }

        //! The same for the widget's rule in every state (PseudoClass_Any, QStyleSheetStyle::polish): a box in any state gives it one
        bool hasBoxInAnyState(const BoxElement& element)
        {
            bool result = hasBox(element, element.m_idle);
            for (const BoxElement::StateFace& stateFace : element.m_stateFaces)
            {
                result = result || (stateFace.m_states != 0 && hasBox(element, stateFace.m_face));
            }
            return result;
        }

        //! The states with no native border once drawn and sized: a flat push button (QPushButton:flat { border: none },
        //! PushButton.qss:48-52) and a line edit without a frame (QLineEdit:no-frame { border: none }, Qt's default sheet), unless its
        //! container's rule draws it. The rule of the polish knows neither state, so the limits count the native border (applyLimits).
        bool isFlat(const QStyleOption* option)
        {
            const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option);
            if (button && button->features.testFlag(QStyleOptionButton::Flat))
            {
                return true;
            }
            const auto* lineEdit = option ? qobject_cast<const QLineEdit*>(option->styleObject) : nullptr;
            return lineEdit && !lineEdit->hasFrame() && BoxElement::containerOf(lineEdit) == lineEdit;
        }

        //! QStyleSheetStyle::nativeFrameWidth for the classes Qt's default sheet gives a native border: a line edit (none once drawn
        //! without a frame), a QFrame (none without a frame), push and tool buttons and a combo box have the base style's frame width.
        //! Any other class has a border only where a rule declares one.
        int nativeWidth(const QWidget* widget)
        {
            if (qobject_cast<const QComboBox*>(widget))
            {
                return widget->style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth, nullptr, widget);
            }
            if (qobject_cast<const QLineEdit*>(widget))
            {
                return widget->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, widget);
            }
            const auto* frame = qobject_cast<const QFrame*>(widget);
            const bool button = qobject_cast<const QPushButton*>(widget) || qobject_cast<const QToolButton*>(widget);
            if (frame ? frame->frameShape() == QFrame::NoFrame : !button)
            {
                return 0;
            }
            return widget->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, widget);
        }

        //! The border widths of a face (QRenderRule::fixupBorder): the declared edges, the style's own width for the others
        QMargins borderOf(const BoxElement& element, const BoxElement::Face& face, bool flat, const QWidget* widget)
        {
            // a border image has no style's width: fixupBorder leaves the declared widths alone
            const int native = (element.m_nativeBorder && !flat && !hasBorderImage(face)) ? nativeWidth(widget) : 0;
            const auto width = [native](const BoxElement::Edge& edge)
            {
                return edge.m_color.isValid() ? edge.m_width : native;
            };
            return QMargins(
                width(face.m_edges.m_left), width(face.m_edges.m_top), width(face.m_edges.m_right), width(face.m_edges.m_bottom));
        }

        //! Margin, border and padding in one, of a face's box: the contents sit inside it and their size grows by it. A rect and a size
        //! are the box of the option's state; the limits a widget gets at polish are those of the idle face, the state it was polished
        //! in (QStyleSheetStyle::setGeometry reads PseudoClass_Enabled).
        QMargins around(const BoxElement& element, const BoxElement::Face& face, bool flat, const QWidget* widget)
        {
            return marginOf(element, face) + borderOf(element, face, flat, widget) + element.m_padding;
        }

        //! The width of the drop-down's box (QRenderRule::boxSize): its contents' width and its padding. With no width its rule has no
        //! size, and Qt makes it DropDownWidth wide (QStyleSheetStyle::defaultSize).
        int dropDownBoxWidth(const BoxElement& element)
        {
            return element.m_dropDownWidth != BoxElement::Unset
                ? element.m_dropDownWidth + element.m_dropDownPadding.left() + element.m_dropDownPadding.right()
                : BoxElement::DropDownWidth;
        }

        //! QRenderRule::hasDrawable() in any state: a border of its own, a background (a transparent fill is one) or an image
        bool drawsInAnyState(const BoxElement& element)
        {
            const auto draws = [](const BoxElement::Face& face)
            {
                return face.m_fill.style() != Qt::NoBrush || face.m_image[0] != 0 || hasBorderImage(face);
            };
            bool result = !element.m_nativeBorder || draws(element.m_idle);
            for (const BoxElement::StateFace& stateFace : element.m_stateFaces)
            {
                result = result || (stateFace.m_states != 0 && draws(stateFace.m_face));
            }
            return result;
        }

        // ------------------------------------------------------------------------------
        // Helpers: polish (QStyleSheetStyle::polish)
        // ------------------------------------------------------------------------------

        //! The widget a rule's background and hover reach: a scroll area's viewport (embeddedWidget, qstylesheetstyle.cpp)
        QWidget* embeddedWidget(QWidget* widget)
        {
            auto* area = qobject_cast<QAbstractScrollArea*>(widget);
            return area ? area->viewport() : widget;
        }

        bool setsLimits(const BoxElement* element)
        {
            return element
                && (element->m_minSize.width() >= 0 || element->m_minSize.height() >= 0
                    || element->m_maxSize.width() >= 0 || element->m_maxSize.height() >= 0);
        }

        //! One limit of QStyleSheetStyle::setGeometry: the contents' limit grown by the box (a declared width or height raises a minimum
        //! and lowers a maximum). A limit of the previous element this one does not declare is lifted, as the sheet lifted its own.
        void setLimit(int& limit, int before, int declared, int size, int grow, bool minimum)
        {
            if (declared >= 0)
            {
                limit = grow + (minimum ? qMax(size, declared) : qMin(size >= 0 ? size : QWIDGETSIZE_MAX, declared));
            }
            else if (before >= 0)
            {
                limit = minimum ? 0 : QWIDGETSIZE_MAX;
            }
        }

        //! The sheet's geometry came after the base style's and the components' polish (PushButton::polish gives every push button a
        //! maximum height, which the element's max-height replaces), in the Enabled state alone: a button's flat state is not part of
        //! the box it grows by.
        void applyLimits(QWidget* widget, const BoxElement* previous, const BoxElement* element)
        {
            if (!setsLimits(previous) && !setsLimits(element))
            {
                return;
            }
            static const BoxElement none{};
            const BoxElement& before = previous ? *previous : none;
            const BoxElement& after = element ? *element : none;
            const QMargins grow = element ? around(*element, element->m_idle, false, widget) : QMargins();
            QSize minimum = widget->minimumSize();
            QSize maximum = widget->maximumSize();
            setLimit(minimum.rwidth(), before.m_minSize.width(), after.m_minSize.width(), after.m_size.width(),
                grow.left() + grow.right(), true);
            setLimit(minimum.rheight(), before.m_minSize.height(), after.m_minSize.height(), after.m_size.height(),
                grow.top() + grow.bottom(), true);
            setLimit(maximum.rwidth(), before.m_maxSize.width(), after.m_maxSize.width(), after.m_size.width(),
                grow.left() + grow.right(), false);
            setLimit(maximum.rheight(), before.m_maxSize.height(), after.m_maxSize.height(), after.m_size.height(),
                grow.top() + grow.bottom(), false);
            widget->setMinimumSize(minimum);
            widget->setMaximumSize(maximum);
        }

        //! QStyleSheetStyle::setProperties: the value takes the type of the property it sets
        void applyProperties(QWidget* widget, const BoxElement& element)
        {
            for (const BoxElement::Property& property : element.m_properties)
            {
                if (!property.m_name)
                {
                    continue;
                }
                QVariant value = property.m_value;
                if (value.userType() == QMetaType::QString && widget->property(property.m_name).userType() == QMetaType::QIcon)
                {
                    value = QIcon(value.toString());
                }
                widget->setProperty(property.m_name, value);
            }
        }

        //! The classes whose own background a rule paints (QStyleSheetStyle::polish): a plain widget, a header view, a tab bar, a
        //! frame, a main window, an MDI sub-window, a menu bar and a dialog
        bool paintsStyledBackground(const QWidget* widget)
        {
            return widget->metaObject() == &QWidget::staticMetaObject || qobject_cast<const QHeaderView*>(widget)
                || qobject_cast<const QTabBar*>(widget) || qobject_cast<const QFrame*>(widget) || qobject_cast<const QMainWindow*>(widget)
                || qobject_cast<const QMdiSubWindow*>(widget) || qobject_cast<const QMenuBar*>(widget)
                || qobject_cast<const QDialog*>(widget);
        }

        //! What QStyleSheetStyle::polish set for a widget's rules: WA_Hover and mouse tracking for a :hover, the styled background of a
        //! rule that draws or has a box, and the autofill that background replaces
        void applyAttributes(QWidget* widget, const BoxElement& element)
        {
            QWidget* embedded = embeddedWidget(widget);
            for (const BoxElement::StateFace& stateFace : element.m_stateFaces)
            {
                if (stateFace.m_states & BoxElement::Hover)
                {
                    widget->setAttribute(Qt::WA_Hover, true);
                    embedded->setAttribute(Qt::WA_Hover, true);
                    embedded->setMouseTracking(true);
                    break;
                }
            }
            if (drawsInAnyState(element) || hasBoxInAnyState(element))
            {
                if (paintsStyledBackground(widget))
                {
                    widget->setAttribute(Qt::WA_StyledBackground, true);
                }
                if (embedded->autoFillBackground())
                {
                    // The background is the style's to paint from now on, through the embedded widget too (a scroll area's viewport)
                    embedded->setAutoFillBackground(false);
                    widget->setProperty(AutoFillProperty, true);
                    if (embedded != widget)
                    {
                        embedded->setAttribute(Qt::WA_StyledBackground, true);
                    }
                }
            }
        }

        // ------------------------------------------------------------------------------
        // Helpers: painting (QRenderRule::drawRule; qcssutil.cpp)
        // ------------------------------------------------------------------------------

        enum Side       // QCss::Edge
        {
            TopSide,
            RightSide,
            BottomSide,
            LeftSide,
        };

        //! qNormalizeRadii for one radius: a corner that does not fit is square
        int fittingRadius(int radius, const QRect& rect)
        {
            return (radius > 0 && 2 * radius <= rect.width() && 2 * radius <= rect.height()) ? radius : 0;
        }

        //! qNormalizeRadii for the four corners: none below 0, and two corners wider together than their side are both square (the top
        //! pair, then the bottom pair), as are two higher than theirs (left, then right)
        BoxElement::Radii normalizedRadii(BoxElement::Radii radii, const QRect& rect)
        {
            radii.m_topLeft = qMax(0, radii.m_topLeft);
            radii.m_topRight = qMax(0, radii.m_topRight);
            radii.m_bottomRight = qMax(0, radii.m_bottomRight);
            radii.m_bottomLeft = qMax(0, radii.m_bottomLeft);
            if (radii.m_topLeft + radii.m_topRight > rect.width())
            {
                radii.m_topLeft = radii.m_topRight = 0;
            }
            if (radii.m_bottomLeft + radii.m_bottomRight > rect.width())
            {
                radii.m_bottomLeft = radii.m_bottomRight = 0;
            }
            if (radii.m_topLeft + radii.m_bottomLeft > rect.height())
            {
                radii.m_topLeft = radii.m_bottomLeft = 0;
            }
            if (radii.m_topRight + radii.m_bottomRight > rect.height())
            {
                radii.m_topRight = radii.m_bottomRight = 0;
            }
            return radii;
        }

        bool anyRound(const BoxElement::Radii& radii)
        {
            return radii.m_topLeft > 0 || radii.m_topRight > 0 || radii.m_bottomRight > 0 || radii.m_bottomLeft > 0;
        }

        BoxElement::Radii allCorners(int radius)
        {
            return { radius, radius, radius, radius };
        }

        //! A fill that shows nothing is not drawn (the legacy '*' rule's, BaseStyleSheet.qss:15, is one)
        bool isInvisible(const QBrush& brush)
        {
            return brush.style() == Qt::NoBrush || (brush.style() == Qt::SolidPattern && brush.color().alpha() == 0);
        }

        //! QRenderRule::configurePalette for a fill: the background of the control and of its window, and the shades of a bevel from a
        //! solid one
        void applyFill(QPalette& palette, const QBrush& fill, QPalette::ColorRole role)
        {
            if (fill.style() == Qt::NoBrush)
            {
                return;
            }
            palette.setBrush(role, fill);
            palette.setBrush(QPalette::Window, fill);
            if (fill.style() == Qt::SolidPattern)
            {
                setShades(palette, fill.color());
            }
        }

        //! QRenderRule::borderClip: the path through the middle of the border. The radii are normalized and one at least is round.
        QPainterPath borderPath(const QRect& rect, const QMargins& border, const BoxElement::Radii& radii)
        {
            const QRectF r(rect);
            QPainterPath path;
            qreal curY = r.y() + border.top() / 2.0;
            path.moveTo(r.x() + radii.m_topLeft, curY);
            path.lineTo(r.right() - radii.m_topRight, curY);
            qreal curX = r.right() - border.right() / 2.0;
            path.arcTo(curX - 2 * radii.m_topRight + border.right(), curY, radii.m_topRight * 2 - border.right(),
                radii.m_topRight * 2 - border.top(), 90, -90);
            path.lineTo(curX, r.bottom() - radii.m_bottomRight);
            curY = r.bottom() - border.bottom() / 2.0;
            path.arcTo(curX - 2 * radii.m_bottomRight + border.right(), curY - 2 * radii.m_bottomRight + border.bottom(),
                radii.m_bottomRight * 2 - border.right(), radii.m_bottomRight * 2 - border.bottom(), 0, -90);
            path.lineTo(r.x() + radii.m_bottomLeft, curY);
            curX = r.left() + border.left() / 2.0;
            path.arcTo(curX, r.bottom() - 2 * radii.m_bottomLeft + border.bottom() / 2.0, radii.m_bottomLeft * 2 - border.left(),
                radii.m_bottomLeft * 2 - border.bottom(), 270, -90);
            path.lineTo(curX, r.top() + radii.m_topLeft);
            path.arcTo(
                curX, r.top() + border.top() / 2.0, radii.m_topLeft * 2 - border.left(), radii.m_topLeft * 2 - border.top(), 180, -90);
            path.closeSubpath();
            return path;
        }

        //! The same path for one radius on all four corners
        QPainterPath borderPath(const QRect& rect, const QMargins& border, int radius)
        {
            return borderPath(rect, border, allCorners(radius));
        }

        //! qDrawRoundedCorners for a solid edge: the quarter-circle arcs at the ends of its straight part, stroked as wide as the edge.
        //! `first` is the radius of the corner at the edge's start (left of a top or bottom edge, top of a side), `second` at its end;
        //! a corner of 0 has no arc.
        void drawCorners(QPainter* painter, const QRectF& strip, qreal first, qreal second, Side side, const QColor& color)
        {
            const bool horizontal = side == TopSide || side == BottomSide;
            const qreal width = horizontal ? strip.height() : strip.width();
            const qreal half = width / 2;
            QPen pen(color, width, Qt::SolidLine, Qt::SquareCap);       // square caps: no off-by-one errors
            painter->save();
            painter->setBrush(Qt::NoBrush);
            painter->setPen(pen);
            const qreal x1 = strip.left();
            const qreal y1 = strip.top();
            const qreal x2 = strip.right();
            const qreal y2 = strip.bottom();
            const auto arc = [painter, width](qreal x, qreal y, qreal radius, int startDegrees, int spanDegrees)
            {
                if (radius > 0)
                {
                    painter->drawArc(QRectF(x, y, 2 * radius - width, 2 * radius - width), startDegrees * 16, spanDegrees * 16);
                }
            };
            switch (side)
            {
            case TopSide:
                arc(x1 - first + half, y1 + half, first, 135, -45);
                arc(x2 - second + half, y1 + half, second, 45, 45);
                break;
            case BottomSide:
                arc(x1 - first + half, y2 - 2 * first + half, first, -90, -45);
                arc(x2 - second + half, y2 - 2 * second + half, second, -90, 45);
                break;
            case LeftSide:
                arc(x1 + half, y1 - first + half, first, 135, 45);
                arc(x1 + half, y2 - second + half, second, 180, 45);
                break;
            case RightSide:
                arc(x2 - 2 * first + half, y1 - first + half, first, 45, -45);
                arc(x2 - 2 * second + half, y2 - second + half, second, 315, 45);
                break;
            }
            painter->restore();
        }

        //! qDrawBorder for solid edges, antialiased as QRenderRule::drawBorder draws them: the edges in increasing precedence (bottom,
        //! right, left, top), each a strip between its corners, with arcs where the radius is. Qt draws an edge wider than 1px that
        //! meets a visible edge of another colour as a trapezoid (qDrawEdge); here the later edge takes the corner (no element has one).
        //! The radii are normalized.
        void drawBorder(QPainter* painter, const QRect& rect, const BoxElement::Edges& edges, const BoxElement::Radii& radii)
        {
            const auto shows = [](const BoxElement::Edge& edge)
            {
                return edge.m_width > 0 && edge.m_color.isValid();
            };
            if (!shows(edges.m_top) && !shows(edges.m_right) && !shows(edges.m_bottom) && !shows(edges.m_left))
            {
                return;
            }
            const QRectF box(rect);
            const qreal topLeft = radii.m_topLeft;
            const qreal topRight = radii.m_topRight;
            const qreal bottomRight = radii.m_bottomRight;
            const qreal bottomLeft = radii.m_bottomLeft;
            struct Entry
            {
                Side m_side;
                const BoxElement::Edge* m_edge;
                QRectF m_strip;
                qreal m_first;      //!< the corner at the start of the strip
                qreal m_second;     //!< the corner at its end
            };
            const Entry order[] = {
                { BottomSide, &edges.m_bottom,
                    QRectF(box.left() + bottomLeft, box.bottom() - edges.m_bottom.m_width, box.width() - bottomLeft - bottomRight,
                        edges.m_bottom.m_width),
                    bottomLeft, bottomRight },
                { RightSide, &edges.m_right,
                    QRectF(box.right() - edges.m_right.m_width, box.top() + topRight, edges.m_right.m_width,
                        box.height() - topRight - bottomRight),
                    topRight, bottomRight },
                { LeftSide, &edges.m_left,
                    QRectF(box.left(), box.top() + topLeft, edges.m_left.m_width, box.height() - topLeft - bottomLeft),
                    topLeft, bottomLeft },
                { TopSide, &edges.m_top,
                    QRectF(box.left() + topLeft, box.top(), box.width() - topLeft - topRight, edges.m_top.m_width),
                    topLeft, topRight },
            };
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            for (const Entry& entry : order)
            {
                if (!shows(*entry.m_edge))
                {
                    continue;
                }
                painter->setBrush(entry.m_edge->m_color);
                painter->drawRect(entry.m_strip);
                if (entry.m_first > 0 || entry.m_second > 0)
                {
                    drawCorners(painter, entry.m_strip, entry.m_first, entry.m_second, entry.m_side, entry.m_edge->m_color);
                }
            }
            painter->restore();
        }

        //! The same border for one radius on all four corners
        void drawBorder(QPainter* painter, const QRect& rect, const BoxElement::Edges& edges, int radius)
        {
            drawBorder(painter, rect, edges, allCorners(radius));
        }

        //! The offset a scroll area's contents are scrolled by, which its background image scrolls with
        //! (QAbstractScrollAreaPrivate::contentsOffset)
        QPoint scrollOffset(const QWidget* widget)
        {
            const auto* area = qobject_cast<const QAbstractScrollArea*>(widget);
            QPoint offset;
            if (area && area->verticalScrollBar()->isVisible())
            {
                offset.setY(area->verticalScrollBar()->value());
            }
            if (area && area->horizontalScrollBar()->isVisible())
            {
                offset.setX(area->isRightToLeft() ? area->horizontalScrollBar()->maximum() - area->horizontalScrollBar()->value()
                                                  : area->horizontalScrollBar()->value());
            }
            return offset;
        }

        //! QRenderRule::drawBackgroundImage for an image that does not repeat: the file at its natural size
        //! (QStyleSheetStyle::loadPixmap), placed in the origin box by the alignment, moved with the scrolled contents, clipped to the
        //! border box
        void drawImage(QPainter* painter, const BoxElement& element, const QRect& border, const QMargins& widths, int radius,
            const char* resource, const QPoint& offset)
        {
            const QPixmap pixmap = Style::cachedPixmap(QString::fromLatin1(resource));
            if (pixmap.isNull())
            {
                return;
            }
            const QRect padding = border.marginsRemoved(widths);
            const QRect origin = element.m_imageOrigin == BoxElement::Origin::Content ? padding.marginsRemoved(element.m_padding) : padding;
            const QSize size = pixmap.size() / pixmap.devicePixelRatio();
            const QRect aligned = QStyle::alignedRect(Qt::LeftToRight, element.m_imageAlign, size, origin);
            const QRect visible = aligned.translated(-offset).intersected(origin);

            painter->save();
            if (radius > 0)
            {
                painter->setClipPath(borderPath(border, widths, radius), Qt::IntersectClip);
            }
            painter->setClipRect(border, Qt::IntersectClip);
            painter->drawPixmap(visible.x(), visible.y(), pixmap, visible.x() - aligned.x() + offset.x(),
                visible.y() - aligned.y() + offset.y(), pixmap.width(), pixmap.height());
            painter->restore();
        }

        //! QRenderRule::drawBorderImage for `border-image: url() 0 0 0 0 stretch stretch`: qDrawBorderPixmap over the border box with
        //! the border's widths as margins, cuts of 0 and stretch tiles, which draws only the image's center, stretched over what the
        //! margins leave, smooth, clipped to the radius (QRenderRule::setClip). False: the file does not load, and Qt drops the border
        //! image (QRenderRule::fixupBorder).
        bool drawBorderImage(QPainter* painter, const QRect& border, const QMargins& widths, int radius, const char* resource)
        {
            const QPixmap pixmap = Style::cachedPixmap(QString::fromLatin1(resource));
            if (pixmap.isNull())
            {
                return false;
            }
            painter->save();
            if (radius > 0)
            {
                painter->setClipPath(borderPath(border, widths, radius), Qt::IntersectClip);
            }
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            qDrawBorderPixmap(painter, border, widths, pixmap, QRect(QPoint(), pixmap.size()), QMargins(),
                QTileRules(Qt::StretchTile, Qt::StretchTile));
            painter->restore();
            return true;
        }
    } // namespace BoxElementInternal

    // ----------------------------------------------------------------------------------
    // Geometry
    // ----------------------------------------------------------------------------------

    bool BoxElement::hasNativeBorder(const QStyleOption* option) const
    {
        // a border image is the rule's own border (QRenderRule::hasNativeBorder)
        return m_nativeBorder && !BoxElementInternal::isFlat(option) && !BoxElementInternal::hasBorderImage(face(option));
    }

    bool BoxElement::ownsBox(const QStyleOption* option) const
    {
        // QRenderRule::hasBox() || !hasNativeBorder(), the test of every geometry function of QStyleSheetStyle
        return BoxElementInternal::hasBox(*this, face(option)) || !hasNativeBorder(option);
    }

    QRect BoxElement::borderRect(const QStyleOption* option) const
    {
        return option->rect.marginsRemoved(BoxElementInternal::marginOf(*this, face(option)));
    }

    QRect BoxElement::contentsRect(const QStyleOption* option, const QWidget* widget) const
    {
        return option->rect.marginsRemoved(BoxElementInternal::around(*this, face(option), BoxElementInternal::isFlat(option), widget));
    }

    QSize BoxElement::boxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const
    {
        const QMargins box = BoxElementInternal::around(*this, face(option), BoxElementInternal::isFlat(option), widget);
        return QSize(contents.width() + box.left() + box.right(), contents.height() + box.top() + box.bottom());
    }

    QSize BoxElement::adjustSize(const QSize& contents) const
    {
        // QRenderRule::adjustSize: the declared size replaces the contents' size, the maximum caps it, the minimum raises it
        QSize size(m_size.width() >= 0 ? m_size.width() : contents.width(), m_size.height() >= 0 ? m_size.height() : contents.height());
        if (m_maxSize.width() >= 0)
        {
            size.setWidth(qMin(size.width(), m_maxSize.width()));
        }
        if (m_maxSize.height() >= 0)
        {
            size.setHeight(qMin(size.height(), m_maxSize.height()));
        }
        return size.expandedTo(m_minSize);
    }

    int BoxElement::spacing(const QStyleOption* option) const
    {
        return BoxElementInternal::spacingOf(*this, face(option));
    }

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    const BoxElement::Face& BoxElement::face(const QStyleOption* option) const
    {
        const unsigned states = BoxElementInternal::statesOf(option);

        // A rule applies while all of its pseudo-classes hold, and a later rule overrides an earlier one (declarations,
        // qstylesheetstyle.cpp)
        const Face* result = &m_idle;
        for (const StateFace& stateFace : m_stateFaces)
        {
            if (stateFace.m_states != 0 && (stateFace.m_states & states) == stateFace.m_states)
            {
                result = &stateFace.m_face;
            }
        }
        return *result;
    }

    bool BoxElement::draw(QPainter* painter, const QStyleOption* option, const QWidget* widget) const
    {
        // QRenderRule::hasDrawable() of the state: a native border with no fill and no image draws nothing
        const Face& state = face(option);
        const bool image = state.m_image[0] != 0;
        if (hasNativeBorder(option) && state.m_fill.style() == Qt::NoBrush && !image)
        {
            return false;
        }

        // drawRule: the fill over the border box, rounded (antialiased) where the border is, then the image, then the border or the
        // border image over both; the box is the state's
        const QRect border = borderRect(option);
        const QMargins widths = BoxElementInternal::borderOf(*this, state, BoxElementInternal::isFlat(option), widget);
        const int radius = BoxElementInternal::fittingRadius(BoxElementInternal::radiusOf(*this, state), border);
        if (!BoxElementInternal::isInvisible(state.m_fill))
        {
            if (radius > 0)
            {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->fillPath(BoxElementInternal::borderPath(border, widths, radius), state.m_fill);
                painter->restore();
            }
            else
            {
                painter->fillRect(border, state.m_fill);
            }
        }
        if (image)
        {
            BoxElementInternal::drawImage(painter, *this, border, widths, radius, state.m_image, BoxElementInternal::scrollOffset(widget));
        }
        if (!BoxElementInternal::hasBorderImage(state)
            || !BoxElementInternal::drawBorderImage(painter, border, widths, radius, state.m_borderImage))
        {
            BoxElementInternal::drawBorder(painter, border, state.m_edges, radius);
        }
        return true;
    }

    void BoxElement::drawFrame(QPainter* painter, const QRect& borderBox, const QBrush& fill, const Edges& edges, const Radii& radii)
    {
        drawFill(painter, borderBox, fill, edges, radii);
        drawEdges(painter, borderBox, edges, radii);
    }

    void BoxElement::drawFill(QPainter* painter, const QRect& borderBox, const QBrush& fill, const Edges& edges, const Radii& radii)
    {
        if (BoxElementInternal::isInvisible(fill))
        {
            return;
        }
        // The path is filled rather than clipped to, for the quality
        const QPainterPath path = borderPath(borderBox, edges, radii);
        if (path.isEmpty())
        {
            painter->fillRect(borderBox, fill);
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(path, fill);
        painter->restore();
    }

    void BoxElement::drawEdges(QPainter* painter, const QRect& borderBox, const Edges& edges, const Radii& radii)
    {
        BoxElementInternal::drawBorder(painter, borderBox, edges, BoxElementInternal::normalizedRadii(radii, borderBox));
    }

    QPainterPath BoxElement::borderPath(const QRect& borderBox, const Edges& edges, const Radii& radii)
    {
        const Radii fitted = BoxElementInternal::normalizedRadii(radii, borderBox);
        if (!BoxElementInternal::anyRound(fitted))
        {
            return QPainterPath();
        }
        const auto width = [](const Edge& edge)
        {
            return edge.m_color.isValid() ? edge.m_width : 0;
        };
        const QMargins widths(width(edges.m_left), width(edges.m_top), width(edges.m_right), width(edges.m_bottom));
        return BoxElementInternal::borderPath(borderBox, widths, fitted);
    }

    bool BoxElement::baseStyleCanDraw(const Face& face, const QWidget* styled)
    {
        // An image is the rule's to draw, a gradient the base style's only where the class lists it among its features
        // (QRenderRule::baseStyleCanDraw)
        const Qt::BrushStyle fill = face.m_fill.style();
        const bool gradient = fill >= Qt::LinearGradientPattern && fill <= Qt::ConicalGradientPattern;
        return face.m_image[0] == 0
            && (fill == Qt::NoBrush || fill == Qt::SolidPattern || (gradient && qobject_cast<const QComboBox*>(styled)));
    }

    void BoxElement::configurePalette(QPalette& palette, const Face& face)
    {
        BoxElementInternal::applyFill(palette, face.m_fill, QPalette::Base);
    }

    void BoxElement::configurePalette(QPalette& palette, const Face& face, QPalette::ColorRole foreground, QPalette::ColorRole background)
    {
        // The state's colour goes to the role of the control's text, to WindowText and to Text (QRenderRule::configurePalette)
        if (background != QPalette::NoRole)
        {
            BoxElementInternal::applyFill(palette, face.m_fill, background);
        }
        if (face.m_text.isValid())
        {
            palette.setColor(foreground, face.m_text);
            palette.setColor(QPalette::WindowText, face.m_text);
            palette.setColor(QPalette::Text, face.m_text);
        }
    }

    QBrush BoxElement::vertical(const QColor& top, const QColor& bottom)
    {
        // relative to the box the brush fills (parseBrushValue, qcssparser.cpp)
        QLinearGradient gradient(0.0, 0.0, 0.0, 1.0);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0.0, top);
        gradient.setColorAt(1.0, bottom);
        return QBrush(gradient);
    }

    BoxElement::Edges BoxElement::border(int width, const QColor& color)
    {
        const Edge edge{ width, color };
        return { edge, edge, edge, edge };
    }

    // ----------------------------------------------------------------------------------
    // Push buttons
    // ----------------------------------------------------------------------------------

    bool BoxElement::drawPushButtonLabel(
        const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const
    {
        if (!m_textAlign)
        {
            return false;
        }

        // QStyleSheetStyle::drawControl, CE_PushButtonLabel, for a rule with a position and no `icon` of its own (the icon is the
        // button's). The rule's alignment, centered where it names none:
        constexpr uint horizontalAlignMask = Qt::AlignHCenter | Qt::AlignLeft | Qt::AlignRight;
        constexpr uint verticalAlignMask = Qt::AlignVCenter | Qt::AlignTop | Qt::AlignBottom;
        const uint alignment = uint(m_textAlign);
        uint textFlags = Qt::TextShowMnemonic;
        QRect textRect = option->rect;
        textFlags |= (alignment & verticalAlignMask) ? (alignment & verticalAlignMask) : uint(Qt::AlignVCenter);
        textFlags |= (alignment & horizontalAlignMask) ? (alignment & horizontalAlignMask) : uint(Qt::AlignHCenter);
        if (!style->styleHint(QStyle::SH_UnderlineShortcut, option, widget))
        {
            textFlags |= Qt::TextHideMnemonic;
        }

        const bool shifted = (option->state & (QStyle::State_On | QStyle::State_Sunken)) != 0;
        const int shiftX = shifted ? style->pixelMetric(QStyle::PM_ButtonShiftHorizontal, option, widget) : 0;
        const int shiftY = shifted ? style->pixelMetric(QStyle::PM_ButtonShiftVertical, option, widget) : 0;

        if (!option->icon.isNull())
        {
            // The icon and the text in one group, 4px apart as QPushButton::sizeHint has them, placed by the alignment
            QIcon::Mode mode = (option->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled;
            if (mode == QIcon::Normal && (option->state & QStyle::State_HasFocus))
            {
                mode = QIcon::Active;
            }
            const QIcon::State iconState = (option->state & QStyle::State_On) ? QIcon::On : QIcon::Off;
            const QPixmap pixmap = option->icon.pixmap(option->iconSize, painter->device()->devicePixelRatio(), mode, iconState);
            const int pixmapWidth = static_cast<int>(pixmap.width() / pixmap.devicePixelRatio());
            const int pixmapHeight = static_cast<int>(pixmap.height() / pixmap.devicePixelRatio());
            int labelWidth = pixmapWidth;
            const int labelHeight = pixmapHeight;
            if (!option->text.isEmpty())
            {
                labelWidth += option->fontMetrics.boundingRect(option->rect, int(textFlags), option->text).width()
                    + BoxElementInternal::PushButtonIconSpacing;
            }

            QRect iconRect;
            if (textFlags & Qt::AlignLeft)
            {
                iconRect = QRect(textRect.x(), textRect.y() + (textRect.height() - labelHeight) / 2, pixmapWidth, pixmapHeight);
            }
            else if (textFlags & Qt::AlignHCenter)
            {
                iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2, textRect.y() + (textRect.height() - labelHeight) / 2,
                    pixmapWidth, pixmapHeight);
            }
            else
            {
                iconRect = QRect(textRect.x() + textRect.width() - labelWidth, textRect.y() + (textRect.height() - labelHeight) / 2,
                    pixmapWidth, pixmapHeight);
            }
            iconRect = QStyle::visualRect(option->direction, textRect, iconRect);

            // Left aligned, the text rect begins after the icon instead
            textFlags &= ~horizontalAlignMask;
            textFlags |= Qt::AlignLeft;
            if (option->direction == Qt::RightToLeft)
            {
                textRect.setRight(iconRect.left() - BoxElementInternal::PushButtonIconSpacing);
            }
            else
            {
                textRect.setLeft(iconRect.left() + iconRect.width() + BoxElementInternal::PushButtonIconSpacing);
            }

            iconRect.translate(shiftX, shiftY);
            painter->drawPixmap(iconRect, pixmap);
        }

        textRect.translate(shiftX, shiftY);
        if (option->features & QStyleOptionButton::HasMenu)
        {
            const int indicatorSize = style->pixelMetric(QStyle::PM_MenuButtonIndicator, option, widget);
            textRect = (option->direction == Qt::LeftToRight) ? textRect.adjusted(0, 0, -indicatorSize, 0)
                                                               : textRect.adjusted(indicatorSize, 0, 0, 0);
        }
        style->drawItemText(painter, textRect, int(textFlags), option->palette, (option->state & QStyle::State_Enabled) != 0, option->text,
            QPalette::ButtonText);
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Tool buttons
    // ----------------------------------------------------------------------------------

    bool BoxElement::drawToolButton(
        const QStyle* style, const QStyleOptionToolButton* option, QPainter* painter, const QWidget* widget) const
    {
        if (hasNativeBorder(option))
        {
            return false;
        }

        // The rule over the whole rect, then the label in its contents rect, in the rule's palette (QStyleSheetStyle::drawComplexControl)
        QStyleOptionToolButton tool(*option);
        configurePalette(tool.palette, face(option), QPalette::ButtonText, QPalette::Button);
        draw(painter, option, widget);
        tool.rect = contentsRect(option, widget);
        style->drawControl(QStyle::CE_ToolButtonLabel, &tool, painter, widget);

        // The drop-down of a split button, or a menu's indicator, as the base style draws them where the rule has no ::menu-button,
        // ::menu-arrow or ::menu-indicator
        const bool hasMenu = option->features.testFlag(QStyleOptionToolButton::HasMenu);
        if (option->features.testFlag(QStyleOptionToolButton::MenuButtonPopup))
        {
            if (option->subControls & QStyle::SC_ToolButtonMenu)
            {
                tool.rect = style->subControlRect(QStyle::CC_ToolButton, option, QStyle::SC_ToolButtonMenu, widget);
                style->drawPrimitive(QStyle::PE_IndicatorButtonDropDown, &tool, painter, widget);
                if (hasMenu)
                {
                    style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &tool, painter, widget);
                }
            }
        }
        else if (hasMenu)
        {
            // QStyleSheetStyle::positionRect: right and bottom of the padding box, 13 x 13 (QStyleSheetStyle::defaultSize)
            const QRect padding = contentsRect(option, widget).marginsAdded(m_padding);
            tool.rect = QStyle::alignedRect(
                option->direction, Qt::AlignRight | Qt::AlignBottom, BoxElementInternal::MenuIndicatorSize, padding);
            style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &tool, painter, widget);
        }
        return true;
    }

    QRect BoxElement::toolButtonRect(const QStyleOptionToolButton* option, QStyle::SubControl subControl, int menuWidth) const
    {
        const QRect border = borderRect(option);
        if (subControl == QStyle::SC_ToolButtonMenu)
        {
            // QStyleSheetStyle::positionRect of a ::menu-button the rule does not have: right and top of the border box, its height
            return QStyle::alignedRect(option->direction, Qt::AlignRight | Qt::AlignTop, QSize(menuWidth, border.height()), border);
        }
        return border;
    }

    // ----------------------------------------------------------------------------------
    // Combo boxes
    // ----------------------------------------------------------------------------------

    QSize BoxElement::comboBoxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const
    {
        return boxSize(
            option, widget, contents + QSize(BoxElementInternal::dropDownBoxWidth(*this) + BoxElementInternal::ComboLabelExtra, 0));
    }

    QRect BoxElement::dropDownRect(const QStyleOption* option, const QWidget* widget) const
    {
        // A drop-down's origin is the padding box (defaultOrigin, qstylesheetstyle.cpp): the contents and the padding
        const QRect padding = contentsRect(option, widget).marginsAdded(m_padding);
        return QStyle::alignedRect(option->direction, Qt::AlignRight | Qt::AlignTop,
            QSize(BoxElementInternal::dropDownBoxWidth(*this), padding.height()), padding);
    }

    QRect BoxElement::editFieldRect(const QStyleOption* option, const QWidget* widget) const
    {
        const QRect contents = contentsRect(option, widget);
        return QStyle::visualRect(option->direction, contents, contents.adjusted(0, 0, -dropDownRect(option, widget).width(), 0));
    }

    QRect BoxElement::arrowRect(const QStyleOption* option, const QWidget* widget) const
    {
        // The image and its margin on the right are centered in the drop-down's contents (QStyleSheetStyle::positionRect); the image
        // is that box less the margin
        const int size = BoxElementInternal::ArrowSize;
        const QRect box = QStyle::alignedRect(option->direction, Qt::AlignCenter, QSize(size + BoxElementInternal::ArrowMarginRight, size),
            dropDownRect(option, widget).marginsRemoved(m_dropDownPadding));
        return QRect(box.topLeft(), QSize(size, size));
    }

    bool BoxElement::drawComboBoxLabel(
        const QStyle* style, const QStyleOptionComboBox* option, QPainter* painter, const QWidget* widget) const
    {
        // QRenderRule::hasDrawable() of the state: a rule that draws nothing leaves the label to the base style
        const Face& state = face(option);
        if (hasNativeBorder(option) && state.m_fill.style() == Qt::NoBrush && state.m_image[0] == 0)
        {
            return false;
        }

        // The label in the edit field, clipped to it (QStyleSheetStyle::drawControl, CE_ComboBoxLabel): the icon at its left, the text
        // after it and the state's spacing, 6px where the rule has none
        QRect editRect = editFieldRect(option, widget);
        painter->save();
        painter->setClipRect(editRect);
        if (!option->currentIcon.isNull())
        {
            const int labelSpacing = spacing(option);
            const int gap = labelSpacing != Unset ? labelSpacing : BoxElementInternal::ComboIconSpacing;
            const QIcon::Mode mode = option->state.testFlag(QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled;
            const QPixmap pixmap = option->currentIcon.pixmap(option->iconSize, painter->device()->devicePixelRatio(), mode);
            QRect iconRect(editRect);
            iconRect.setWidth(option->iconSize.width());
            iconRect = QStyle::alignedRect(option->direction, Qt::AlignLeft | Qt::AlignVCenter, iconRect.size(), editRect);
            style->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);
            editRect.translate((option->direction == Qt::RightToLeft ? -1 : 1) * (option->iconSize.width() + gap), 0);
        }
        if (!option->currentText.isEmpty() && !option->editable)
        {
            QPalette palette(option->palette);
            configurePalette(palette, state, QPalette::Text, QPalette::Base);
            style->drawItemText(painter, editRect, option->textAlignment, palette, option->state.testFlag(QStyle::State_Enabled),
                option->currentText, QPalette::Text);
        }
        painter->restore();
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Check boxes and radio buttons
    // ----------------------------------------------------------------------------------

    bool BoxElement::drawIndicatorButton(
        const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const
    {
        if (!IndicatorElement::forWidget(widget))
        {
            return false;
        }

        // The sheet's rule drew over the whole rect, and the style laid the indicator and the label out inside its contents. The
        // label drew in the colour of the rule for the state (the palette of the widget has the rest).
        draw(painter, option, widget);
        QStyleOptionButton inner(*option);
        inner.rect = contentsRect(option, widget);
        configurePalette(inner.palette, face(option), QPalette::ButtonText, QPalette::NoRole);
        return IndicatorElement::draw(style, &inner, painter, widget);
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void BoxElement::registerElement(
        const BoxElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        BoxElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, &BoxElement::resolve);
    }

    void BoxElement::resolve(QWidget* widget)
    {
        using namespace BoxElementInternal;
        if (registry().isEmpty() || !Style::hasStyle(widget))
        {
            return;
        }
        const BoxElement* previous = forWidget(widget);
        // The sheet styles no line edit of a combo box or a spin box and no viewport by a rule of its own: the rule of the container is
        // drawn for it (unstylable, qstylesheetstyle.cpp)
        const BoxElement* element = (containerOf(widget) == widget) ? registry().find(widget) : nullptr;
        if (!previous && !element)
        {
            return;
        }

        // Kept on the widget for the contact points of Style; a widget with no element keeps nothing
        if (previous != element)
        {
            if (element)
            {
                registry().keep(widget, element);
            }
            else
            {
                registry().forget(widget);
            }
        }

        applyLimits(widget, previous, element);
        if (element)
        {
            applyProperties(widget, *element);
            applyAttributes(widget, *element);
        }

        // The first polish is followed by the Polish event, on which a QFrame reads its box. A widget polished already is told
        // its style changed, as QStyleSheetStyle told the widgets it repolished (updateObjects, qstylesheetstyle.cpp)
        if (previous != element && widget->testAttribute(Qt::WA_WState_Polished))
        {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(widget, &styleChange);
        }
    }

    const BoxElement* BoxElement::forWidget(const QWidget* widget)
    {
        if (!widget || BoxElementInternal::registry().isEmpty())
        {
            return nullptr;
        }
        return BoxElementInternal::registry().kept(widget).value_or(nullptr);
    }

    bool BoxElement::fillsBackgroundRole(const QWidget* widget, const QStyleOption* option)
    {
        // A viewport takes its scroll area's element. It is filled where the scroll area has no background, and a widget whose own
        // element draws nothing is filled too (QStyleSheetStyle::drawPrimitive, PE_Widget)
        const QWidget* container = containerOf(widget);
        const BoxElement* box = forWidget(container);
        if (!box || !container->property(BoxElementInternal::AutoFillProperty).toBool())
        {
            return false;
        }
        const Face& state = box->face(option);
        return container == widget || (state.m_fill.style() == Qt::NoBrush && state.m_image[0] == 0);
    }

    const QWidget* BoxElement::containerOf(const QWidget* widget)
    {
        // The line edit of a combo box or a spin box and the viewport of a scroll area take their container's rule (containerWidget,
        // qstylesheetstyle.cpp)
        const QWidget* parent = widget ? widget->parentWidget() : nullptr;
        const bool lineEdit = qobject_cast<const QLineEdit*>(widget)
            && (qobject_cast<const QComboBox*>(parent) || qobject_cast<const QAbstractSpinBox*>(parent));
        const auto* area = qobject_cast<const QAbstractScrollArea*>(parent);
        return (lineEdit || (area && area->viewport() == widget)) ? parent : widget;
    }
} // namespace AzQtComponents
