/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/optional.h>
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <QBrush>
#include <QColor>
#include <QMargins>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QStyle>
#include <QVariant>
#endif

class QPainter;
class QPainterPath;
class QStyleOption;
class QStyleOptionButton;
class QStyleOptionComboBox;
class QStyleOptionToolButton;

namespace AzQtComponents
{
    //! The box a widget's rules give it, in every state: margin, border and padding around the contents, the limits of its size,
    //! and what each state draws (fill, edges, radius, images). It is Qt's style sheet box model (QRenderRule::boxSize): the rect
    //! less the margin is the border box, less the border the padding box, less the padding the contents. Qt reads the contents rect
    //! through the style and derives frame widths, contents margins, viewports and size hints from it; the width, height and limits
    //! are the contents', and the widget's own limits are those grown by the box (QStyleSheetStyle::setGeometry).
    //!
    //! A rule is computed for each state (QStyleSheetStyle::renderRule), and a state may change the box: a face may give its own
    //! margin, radius and spacing. What is drawn, the rects and the size a widget asks for are those of the option's state; only
    //! the limits a widget gets at polish are those of the idle face, the state Qt polished it in. Elements and the strings of their
    //! images have static storage and are written with designated initializers, in the order of the members; the tables have a
    //! fixed capacity (6 state faces, 3 properties).
    struct AZ_QT_COMPONENTS_API BoxElement
    {
        //! One side of a box: a solid line; a width of 0 or no colour draws nothing
        struct Edge
        {
            int m_width = 0;
            QColor m_color;
        };

        struct Edges
        {
            Edge m_top;
            Edge m_right;
            Edge m_bottom;
            Edge m_left;
        };

        //! An integer member no rule sets (the sheet's own -1)
        static constexpr int Unset = -1;

        //! background-origin: the box a background image is laid out in
        enum class Origin
        {
            Padding,
            Content,
        };

        //! The pseudo-states a face stands for
        enum State : unsigned
        {
            Hover = 1,                          //!< enabled and under the pointer
            Pressed = 2,                        //!< QStyle::State_Sunken
            Focus = 4,                          //!< QStyle::State_HasFocus
            Checked = 8,                        //!< QStyle::State_On (a combo box while its popup is shown)
            Disabled = 16,                      //!< not QStyle::State_Enabled
        };

        //! What the box draws in one state, and the parts of the box the rule of that state may change
        struct Face
        {
            QBrush m_fill;                      //!< no brush: no fill; a gradient is a brush (vertical)
            Edges m_edges;                      //!< the declared solid edges; an edge with no colour is the style's own or none
            QColor m_text;                      //!< the label of a push button; invalid: the widget's own
            const char* m_image = "";           //!< background image resource, once, at its natural size ("": none)
            AZStd::optional<QMargins> m_margin; //!< unset: the element's m_margin
            int m_radius = Unset;               //!< border-radius of the four corners; Unset: the element's m_radius
            int m_spacing = Unset;              //!< the label spacing; Unset: the element's m_spacing
            //! border-image: url(...) 0 0 0 0 stretch stretch ("": none): the image stretched over the border box in place of the
            //! declared border, clipped to the radius (QRenderRule::drawBorderImage); the border is then the rule's, not the style's
            const char* m_borderImage = "";
        };

        //! A face that applies while all of its states hold, as a compound pseudo-class does (:hover:focus is Hover | Focus). Of the
        //! entries whose states all hold, the last is the face, so the order of the entries reproduces the cascade. A face is
        //! complete, except for its members that say "unset", which are the element's.
        struct StateFace
        {
            unsigned m_states = 0;              //!< State flags (0: unused entry)
            Face m_face;
        };

        //! A qproperty-* assignment of the sheet, made at each polish; a string becomes an icon where the property is one
        struct Property
        {
            const char* m_name = nullptr;       //!< nullptr: unused entry
            QVariant m_value;
        };

        const char* m_name = "";                //!< Scope.Box.Look

