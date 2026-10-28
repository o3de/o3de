/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/SpinBoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/IconUtilities.h>

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
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace SpinBoxElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- The box at rest: the entry rule of BaseStyleSheet.qss under SpinBox.qss
        const QColor FieldFill(0xCC, 0xCC, 0xCC);                   // BaseStyleSheet.qss:76 $fill.field.idle.Alt1
        constexpr int FieldRadius = 2;                              // BaseStyleSheet.qss:79 $radius.field.idle.all
        constexpr QMargins FieldMargin(2, 2, 2, 2);                 // SpinBox.qss:13 $space.field.idle.margin.Alt1
        constexpr int ButtonsPadding = 12;                          // SpinBox.qss:16 $space.field.idle.padding-right.Alt1

        // ---- Hovered and focused (SpinBox.qss:22-55)
        constexpr QMargins HoverMargin(1, 1, 1, 1);                 // SpinBox.qss:27 $space.field.hover.margin.Alt1
        constexpr QMargins FocusMargin(1, 1, 1, 1);                 // SpinBox.qss:27 $space.field.focus.margin.Alt1
        constexpr int HoverLineWidth = 1;                           // SpinBox.qss:28 $line.field.hover.width.Alt1
        constexpr int FocusLineWidth = 1;                           // SpinBox.qss:28 $line.field.focus.width.Alt1
        constexpr int HoverRadius = 3;                              // SpinBox.qss:30 $radius.field.hover.all
        constexpr int FocusRadius = 3;                              // SpinBox.qss:30 $radius.field.focus.all
        const QColor HoverLine(0x22, 0x22, 0x22);                   // SpinBox.qss:36 $line.field.hover.color.BG
        constexpr const char* HoverImage = ":/SpinBox/hoverBackground.svg";         // SpinBox.qss:37 $asset.field.hover
        const QColor FocusLine(0x94, 0xD2, 0xFF);                   // SpinBox.qss:45 $line.field.focus.color.Alt1
        const QColor FocusFill(0xFF, 0xFF, 0xFF);                   // SpinBox.qss:46 $fill.field.focus.Alt1
        constexpr const char* HoverFocusImage = ":/SpinBox/focusedBackground.svg";  // SpinBox.qss:52 $asset.field.hover+focus

        // ---- With an error, and disabled
        constexpr QMargins ErrorMargin(0, 0, 0, 0);                 // SpinBox.qss:62 $space.field.HasError
        constexpr int ErrorLineWidth = 2;                           // SpinBox.qss:63 $line.field.HasError.width
        constexpr int ErrorRadius = 4;                              // SpinBox.qss:65 $radius.field.HasError
        const QColor ErrorLine(0xE2, 0x52, 0x43);                   // SpinBox.qss:66 $line.field.HasError.color
        const QColor DisabledFill(0x66, 0x66, 0x66);                // SpinBox.qss:72 $fill.field.disabled

        // ---- In a vector element: flush with the coordinate label, in every state (one rule for its three selectors)
        // VectorInput.qss:25 $space.field.idle.margin-left.Alt1 $space.field.hover.margin-left $space.field.focus.margin-left
        constexpr int VectorMarginLeft = 0;
        // VectorInput.qss:26 $line.field.idle.width.Alt1 $line.field.hover.width.Alt2 $line.field.focus.width.Alt2
        constexpr int VectorLineLeft = 0;
        // VectorInput.qss:27-28 $radius.field.idle.top-left.Alt1 $radius.field.idle.bottom-left.Alt1, and the same of :hover, :focus
        constexpr int VectorLeftRadius = 0;

        // ---- The glyphs of the buttons: their box, their inset from the right, and the images of each state (up, down)
        constexpr QSize GlyphSize(6, 10);           // SpinBox.qss:81-82 $size.field-button.idle.width.Alt1 $size.field-button.idle.height
        constexpr int UpGlyphInset = 2;                             // SpinBox.qss:96 $other.field-button.idle.right.Alt2
        constexpr int DownGlyphInset = 8;                           // SpinBox.qss:88 $other.field-button.idle.right.Alt1

        struct Glyphs
        {
            const char* m_up;
            const char* m_down;
        };
        // SpinBox.qss:98, :90 $asset.field-button.idle.Alt2 $asset.field-button.idle.Alt1
        constexpr Glyphs HoverGlyphs{ ":/SpinBox/arrowRightHover.svg", ":/SpinBox/arrowLeftHover.svg" };
        // SpinBox.qss:105, :111 $asset.field-button.SpinBoxValueIncreasing.Alt1 $asset.field-button.SpinBoxValueIncreasing.Alt2
        constexpr Glyphs IncreasingGlyphs{ ":/SpinBox/arrowRightIncreasing.svg", ":/SpinBox/arrowLeftIncreasing.svg" };
        // SpinBox.qss:117, :123 $asset.field-button.SpinBoxValueDecreasing.Alt1 $asset.field-button.SpinBoxValueDecreasing.Alt2
        constexpr Glyphs DecreasingGlyphs{ ":/SpinBox/arrowRightDecreasing.svg", ":/SpinBox/arrowLeftDecreasing.svg" };
        // SpinBox.qss:129, :135 $asset.field-button.SpinBoxMinReached.Alt1 $asset.field-button.SpinBoxMinReached.Alt2
        constexpr Glyphs MinReachedGlyphs{ ":/SpinBox/arrowRightMinReached.svg", ":/SpinBox/arrowLeftMinReached.svg" };
        // SpinBox.qss:141, :147 $asset.field-button.SpinBoxMaxReached.Alt1 $asset.field-button.SpinBoxMaxReached.Alt2
        constexpr Glyphs MaxReachedGlyphs{ ":/SpinBox/arrowRightMaxReached.svg", ":/SpinBox/arrowLeftMaxReached.svg" };
        // SpinBox.qss:153, :159 $asset.field-button.SpinBoxDragging.Alt1 $asset.field-button.SpinBoxDragging.Alt2
        constexpr Glyphs DraggingGlyphs{ ":/SpinBox/arrowRightDragging.svg", ":/SpinBox/arrowLeftDragging.svg" };
        // SpinBox.qss:169, :175 $asset.field-button.SpinBoxFocused.Alt1 $asset.field-button.SpinBoxFocused.Alt2
        constexpr Glyphs FocusedGlyphs{ ":/SpinBox/arrowRightFocused.svg", ":/SpinBox/arrowLeftFocused.svg" };

        // ------------------------------------------------------------------------------
        // Helpers: the face of the box
        // ------------------------------------------------------------------------------

        //! What the cascade is asked: the state of the box and where it is
        struct Look
        {
            bool m_enabled = true;
            bool m_hover = false;               //!< under the pointer (a disabled box is never)
            bool m_focus = false;
            bool m_error = false;               //!< enabled, with the HasError property
            bool m_vector = false;              //!< in a VectorElement: flush with the coordinate label
            bool m_colorPicker = false;         //!< in a color picker: no hover face, no buttons
        };

        //! What the box draws: the margin around it (the box is the widget's rect inside it), the solid border inside the box, the
        //! radius of each corner, the fill and the image the fill carries
        struct Face
        {
            QMargins m_margin;                  //!< left, top, right, bottom
            QMargins m_border;                  //!< widths, in the same order
            QColor m_borderColor;
            BoxElement::Radii m_radii;
            QColor m_fill;                      //!< invalid: no fill
            const char* m_image = nullptr;      //!< placed once at the right, centered vertically (`background-position: right`)
        };

        //! The cascade, by specificity and, for equal ones, the later: BaseStyleSheet.qss's entry rule, SpinBox.qss's rule, `:focus,
        //! :hover`, `:hover`, `:focus`, `:hover:focus`, `:enabled[HasError=true]`, `:disabled`; VectorInput.qss over them in a vector
        //! element, ColorPicker.qss in a color picker
        Face faceOf(const Look& look)
        {
            Face face;
            face.m_margin = FieldMargin;
            face.m_fill = FieldFill;
            face.m_radii = { FieldRadius, FieldRadius, FieldRadius, FieldRadius };

            // `:focus, :hover`: a margin of 1 and a border of 1 where the margin 2 was, a radius of 3
            if (look.m_hover)
            {
                face.m_margin = HoverMargin;
                face.m_border = QMargins(HoverLineWidth, HoverLineWidth, HoverLineWidth, HoverLineWidth);
                face.m_radii = { HoverRadius, HoverRadius, HoverRadius, HoverRadius };
                face.m_borderColor = HoverLine;
                face.m_image = HoverImage;
            }
            // :focus, equally specific and later than :hover, over its border colour
            if (look.m_focus)
            {
                face.m_margin = FocusMargin;
                face.m_border = QMargins(FocusLineWidth, FocusLineWidth, FocusLineWidth, FocusLineWidth);
                face.m_radii = { FocusRadius, FocusRadius, FocusRadius, FocusRadius };
                face.m_borderColor = FocusLine;
                face.m_fill = FocusFill;
            }
            if (look.m_hover && look.m_focus)
            {
                face.m_image = HoverFocusImage;
            }

            // ColorPicker.qss: `background: none` under the pointer, the fill and the image both. A rule with no background draws the
            // default one, the application palette's Base (Qt's default sheet gives a spin box `-qt-background-role: base`)
            if (look.m_colorPicker && look.m_hover)
            {
                face.m_fill = QGuiApplication::palette().color(QPalette::Base);
                face.m_image = nullptr;
            }

            // VectorInput.qss: no margin and no border at the left, square corners there, in every state
            if (look.m_vector)
            {
                face.m_margin.setLeft(VectorMarginLeft);
                face.m_border.setLeft(VectorLineLeft);
                face.m_radii.m_topLeft = VectorLeftRadius;
                face.m_radii.m_bottomLeft = VectorLeftRadius;
            }

            // `:enabled[HasError=true]`, as specific as :hover:focus and later
            if (look.m_error)
            {
                face.m_margin = ErrorMargin;
                face.m_border = QMargins(ErrorLineWidth, ErrorLineWidth, ErrorLineWidth, ErrorLineWidth);
                face.m_borderColor = ErrorLine;
                face.m_radii = { ErrorRadius, ErrorRadius, ErrorRadius, ErrorRadius };
            }

            if (!look.m_enabled)
            {
                face.m_fill = DisabledFill;
            }
            return face;
        }

        // ------------------------------------------------------------------------------
        // Helpers: painting, as QRenderRule::drawRule paints a rule with a border
        // ------------------------------------------------------------------------------

        BoxElement::Edges edgesOf(const Face& face)
        {
            return { { face.m_border.top(), face.m_borderColor }, { face.m_border.right(), face.m_borderColor },
                { face.m_border.bottom(), face.m_borderColor }, { face.m_border.left(), face.m_borderColor } };
        }

        //! QRenderRule::drawBackgroundImage for an image that does not repeat: the file at its natural size, placed in the padding box
        //! by `background-position: right` (right, and centered vertically), clipped to the border box and its rounded path
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
            painter->drawPixmap(visible.x(), visible.y(), pixmap, visible.x() - aligned.x(), visible.y() - aligned.y(), pixmap.width(),
                pixmap.height());
            painter->restore();
        }

        //! The fill over the border box, the image, then the border over both
        void paintBox(QPainter* painter, const QRect& rect, const Face& face)
        {
            const QRect box = rect.marginsRemoved(face.m_margin);
            const BoxElement::Edges edges = edgesOf(face);
            if (face.m_fill.isValid())
            {
                BoxElement::drawFill(painter, box, face.m_fill, edges, face.m_radii);
            }
            if (face.m_image)
            {
                drawImage(painter, box, face, BoxElement::borderPath(box, edges, face.m_radii));
            }
            BoxElement::drawEdges(painter, box, edges, face.m_radii);
        }

        // ------------------------------------------------------------------------------
        // Helpers: the glyphs of the buttons
        // ------------------------------------------------------------------------------

        //! A bool property as the sheet's attribute selector reads it ([name=true]): a property the watcher never set matches neither
        bool propertyIs(const QWidget* widget, const char* name, bool value)
        {
            const QVariant property = widget->property(name);
            return property.isValid() && property.toBool() == value;
        }

        //! The image of the up or the down button: of the rules whose attributes hold, the most specific, and of equal ones the later.
        //! The down button is the up one with the limits and the direction of the value swapped.
        const char* glyphImage(const QWidget* spinBox, bool up)
        {
            const char* const limitName = up ? "SpinBoxMaxReached" : "SpinBoxMinReached";      // the limit this button leads to
            const char* const directionName = up ? "SpinBoxValueIncreasing" : "SpinBoxValueDecreasing";
            const auto is = [&](const char* name, bool value) { return propertyIs(spinBox, name, value); };
            const auto pick = [up](const Glyphs& glyphs) { return up ? glyphs.m_up : glyphs.m_down; };

            // Focused, after dragging: the same three attributes, the button not at its limit and the value not moving its way
            const bool unblocked = is(limitName, false) && is(directionName, false);
            if (is("SpinBoxFocused", true) && unblocked)
            {
                return pick(FocusedGlyphs);
            }
            if (is("SpinBoxDragging", true) && unblocked)
            {
                return pick(DraggingGlyphs);
            }
            // One attribute each, in the order of the sheet
            if (is("SpinBoxMaxReached", true))
            {
                return pick(MaxReachedGlyphs);
            }
            if (is("SpinBoxMinReached", true))
            {
                return pick(MinReachedGlyphs);
            }
            if (is("SpinBoxValueDecreasing", true))
            {
                return pick(DecreasingGlyphs);
            }
            if (is("SpinBoxValueIncreasing", true))
            {
                return pick(IncreasingGlyphs);
            }
            return pick(HoverGlyphs);
        }

        //! A button's glyph: the image centered in the rect the sheet's position gave the button (QStyleSheetStyle::positionRect): the
        //! glyph's box at the right of the padding box, centered vertically, moved left by the inset (left to right)
        void paintGlyph(QPainter* painter, const QRect& rect, const Face& face, const QWidget* widget, bool up)
        {
            const QRect padding = rect.marginsRemoved(face.m_margin + face.m_border);
            QRect glyph = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignVCenter, GlyphSize, padding);
            glyph.translate(-(up ? UpGlyphInset : DownGlyphInset), 0);
            CachedIcon(QString::fromLatin1(glyphImage(widget, up))).paint(painter, glyph, Qt::AlignCenter);
        }

        //! A spin box of the classes the sheet's rules name: QSpinBox and QDoubleSpinBox, and what derives from them
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

        // The state the pseudo-classes read: :hover needs :enabled
        Look look;
        look.m_enabled = option->state.testFlag(QStyle::State_Enabled);
        look.m_hover = look.m_enabled && option->state.testFlag(QStyle::State_MouseOver);
        look.m_focus = option->state.testFlag(QStyle::State_HasFocus);
        look.m_error = look.m_enabled && widget->property(HasError).toBool();
        look.m_vector = Selectors::withinClass(widget, "AzQtComponents::VectorElement");
        look.m_colorPicker = Selectors::withinClass(widget, "AzQtComponents::ColorPicker");

        const Face face = faceOf(look);
        paintBox(painter, option->rect, face);

        // Each button is drawn when the option asks for it, as the sheet does: a spin box without buttons leaves them out of its
        // subcontrols. A color picker's buttons are 0px wide (ColorPicker.qss), and none is drawn.
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
    // Geometry
    // ----------------------------------------------------------------------------------

    int SpinBoxElement::extraWidth(const QWidget* widget)
    {
        using namespace SpinBoxElementInternal;

        if (!widget || !isSpinBox(widget))
        {
            return -1;
        }

        Look look;
        look.m_vector = Selectors::withinClass(widget, "AzQtComponents::VectorElement");
        look.m_colorPicker = Selectors::withinClass(widget, "AzQtComponents::ColorPicker");

        // The margin and the border of every state's face add up to the same, so the box does not change the hint's size
        const Face face = faceOf(look);
        const QMargins box = face.m_margin + face.m_border;
        return (look.m_colorPicker ? 0 : GlyphSize.width()) + box.left() + box.right() + ButtonsPadding;
    }
} // namespace AzQtComponents
