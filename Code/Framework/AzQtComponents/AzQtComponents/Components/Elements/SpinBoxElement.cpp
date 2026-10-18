/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/SpinBoxElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>

#include <QColor>
#include <QDoubleSpinBox>
#include <QGuiApplication>
#include <QIcon>
#include <QMargins>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOption>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others
    //! into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace SpinBoxElementInternal
    {
        // ------------------------------------------------------------------------------
        // The face of the box
        //
        // Rules of the cascade, by specificity (Qt's: a type selector 1, a class, attribute or pseudo-class 0x100) and, for equal
        // ones, the later: BaseStyleSheet.qss:68-84 (0x001), SpinBox.qss:9-20 (0x001), :22-31 `:focus, :hover` (0x101), :33-40
        // `:hover`, :42-47 `:focus`, :49-55 `:hover:focus` (0x201), :57-67 `:enabled[HasError=true]` (0x201), :69-74 `:disabled`
        // (0x101); VectorInput.qss:18-29 (0x002, with :hover or :focus 0x102); ColorPicker.qss:66-72 (:hover 0x102, :hover:focus 0x202).
        // The `[hasAcceptableInput=false]` of SpinBox.qss:57-58 names no property of a spin box (Qt's is `acceptableInput`,
        // qabstractspinbox.h:34) and never matched: only the HasError property gives the error face. UNVERIFIED: no capture shows a
        // box with unacceptable input.
        // VectorInput.qss:31-35 gives a hovered spin box of a vector element `height: 19px`. It paints nothing: the rule's height is
        // read by the size hint (QAbstractSpinBox caches it) and by the geometry set at polish, in the state Enabled alone
        // (qstylesheetstyle.cpp:2572). Not carried. UNVERIFIED.
        //
        // The captures (n2, 200 spin boxes of the Gallery, Editor, Material and Pass Canvas: every pixel outside the line edit equal)
        // show the box at rest in a plain box, in a vector element and in a color picker, disabled, with the error border (plain),
        // and with the focus in a color picker (Gallery 07_color-picker__dialog0). UNVERIFIED, carried from the rules and equal to the
        // sheet style's own drawing in the tests: every face with the pointer on the box, the focus outside a color picker, the error
        // face in a vector element or a color picker, and the images of the buttons for the flags the watcher sets while a value
        // moves (increasing, decreasing, a limit reached, dragging, focused: no capture has one of them set but focused, in a picker).
        // ------------------------------------------------------------------------------

        enum Corner     // QCss::Corner: the order of the radii of a rule
        {
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight,
        };

        const QColor FieldFill(0xCC, 0xCC, 0xCC);                   // BaseStyleSheet.qss:76. TOKENIZE: SpinBoxFieldColor
        const QColor DisabledFill(0x66, 0x66, 0x66);                // SpinBox.qss:72. TOKENIZE: SpinBoxDisabledFieldColor
        const QColor FocusFill(0xFF, 0xFF, 0xFF);                   // SpinBox.qss:46. TOKENIZE: SpinBoxFocusFill
        const QColor HoverBorder(0x22, 0x22, 0x22);                 // SpinBox.qss:36. TOKENIZE: SpinBoxHoverBorder
        const QColor FocusBorder(0x94, 0xD2, 0xFF);                 // SpinBox.qss:45. TOKENIZE: SpinBoxFocusBorder
        const QColor ErrorBorder(0xE2, 0x52, 0x43);                 // SpinBox.qss:66. TOKENIZE: SpinBoxErrorBorder
        constexpr const char* HoverImage = ":/SpinBox/hoverBackground.svg";         // SpinBox.qss:37
        constexpr const char* FocusedImage = ":/SpinBox/focusedBackground.svg";     // SpinBox.qss:52

        //! What the cascade is asked: the state of the box and where it is
        struct Look
        {
            bool m_enabled = true;
            bool m_hover = false;               //!< under the pointer (a disabled box is never)
            bool m_focus = false;
            bool m_error = false;               //!< enabled, with the HasError property
            bool m_vector = false;              //!< in a VectorElement: flush with the coordinate label (VectorInput.qss)
            bool m_colorPicker = false;         //!< in a color picker: no hover face, no buttons (ColorPicker.qss)
        };

        //! What the box draws: the margin around it (the box is the widget's rect inside it), the solid border inside the box, the
        //! radius of each corner, the fill and the image the fill carries
        struct Face
        {
            QMargins m_margin;                  //!< left, top, right, bottom
            QMargins m_border;                  //!< widths, in the same order
            QColor m_borderColor;
            int m_radius[4] = {};               //!< by Corner
            QColor m_fill;                      //!< invalid: no fill
            const char* m_image = nullptr;      //!< placed once at the right, centred vertically (`background-position: right`)
        };

        Face faceOf(const Look& look)
        {
            Face face;

            // At rest: BaseStyleSheet.qss:68-84 (fill, radius 2) and SpinBox.qss:9-20 (margin 2, no border)
            face.m_margin = QMargins(2, 2, 2, 2);
            face.m_fill = FieldFill;
            for (int& radius : face.m_radius)
            {
                radius = 2;
            }

            // :focus and :hover: a margin of 1 and a border of 1 where the margin 2 was, a radius of 3 (SpinBox.qss:22-31)
            if (look.m_hover || look.m_focus)
            {
                face.m_margin = QMargins(1, 1, 1, 1);
                face.m_border = QMargins(1, 1, 1, 1);
                for (int& radius : face.m_radius)
                {
                    radius = 3;
                }
            }
            // :hover (SpinBox.qss:33-40), then :focus, equally specific and later, over its border colour (:42-47)
            if (look.m_hover)
            {
                face.m_borderColor = HoverBorder;
                face.m_image = HoverImage;
            }
            if (look.m_focus)
            {
                face.m_borderColor = FocusBorder;
                face.m_fill = FocusFill;
            }
            if (look.m_hover && look.m_focus)
            {
                face.m_image = FocusedImage;                        // SpinBox.qss:49-55
            }

            // ColorPicker.qss:66-72: `background: none` under the pointer, the fill and the image both. A rule with no background
            // draws the default one, the base of the application palette (-qt-background-role: base, which Qt's default sheet gives
            // a spin box, qstylesheetstyle_default.cpp:323-343; the rule reads QGuiApplication::palette(), qstylesheetstyle.cpp:885)
            if (look.m_colorPicker && look.m_hover)
            {
                face.m_fill = QGuiApplication::palette().color(QPalette::Base);
                face.m_image = nullptr;
            }

            // VectorInput.qss:18-29: no margin and no border at the left, square corners there, in every state
            if (look.m_vector)
            {
                face.m_margin.setLeft(0);
                face.m_border.setLeft(0);
                face.m_radius[TopLeft] = 0;
                face.m_radius[BottomLeft] = 0;
            }

            // SpinBox.qss:57-67, as specific as :hover:focus and later: margin 0, a border of 2, radius 4 on every corner
            if (look.m_error)
            {
                face.m_margin = QMargins(0, 0, 0, 0);
                face.m_border = QMargins(2, 2, 2, 2);
                face.m_borderColor = ErrorBorder;
                for (int& radius : face.m_radius)
                {
                    radius = 4;
                }
            }

            // SpinBox.qss:69-74
            if (!look.m_enabled)
            {
                face.m_fill = DisabledFill;
            }
            return face;
        }

        // ------------------------------------------------------------------------------
        // Painting, as QRenderRule::drawRule paints a rule with a border (qstylesheetstyle.cpp:1348-1390, 1205-1286)
        // ------------------------------------------------------------------------------

        enum Side       // QCss::Edge
        {
            TopSide,
            RightSide,
            BottomSide,
            LeftSide,
        };

        //! qNormalizeRadii (qcssutil.cpp:279-294): a corner that does not fit is square
        void normalizeRadii(const QRect& rect, const int (&radius)[4], QSize (&out)[4])
        {
            for (int corner = 0; corner < 4; ++corner)
            {
                out[corner] = QSize(qMax(0, radius[corner]), qMax(0, radius[corner]));
            }
            if (out[TopLeft].width() + out[TopRight].width() > rect.width())
            {
                out[TopLeft] = out[TopRight] = QSize(0, 0);
            }
            if (out[BottomLeft].width() + out[BottomRight].width() > rect.width())
            {
                out[BottomLeft] = out[BottomRight] = QSize(0, 0);
            }
            if (out[TopLeft].height() + out[BottomLeft].height() > rect.height())
            {
                out[TopLeft] = out[BottomLeft] = QSize(0, 0);
            }
            if (out[TopRight].height() + out[BottomRight].height() > rect.height())
            {
                out[TopRight] = out[BottomRight] = QSize(0, 0);
            }
        }

        //! QRenderRule::borderClip (qstylesheetstyle.cpp:1288-1324): the path through the middle of the border. Empty: no corner is
        //! round, the rect itself is the box
        QPainterPath borderPath(const QRect& box, const Face& face)
        {
            QSize radii[4];
            normalizeRadii(box, face.m_radius, radii);
            if (radii[TopLeft].isNull() && radii[TopRight].isNull() && radii[BottomLeft].isNull() && radii[BottomRight].isNull())
            {
                return QPainterPath();
            }
            const QRectF rect(box);
            const qreal left = face.m_border.left();
            const qreal top = face.m_border.top();
            const qreal right = face.m_border.right();
            const qreal bottom = face.m_border.bottom();
            QPainterPath path;
            qreal curY = rect.y() + top / 2.0;
            path.moveTo(rect.x() + radii[TopLeft].width(), curY);
            path.lineTo(rect.right() - radii[TopRight].width(), curY);
            qreal curX = rect.right() - right / 2.0;
            path.arcTo(curX - 2 * radii[TopRight].width() + right, curY, radii[TopRight].width() * 2 - right, radii[TopRight].height() * 2 - top, 90, -90);

            path.lineTo(curX, rect.bottom() - radii[BottomRight].height());
            curY = rect.bottom() - bottom / 2.0;
            path.arcTo(curX - 2 * radii[BottomRight].width() + right, curY - 2 * radii[BottomRight].height() + bottom,
                radii[BottomRight].width() * 2 - right, radii[BottomRight].height() * 2 - bottom, 0, -90);

            path.lineTo(rect.x() + radii[BottomLeft].width(), curY);
            curX = rect.left() + left / 2.0;
            path.arcTo(curX, rect.bottom() - 2 * radii[BottomLeft].height() + bottom / 2.0,
                radii[BottomLeft].width() * 2 - left, radii[BottomLeft].height() * 2 - bottom, 270, -90);

            path.lineTo(curX, rect.top() + radii[TopLeft].height());
            path.arcTo(curX, rect.top() + top / 2.0, radii[TopLeft].width() * 2 - left, radii[TopLeft].height() * 2 - top, 180, -90);
            path.closeSubpath();
            return path;
        }

        //! qDrawRoundedCorners (qcssutil.cpp:44-146) for a solid edge: the two quarter-circle arcs at the ends of its straight part,
        //! stroked as wide as the edge. r1 and r2 are the radii at its first and second end
        void drawCorners(QPainter* painter, const QRectF& strip, const QSizeF& r1, const QSizeF& r2, Side side, const QColor& color)
        {
            const qreal x1 = strip.left();
            const qreal y1 = strip.top();
            const qreal x2 = strip.right();
            const qreal y2 = strip.bottom();
            const qreal width = (side == TopSide || side == BottomSide) ? y2 - y1 : x2 - x1;
            const qreal half = width / 2;
            QPen pen(color, width, Qt::SolidLine, Qt::SquareCap);       // square caps: no off-by-one errors
            painter->save();
            painter->setBrush(Qt::NoBrush);
            painter->setPen(pen);
            const auto arc = [&](qreal x, qreal y, const QSizeF& r, int start, int span)
            {
                painter->drawArc(QRectF(x, y, 2 * r.width() - width, 2 * r.height() - width), start * 16, span * 16);
            };
            switch (side)
            {
            case TopSide:
                if (!r1.isEmpty())
                {
                    arc(x1 - r1.width() + half, y1 + half, r1, 135, -45);
                }
                if (!r2.isEmpty())
                {
                    arc(x2 - r2.width() + half, y1 + half, r2, 45, 45);
                }
                break;
            case BottomSide:
                if (!r1.isEmpty())
                {
                    arc(x1 - r1.width() + half, y2 - 2 * r1.height() + half, r1, -90, -45);
                }
                if (!r2.isEmpty())
                {
                    arc(x2 - r2.width() + half, y2 - 2 * r2.height() + half, r2, -90, 45);
                }
                break;
            case LeftSide:
                if (!r1.isEmpty())
                {
                    arc(x1 + half, y1 - r1.height() + half, r1, 135, 45);
                }
                if (!r2.isEmpty())
                {
                    arc(x1 + half, y2 - r2.height() + half, r2, 180, 45);
                }
                break;
            case RightSide:
                if (!r1.isEmpty())
                {
                    arc(x2 - 2 * r1.width() + half, y1 - r1.height() + half, r1, 45, -45);
                }
                if (!r2.isEmpty())
                {
                    arc(x2 - 2 * r2.width() + half, y2 - r2.height() + half, r2, 315, 45);
                }
                break;
            }
            painter->restore();
        }

        //! QRenderRule::drawBorder (:1272-1286) over qDrawBorder (qcssutil.cpp:313-369) for solid edges of one colour, which each
        //! paint over the corner of the next (paintsOver, :296-311): the edges in increasing precedence, each a strip between the
        //! corners, antialiased, and arcs where a corner is round
        void drawBorder(QPainter* painter, const QRect& box, const Face& face)
        {
            if (face.m_border.isNull())
            {
                return;
            }
            QSize radii[4];
            normalizeRadii(box, face.m_radius, radii);
            const QRectF br(box);
            const QColor color = face.m_borderColor;
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            const auto strip = [&](qreal x1, qreal y1, qreal x2, qreal y2, Side side, const QSize& first, const QSize& second)
            {
                const QRectF edge(x1, y1, x2 - x1, y2 - y1);
                painter->drawRect(edge);
                if (first.width() || second.width())
                {
                    drawCorners(painter, edge, first, second, side, color);
                }
            };
            if (face.m_border.bottom() > 0)
            {
                strip(br.x() + radii[BottomLeft].width(), br.y() + br.height() - face.m_border.bottom(),
                    br.x() + br.width() - radii[BottomRight].width(), br.y() + br.height(),
                    BottomSide, radii[BottomLeft], radii[BottomRight]);
            }
            if (face.m_border.right() > 0)
            {
                strip(br.x() + br.width() - face.m_border.right(), br.y() + radii[TopRight].height(),
                    br.x() + br.width(), br.y() + br.height() - radii[BottomRight].height(),
                    RightSide, radii[TopRight], radii[BottomRight]);
            }
            if (face.m_border.left() > 0)
            {
                strip(br.x(), br.y() + radii[TopLeft].height(),
                    br.x() + face.m_border.left(), br.y() + br.height() - radii[BottomLeft].height(),
                    LeftSide, radii[TopLeft], radii[BottomLeft]);
            }
            if (face.m_border.top() > 0)
            {
                strip(br.x() + radii[TopLeft].width(), br.y(),
                    br.left() + br.width() - radii[TopRight].width(), br.y() + face.m_border.top(),
                    TopSide, radii[TopLeft], radii[TopRight]);
            }
            painter->restore();
        }

        //! QRenderRule::drawBackgroundImage (:1205-1259) for an image that does not repeat: the file at its natural size
        //! (loadPixmap, :6537-6560), placed in the padding box by `background-position: right` (the position the sheet reads:
        //! right and, as only one is given, centred vertically, qcssparser.cpp alignmentValue), clipped to the border box
        void drawImage(QPainter* painter, const QRect& box, const Face& face, const QPainterPath& path)
        {
            const QPixmap pixmap = Style::cachedPixmap(QString::fromLatin1(face.m_image));
            if (pixmap.isNull())
            {
                return;
            }
            const QRect padding = box.marginsRemoved(face.m_border);
            const QSize size = pixmap.size() / pixmap.devicePixelRatio();
            const QRect aligned = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignVCenter, size, padding);
            const QRect visible = aligned.intersected(padding);

            painter->save();
            if (!path.isEmpty())
            {
                painter->setClipPath(path, Qt::IntersectClip);
            }
            painter->setClipRect(box, Qt::IntersectClip);
            painter->drawPixmap(visible.x(), visible.y(), pixmap, visible.x() - aligned.x(), visible.y() - aligned.y(), pixmap.width(), pixmap.height());
            painter->restore();
        }

        //! QRenderRule::drawRule (:1386-1390): the fill over the border box, rounded (a path, antialiased: :1348-1370) where the
        //! rule has a radius, then the image, then the border over both
        void paintBox(QPainter* painter, const QRect& rect, const Face& face)
        {
            const QRect box = rect.marginsRemoved(face.m_margin);
            const QPainterPath path = borderPath(box, face);
            if (face.m_fill.isValid())
            {
                if (path.isEmpty())
                {
                    painter->fillRect(box, face.m_fill);
                }
                else
                {
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->fillPath(path, face.m_fill);
                    painter->restore();
                }
            }
            if (face.m_image)
            {
                drawImage(painter, box, face, path);
            }
            drawBorder(painter, box, face);
        }

        // ------------------------------------------------------------------------------
        // The glyphs of the buttons: SpinBox.qss:76-176
        // ------------------------------------------------------------------------------

        constexpr QSize GlyphSize(6, 10);               // SpinBox.qss:81-82 (::up-button, ::down-button)
        constexpr int UpGlyphInset = 2;                 // SpinBox.qss:96 `right: 2px`. TOKENIZE: SpinBoxGlyphInset
        constexpr int DownGlyphInset = 8;               // SpinBox.qss:88 `right: 8px`
        constexpr int PaddingRight = 12;                // SpinBox.qss:16, the room of the buttons (padding-left is 0, :17)

        //! The text of a bool property as the sheet's attribute selector reads it ([name=true]): a property the watcher never set
        //! matches neither value
        bool propertyIs(const QWidget* widget, const char* name, bool value)
        {
            const QVariant property = widget->property(name);
            return property.isValid() && property.toBool() == value;
        }

        //! The image of the up (`up` true) or the down button: of the rules whose attributes hold, the most specific (one attribute,
        //! or three for dragging and focused) and, equally specific, the later (SpinBox.qss:85-176). The down button is the up one
        //! with the roles of the limits and of the direction of the value swapped
        const char* glyphImage(const QWidget* spinBox, bool up)
        {
            const char* const limitName = up ? "SpinBoxMaxReached" : "SpinBoxMinReached";      // the limit this button leads to
            const char* const directionName = up ? "SpinBoxValueIncreasing" : "SpinBoxValueDecreasing";
            const auto is = [&](const char* name, bool value) { return propertyIs(spinBox, name, value); };

            // :166-176 focused, after :150-160 dragging: the same three attributes, the button not at its limit and the value not
            // moving its way
            const bool unblocked = is(limitName, false) && is(directionName, false);
            if (is("SpinBoxFocused", true) && unblocked)
            {
                return up ? ":/SpinBox/arrowRightFocused.svg" : ":/SpinBox/arrowLeftFocused.svg";
            }
            if (is("SpinBoxDragging", true) && unblocked)
            {
                return up ? ":/SpinBox/arrowRightDragging.svg" : ":/SpinBox/arrowLeftDragging.svg";
            }
            // :102-148, one attribute each, in the order of the sheet: increasing, decreasing, minimum reached, maximum reached
            if (is("SpinBoxMaxReached", true))
            {
                return up ? ":/SpinBox/arrowRightMaxReached.svg" : ":/SpinBox/arrowLeftMaxReached.svg";
            }
            if (is("SpinBoxMinReached", true))
            {
                return up ? ":/SpinBox/arrowRightMinReached.svg" : ":/SpinBox/arrowLeftMinReached.svg";
            }
            if (is("SpinBoxValueDecreasing", true))
            {
                return up ? ":/SpinBox/arrowRightDecreasing.svg" : ":/SpinBox/arrowLeftDecreasing.svg";
            }
            if (is("SpinBoxValueIncreasing", true))
            {
                return up ? ":/SpinBox/arrowRightIncreasing.svg" : ":/SpinBox/arrowLeftIncreasing.svg";
            }
            return up ? ":/SpinBox/arrowRightHover.svg" : ":/SpinBox/arrowLeftHover.svg";      // :95-98, :88-91
        }

        //! A button's glyph: the image, centred (the rule's `image` alignment) in the rect the sheet's position gave the button
        //! (positionRect, qstylesheetstyle.cpp:2395-2433): the box of the glyph, right and centred vertically in the padding box of
        //! the rule (the box less its margin and border), then moved to the left by the inset. Left to right, as every capture is: the
        //! sheet read the option's direction (positionRect), UNVERIFIED for a right to left layout.
        void paintGlyph(QPainter* painter, const QRect& rect, const Face& face, const QWidget* widget, bool up)
        {
            const QRect padding = rect.marginsRemoved(face.m_margin + face.m_border);
            QRect glyph = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignVCenter, GlyphSize, padding);
            glyph.translate(-(up ? UpGlyphInset : DownGlyphInset), 0);
            QIcon(QString::fromLatin1(glyphImage(widget, up))).paint(painter, glyph, Qt::AlignCenter);
        }

        //! Whether the widget is a spin box of the classes the sheet's rules name (QSpinBox and QDoubleSpinBox, and what derives)
        bool isSpinBox(const QWidget* widget)
        {
            return qobject_cast<const QSpinBox*>(widget) || qobject_cast<const QDoubleSpinBox*>(widget);
        }
    } // namespace SpinBoxElementInternal

    // ----------------------------------------------------------------------------------
    // Drawing
    // ----------------------------------------------------------------------------------

    bool SpinBoxElement::draw(const QStyleOptionSpinBox* option, QPainter* painter, const QWidget* widget)
    {
        using namespace SpinBoxElementInternal;

        if (!option || !widget || !isSpinBox(widget))
        {
            return false;
        }

        // The state the pseudo-classes read (qstylesheetstyle.cpp:1757-1804): :hover needs :enabled
        Look look;
        look.m_enabled = option->state.testFlag(QStyle::State_Enabled);
        look.m_hover = look.m_enabled && option->state.testFlag(QStyle::State_MouseOver);
        look.m_focus = option->state.testFlag(QStyle::State_HasFocus);
        look.m_error = look.m_enabled && widget->property(HasError).toBool();
        look.m_vector = TextElement::withinClass(widget, "AzQtComponents::VectorElement");
        look.m_colorPicker = TextElement::withinClass(widget, "AzQtComponents::ColorPicker");

        const Face face = faceOf(look);
        paintBox(painter, option->rect, face);

        // The buttons: each is drawn when the option asks for it, as the sheet style does (qstylesheetstyle.cpp:3123, :3137): a spin
        // box without buttons (StyledSpinBox.cpp:109, QAbstractSpinBox::NoButtons) leaves them out of its subcontrols. Both are drawn
        // in every state (the :disabled rule changes no image), but not in a color picker, whose buttons are 0px wide
        // (ColorPicker.qss:74-81)
        if (!look.m_colorPicker)
        {
            if (option->subControls.testFlag(QStyle::SC_SpinBoxUp))
            {
                paintGlyph(painter, option->rect, face, widget, true);
            }
            if (option->subControls.testFlag(QStyle::SC_SpinBoxDown))
            {
                paintGlyph(painter, option->rect, face, widget, false);
            }
        }
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Size
    // ----------------------------------------------------------------------------------

    int SpinBoxElement::extraWidth(const QWidget* widget)
    {
        using namespace SpinBoxElementInternal;

        if (!widget || !isSpinBox(widget))
        {
            return -1;
        }

        Look look;
        look.m_vector = TextElement::withinClass(widget, "AzQtComponents::VectorElement");
        look.m_colorPicker = TextElement::withinClass(widget, "AzQtComponents::ColorPicker");

        // The margin and the border of the faces of every state add up to the same, so the box does not change the size of the hint
        const Face face = faceOf(look);
        const QMargins box = face.m_margin + face.m_border;
        return (look.m_colorPicker ? 0 : GlyphSize.width()) + box.left() + box.right() + PaddingRight;
    }
} // namespace AzQtComponents