        // ---- The box: lengths in px; margin and padding left, top, right, bottom (a negative side pulls the contents out)
        QMargins m_margin;
        QMargins m_padding;
        //! `spacing`: the label spacing of a check box or radio button, and the space after a combo box label's icon; Unset: the
        //! style's own
        int m_spacing = Unset;
        //! The winning rules declare a margin or a padding, even a 0 (QRenderRule::hasBox), so Qt takes the box from the rule.
        //! Implied by a non-zero side, a spacing or a face's margin.
        bool m_hasBox = false;
        //! The border is the style's own (Qt's default sheet gives QFrame, push and tool buttons `border: native`); false where the
        //! rules give the border a style of their own: a border, border: none, a radius or a border image
        bool m_nativeBorder = true;
        int m_radius = 0;                       //!< border-radius of the four corners

        // ---- The contents' width, height and limits (-1: unset)
        QSize m_size;
        QSize m_minSize;
        QSize m_maxSize;

        // ---- The drop-down of a combo box, the same in every state
        int m_dropDownWidth = Unset;            //!< the width of its contents; Unset: DropDownWidth
        QMargins m_dropDownPadding;             //!< the arrow is centered in what the padding leaves

        // ---- The faces: idle, then the state entries in the order of the cascade
        Face m_idle;
        StateFace m_stateFaces[6];

        // ---- Where the background image sits, in every state
        Qt::Alignment m_imageAlign = Qt::AlignTop | Qt::AlignLeft;     //!< background-position (Qt default)
        Origin m_imageOrigin = Origin::Padding;                        //!< background-origin (Qt default)

        Property m_properties[3];

        //! `text-align`: the push button's rule has a position, and the sheet lays the label out itself, the icon and the text in one
        //! group in this alignment (QStyleSheetStyle::drawControl, CE_PushButtonLabel). 0: the base style's label.
        Qt::Alignment m_textAlign;

        // ---- Geometry, for the state of an option (no option: Disabled alone, as QStyleSheetStyle::renderRule reads it)
        bool hasNativeBorder(const QStyleOption* option) const;     //!< the style's border draws in this state
        bool ownsBox(const QStyleOption* option) const;             //!< Qt takes the box from the rule: a margin, a padding or a border
        QRect borderRect(const QStyleOption* option) const;         //!< option->rect inside the margin
        QRect contentsRect(const QStyleOption* option, const QWidget* widget) const;    //!< option->rect inside margin, border, padding
        //! The contents plus margin, border and padding (QStyleSheetStyle::sizeFromContents)
        QSize boxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const;
        QSize adjustSize(const QSize& contents) const;              //!< the contents within the width, height and limits
        //! The label spacing: the face's, else the element's; Unset: the style's. Qt reads it with the option for the size of the
        //! button, and with no option for where its label sits.
        int spacing(const QStyleOption* option) const;

        // ---- Drawing
        const Face& face(const QStyleOption* option) const;
        //! The rule of the state in option->rect (QRenderRule::drawRule): fill, image, then border or border image. False: none.
        bool draw(QPainter* painter, const QStyleOption* option, const QWidget* widget) const;
        //! The base style draws this face (QRenderRule::baseStyleCanDraw): no image, and no fill or one Qt's default sheet leaves to
        //! the base style for the class (a line edit's or spin box's colour, a combo box's colour or gradient)
        static bool baseStyleCanDraw(const Face& face, const QWidget* styled);
        //! The palette the base style draws a native frame with: the fill as Window and Base, and the shades of a solid one
        static void configurePalette(QPalette& palette, const Face& face);
        //! The palette of a button-like control's label (QRenderRule::configurePalette): the fill as `background` (NoRole: not
        //! applied), the state's text colour as `foreground`
        static void configurePalette(QPalette& palette, const Face& face, QPalette::ColorRole foreground, QPalette::ColorRole background);
        //! qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 top, stop: 1 bottom)
        static QBrush vertical(const QColor& top, const QColor& bottom);
        //! The same solid edge on the four sides
        static Edges border(int width, const QColor& color);

        //! The radius of each corner of a border box; a corner that does not fit its box is square (qNormalizeRadii)
        struct Radii
        {
            int m_topLeft = 0;
            int m_topRight = 0;
            int m_bottomRight = 0;
            int m_bottomLeft = 0;
        };

        //! QRenderRule::drawFrame of a rule with no image, in its border box: drawFill, then drawEdges. draw() is this for an element.
        static void drawFrame(QPainter* painter, const QRect& borderBox, const QBrush& fill, const Edges& edges, const Radii& radii);
        //! The fill in the border box (QRenderRule::drawBackground): a fillRect where no corner is round, else the antialiased fill of
        //! borderPath. A fill that shows nothing is not drawn.
        static void drawFill(QPainter* painter, const QRect& borderBox, const QBrush& fill, const Edges& edges, const Radii& radii);
        //! The edges as qDrawBorder draws them, antialiased; an edge with no colour is not drawn
        static void drawEdges(QPainter* painter, const QRect& borderBox, const Edges& edges, const Radii& radii);
        //! The path through the middle of the border (QRenderRule::borderClip), radii normalized; empty where no corner is round
        static QPainterPath borderPath(const QRect& borderBox, const Edges& edges, const Radii& radii);

        // ---- Push buttons
        //! CE_PushButtonLabel of a rule with `text-align`, laid out as QStyleSheetStyle lays it out. False: the base style draws it.
        bool drawPushButtonLabel(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const;

        // ---- Tool buttons
        //! CC_ToolButton of a border of its own: the rule over the whole rect, the label in its contents rect, and the base style's
        //! drop-down and menu indicator. False: the style's own border, which the style draws.
        bool drawToolButton(const QStyle* style, const QStyleOptionToolButton* option, QPainter* painter, const QWidget* widget) const;
        //! SC_ToolButton inside the margin, and SC_ToolButtonMenu right and top of it, menuWidth wide (PM_MenuButtonIndicator)
        QRect toolButtonRect(const QStyleOptionToolButton* option, QStyle::SubControl subControl, int menuWidth) const;

        // ---- Combo boxes
        static constexpr int DropDownWidth = 16;        //!< QStyleSheetStyle::defaultSize $size.drop-down.idle.width
        //! CT_ComboBox: the contents with the drop-down and the base style's label space, the box around it all
        QSize comboBoxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const;
        //! SC_ComboBoxArrow: the drop-down at the right and top of the padding box
        QRect dropDownRect(const QStyleOption* option, const QWidget* widget) const;
        //! SC_ComboBoxEditField: the contents rect less the drop-down
        QRect editFieldRect(const QStyleOption* option, const QWidget* widget) const;
        //! The arrow the drop-down shows (ComboBox.qss `::down-arrow`), centered in the drop-down's contents with its right margin
        QRect arrowRect(const QStyleOption* option, const QWidget* widget) const;
        //! CE_ComboBoxLabel of a rule that draws: the icon and the text in the edit field, the text after the state's spacing.
        //! False: the base style draws the label.
        bool drawComboBoxLabel(const QStyle* style, const QStyleOptionComboBox* option, QPainter* painter, const QWidget* widget) const;

        // ---- Check boxes and radio buttons
        //! CE_CheckBox and CE_RadioButton: the rule over the whole rect, then the indicator element draws the indicator and the label
        //! in the contents rect. False: the widget has no indicator element.
        bool drawIndicatorButton(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const;

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const BoxElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish, after the base style and the components (as QStyleSheetStyle::polish set geometry and properties last):
        //! finds the widget's element (none for a widget containerOf gives another), keeps it, and sets its limits, properties,
        //! hover and styled background
        static void resolve(QWidget* widget);
        //! The kept element; none before the widget is polished
        static const BoxElement* forWidget(const QWidget* widget);
        //! PE_Widget where no element draws: the background role fills a widget whose element took its autofill and draws nothing,
        //! and the viewport of a scroll area whose element has no background in this state
        static bool fillsBackgroundRole(const QWidget* widget, const QStyleOption* option);
        //! The widget whose element draws this one: the combo box or spin box a line edit sits in, the scroll area a viewport
        //! belongs to (containerWidget, qstylesheetstyle.cpp), else the widget itself; such a widget has no element of its own
        static const QWidget* containerOf(const QWidget* widget);
    };
} // namespace AzQtComponents
