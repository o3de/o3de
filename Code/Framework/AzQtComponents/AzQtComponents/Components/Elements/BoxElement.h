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
#include <AzQtComponents/Components/Elements/TabElement.h>
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
class QStyleOption;
class QStyleOptionButton;
class QStyleOptionComboBox;
class QStyleOptionToolButton;
struct QMetaObject;
class QWidget;

namespace AzQtComponents
{
    //! A box element of the flattened GUI: one complete definition of the box a widget of this look renders, in every state:
    //! the margin, border and padding around its contents, the limits on its size, and what the box draws (fill, border,
    //! radius, background image) in each state. Values are the ones the legacy GUI finally renders for the sites the element
    //! covers (the winners of its stylesheet cascade, or Qt's own behavior where no rule sets one); each definition names where
    //! they come from. Sites that render identically in every state share one element; no element is built from another.
    //!
    //! The box is Qt's QSS box model (qstylesheetstyle.cpp:1061-1122): the rect less the margin is the border box, less the
    //! border the padding box, less the padding the contents. Qt reads the contents rect through the style (SE_ShapedFrameContents
    //! of a frame, a label or a scroll area, SE_PushButtonContents of a push button) and derives frame widths, contents margins,
    //! viewports and size hints from it. The width, height and limits are those of the CONTENTS; the widget's own limits are those
    //! grown by the box (qstylesheetstyle.cpp:2570-2616).
    //!
    //! A module registers its elements with the class of the widgets they cover and a matcher (registerElement): of the matching
    //! registrations the first registered wins, and the Global elements (Scope::Global) come after every module's, whenever each
    //! was registered: AzQtComponents registers its own before a module has set up its styles. A widget resolves its element once,
    //! when it is polished (apply), and keeps it (forWidget); a draw reads the kept element. A widget flagged to ignore the style
    //! (Style::flagToIgnore) has none. Style.cpp reads the element at its contact points, in the flattened mode only. Elements
    //! and the strings of their images have static storage. An element is written with designated initializers, in the order of
    //! the members below; its tables have a fixed capacity (6 state faces, 3 properties), and a longer one does not compile.
    //!
    //! The rule of a widget is computed for each state (QRenderRule, qstylesheetstyle.cpp:1860-2128), so the box can change with
    //! it: a :hover rule that sets another margin or radius moves and rounds the box of that state alone. The element's margin,
    //! radius and spacing are those of every state, a face may give its own (Face::m_margin, m_radius, m_spacing). What is drawn,
    //! the rects of a state (borderRect, contentsRect and the sub-controls built on them) and the size a widget asks for (boxSize,
    //! comboBoxSize) are those of the state of the option: QStyleSheetStyle reads the rule of the option it is given for all of them
    //! (sizeFromContents, qstylesheetstyle.cpp:5299-5304), so a hint is the box of the state the widget is in when it computes
    //! it. Only the limits a widget gets at polish are those of the box Qt polished it in, the Enabled state, which is the idle face
    //! (QStyleSheetStyle::setGeometry reads the rule with PseudoClass_Enabled, :2572): the limits of apply do not follow the state.
    //! The label spacing of check boxes and radio buttons follows the option for the size of the button, and is that of the Disabled
    //! state (no option) for where its label sits and is drawn (spacing).
    struct AZ_QT_COMPONENTS_API BoxElement
    {
        using Edge = TabElement::Edge;
        using Edges = TabElement::Edges;

        //! The value of an integer member that is not set (the sheet's own -1: a size, a limit or a spacing it has not been given,
        //! qstylesheetstyle.cpp:410, 5094-5139)
        static constexpr int Unset = -1;

        //! background-origin: the box a background image is laid out in (the sheet's default is the padding box)
        enum class Origin
        {
            Padding,
            Content,
        };

        //! Who owns a registration: a module's elements are checked before the Global ones (as TabElement and TextElement do)
        enum class Scope
        {
            Module,
            Global,
        };

        //! The pseudo-states a face stands for (the legacy :hover, :pressed, :focus, :checked and :disabled)
        enum State : unsigned
        {
            Hover = 1,                          //!< enabled and under the pointer (a disabled widget is never hovered)
            Pressed = 2,                        //!< QStyle::State_Sunken
            Focus = 4,                          //!< QStyle::State_HasFocus
            Checked = 8,                        //!< QStyle::State_On (a combo box has it while its popup is shown, qcombobox.cpp:1499-1500)
            Disabled = 16,                      //!< not QStyle::State_Enabled
        };

        //! What the box draws in one state, and the parts of its box the rule of that state may change.
        struct Face
        {
            QBrush m_fill;                      //!< no brush: no fill. A vertical gradient is a brush (vertical)
            Edges m_edges;                      //!< the solid edges the sheet declared; an edge without a colour is the style's own or none
            QColor m_text;                      //!< push button label colour of the state; invalid: the widget's own
            const char* m_image = "";           //!< background image resource, once (no repeat), at its natural size ("": none)

            // ---- the box of the state, appended after the members above (an element written before them keeps compiling). The rules
            // of the states are computed one by one, so one that sets another margin, radius or spacing than the others changes the
            // box of its state alone (ComboBox.qss:22-42: :hover and :focus take the margin away, give a border and round the corners
            // by 3px instead of 2). Each is unset by default: the state has the element's.
            AZStd::optional<QMargins> m_margin; //!< margin of the state (left, top, right, bottom); unset: the element's m_margin
            int m_radius = Unset;               //!< border-radius of all four corners in the state; Unset: the element's m_radius
            int m_spacing = Unset;              //!< `spacing` of the state, the label spacing; Unset: the element's m_spacing

            //! border-image: url(...) 0 0 0 0 stretch stretch (a resource; "": none), the border of the state (AssetProcessor.qss:199):
            //! the image is stretched over the border box, whatever its size, in place of the declared border, clipped to the radius
            //! (QRenderRule::drawBorderImage, qstylesheetstyle.cpp:1167-1188, with qDrawBorderPixmap: the cuts of the image are 0, so only
            //! its center is drawn, stretched, qdrawutil.cpp:1119-1145). Qt then takes the border for the rule's own: it is not the
            //! style's (QRenderRule::hasNativeBorder, :511-514) and its width is the declared one, not the style's (fixupBorder, :1124-1165).
            //! OPEN, not carried: cuts other than 0 0 0 0 (qDrawBorderPixmap then draws the eight parts around the center, at the
            //! width of the border), the cuts left out (they are the border's widths, :1161-1163), the tile modes repeat and round
            //! (:1170-1171, 1185), an image that does not load (Qt then drops the border image and the rule draws its border, :1129-1130; the
            //! face here draws its edges), and a scroll area (its viewport is transparent over a border image, :1445-1448, and
            //! it repaints when scrolled, :2911-2917).
            const char* m_borderImage = "";
        };

        //! A face that applies while all of its states hold, as a rule with a compound pseudo-class does (:hover:focus is
        //! Hover | Focus, qstylesheetstyle.cpp:1714-1715). It is complete: where two rules reach a widget together and set
        //! different properties, the state that holds both is an entry of its own, after the entries of both. An entry is the
        //! face of exactly the states it names, measured with no other state held, and the last entry whose states all hold
        //! is the face: the table is right only when that reproduces the cascade for every combination of states that can
        //! hold together, so the order of the single states is not free. A Hover entry placed after a Focus one shows the
        //! hover face, whole, of a button that is hovered and focused; a Pressed entry measured with the pointer over the
        //! button shows hover's look when the button is pressed from the keyboard. The members of a face that say "unset" are the
        //! exception to its being complete: they are the element's until the rule of the state sets its own.
        struct StateFace
        {
            unsigned m_states = 0;              //!< State flags (0: unused entry)
            Face m_face;
        };

        //! A qproperty-* assignment of the sheet, made at each polish; a string becomes an icon where the property is one.
        struct Property
        {
            const char* m_name = nullptr;       //!< nullptr: unused entry
            QVariant m_value;
        };

        const char* m_name = "";                //!< Scope.Box.Look

        // ---- the box. Lengths are px. margin, padding: left, top, right, bottom; a negative side pulls the contents out. The margin,
        // the radius and the spacing are those of every state a face does not set its own for (Face::m_margin, m_radius, m_spacing).
        QMargins m_margin;
        QMargins m_padding;
        //! `spacing`, the label spacing of a check box or a radio button (QStyleSheetBoxData::spacing, qstylesheetstyle.cpp:406-411;
        //! PM_CheckBoxLabelSpacing and PM_RadioButtonLabelSpacing read it, :5125-5139) and the space after the icon of a combo box
        //! label (:4029-4031). Unset: the style's own. A rule that sets it has a box (hasBox, qcssparser.cpp:570-596).
        int m_spacing = Unset;
        //! The winning declarations include a margin or a padding, even a 0 (QRenderRule::hasBox): Qt then takes the box from the
        //! rule, and a frame or a push button reads its contents from it. Set it where a 0 is written (a non-zero side implies
        //! it, and so does a spacing, or a margin of a face). Without it a 0 would leave a Box frame of line width 3 at 3 where Qt
        //! reads the native 1, and a button sized by the base style where Qt sizes it by the box (qstylesheetstyle.cpp:5336-5350,
        //! 6177-6182).
        bool m_hasBox = false;
        //! The border is the style's own (Qt's default sheet gives a QFrame, a push button and a tool button "border: native",
        //! qstylesheetstyle_default.cpp:171-184, 246-272; any other class has none): as wide as the base style's frame, 0 for
        //! a QFrame without one, drawn by the base style inside the margin. False when the winning declarations give the
        //! border a style of its own: a border (its edges are the faces'), border: none, a border-radius, which cancels
        //! the native border (qstylesheetstyle.cpp:1133-1138), or a border-image (a face's m_borderImage makes it so for its state
        //! alone). A flat push button has none once drawn and sized (PushButton.qss:48-52) but has one when polished (:2572): its
        //! limits count it.
        bool m_nativeBorder = true;
        int m_radius = 0;                       //!< border-radius of all four corners

        // ---- the contents' width, height and limits (-1: not set)
        QSize m_size;
        QSize m_minSize;
        QSize m_maxSize;

        // ---- the drop-down of a combo box: its `::drop-down { width; padding }` rule (ProjectManager.qss:161-168 over ComboBox.qss:59-62),
        // the same in every state. A sheet that gives it no size leaves it as wide as Qt makes it (DropDownWidth).
        //! The width of the drop-down's contents. Set: its box is that and its padding wide (QRenderRule::boxSize, qstylesheetstyle.cpp:1116-1122).
        //! A padding with no width is not counted: the rule then has no size (boxSize keeps an unset width unset, :1119-1120) and Qt makes the
        //! drop-down DropDownWidth wide (defaultSize, :2302-2305). Unset: DropDownWidth.
        int m_dropDownWidth = Unset;
        //! The padding of the drop-down: its arrow is centered in what the padding leaves (positionRect with the origin of an arrow, which is
        //! the contents of its drop-down: qstylesheetstyle.cpp:2198-2211, 2426-2433)
        QMargins m_dropDownPadding;

        // ---- the faces: idle, then the entries of the states in the order of the cascade; of those whose states all hold, the
        // last is the face (a later rule overrode an earlier one). The idle face is the face of a state with no pseudo-class held, and the
        // box the limits of a widget are taken from (apply); what it leaves unset is the element's.
        Face m_idle;
        StateFace m_stateFaces[6];

        // ---- where the background image sits in every state (background-position, background-origin)
        Qt::Alignment m_imageAlign = Qt::AlignTop | Qt::AlignLeft;
        Origin m_imageOrigin = Origin::Padding;

        // ---- qproperty-* of the sheet
        Property m_properties[3];

        //! `text-align` of the rule, the one of the position declarations a push button's rule has in the sheets (ValueExtractor::extractPosition,
        //! qcssparser.cpp:546-568: left, top, right, bottom, subcontrol-origin, subcontrol-position, position and text-align make a rule have a
        //! position, QRenderRule::hasPosition, qstylesheetstyle.cpp:534, 898). The sheet style then lays out the label of a push button itself
        //! (CE_PushButtonLabel, qstylesheetstyle.cpp:3688-3783, the condition at :3697): the icon and the text in one group, in the alignment of
        //! the rule (centered where it names none), with 4px between them, where QCommonStyle puts the text 2px after the icon
        //! (qcommonstyle.cpp:1362-1377; its group is as wide, 4px, so the sheet's label is the base style's with the text 2px further from the
        //! icon). 0: the rule has no position and the label is the base style's.
        Qt::Alignment m_textAlign;

        // ---- the box, for the state of an option (QRenderRule, qstylesheetstyle.cpp:505-536, 1061-1122). An option gives the state
        // the way Qt's renderRule reads it (pseudoClass, :1757-1804); no option (nullptr) is the state Qt reads then, which is
        // Disabled alone (renderRule, :1860-1862 with State_None).
        bool hasNativeBorder(const QStyleOption* option) const;     //!< the style's border draws in this state (a border image is not the style's)
        bool ownsBox(const QStyleOption* option) const;             //!< Qt takes the box from the rule: a margin or padding, or a border of its own
        QRect borderRect(const QStyleOption* option) const;         //!< option->rect inside the margin of the state
        QRect contentsRect(const QStyleOption* option, const QWidget* widget) const;    //!< option->rect inside margin, border and padding of the state
        //! contents plus margin, border, padding of the state of the option (CT_* of QStyleSheetStyle, qstylesheetstyle.cpp:5304-5413)
        QSize boxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const;
        QSize adjustSize(const QSize& contents) const;              //!< contents within the width, height and limits (QRenderRule::adjustSize)
        //! The label spacing of the state (PM_RadioButtonLabelSpacing and PM_CheckBoxLabelSpacing, qstylesheetstyle.cpp:5125-5139; the
        //! icon of a combo box label, :4029-4031): the face's, else the element's; Unset (-1): the style's own. Qt reads it with
        //! the option for the size of a check box or a radio button (CT_CheckBox, :5409) and with no option for where the label sits and
        //! is drawn (SE_CheckBoxContents, :6200), so the two can differ for a state whose rule sets another spacing (:checked).
        int spacing(const QStyleOption* option) const;

        // ---- faces and paint
        const Face& face(const QStyleOption* option) const;
        //! QRenderRule::drawRule (qstylesheetstyle.cpp:1386-1390) of the state in option->rect: fill, image, then the declared
        //! border or the border image. False: the state draws nothing (a native border, no fill, no image).
        bool draw(QPainter* painter, const QStyleOption* option, const QWidget* widget) const;
        //! QRenderRule::baseStyleCanDraw (qstylesheetstyle.cpp:521-529) of a state, for the class of the widget the rule styles: its
        //! default sheet lists the background colour as a feature of the base style for a line edit and a spin box
        //! (qstylesheetstyle_default.cpp:130-153, 323-347) and the colour and the gradient for a combo box (:274-301). No fill, or one of
        //! those, and no image: anything else is drawn by the rule, with the style's own frame over it.
        static bool baseStyleCanDraw(const Face& face, const QWidget* styled);
        //! The palette the base style draws a native frame with: the fill is its Window and Base, and the shades of a solid
        //! one (QRenderRule::configurePalette, qstylesheetstyle.cpp:1396-1405).
        static void configurePalette(QPalette& palette, const Face& face);
        //! QRenderRule::configurePalette (qstylesheetstyle.cpp:1393-1422) for the label of a button-like control: the fill as
        //! the role the control paints its face with (Button; NoRole: the fill is not applied), the colour of the state as the role it
        //! paints its text with
        static void configurePalette(QPalette& palette, const Face& face, QPalette::ColorRole foreground, QPalette::ColorRole background);
        //! qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 top, stop: 1 bottom)
        static QBrush vertical(const QColor& top, const QColor& bottom);
        //! The same solid edge on all four sides.
        static Edges border(int width, const QColor& color);

        // ---- a rule a component draws itself: the corners differ (SegmentControl.qss:35-50 rounds two of a bar's four)
        //! The radius of each corner of a border box (border-top-left-radius and the rest; `border-radius` sets all four, qcssparser.cpp
        //! QStyleSheetBorderData::radii, qstylesheetstyle.cpp:1141-1156). A corner that does not fit its box is square (qNormalizeRadii,
        //! qcssutil.cpp:279-294).
        struct Radii
        {
            int m_topLeft = 0;
            int m_topRight = 0;
            int m_bottomRight = 0;
            int m_bottomLeft = 0;
        };

        //! QRenderRule::drawFrame (qstylesheetstyle.cpp:1372-1377) of a rule with no image, in its border box (the rect less the margin of
        //! the rule): the fill (a fillRect where no corner is round, else an antialiased path through the middle of the border, :1348-1370),
        //! then the solid edges as qDrawBorder draws them, antialiased: a strip for each edge between its corners and an arc at each round
        //! one (qcssutil.cpp:313-369), so that at a fractional device ratio the end rows of a line blend as Native's do. Edges are the
        //! declared ones (an edge with no colour is not drawn). draw() is this for the rule of an element.
        static void drawFrame(QPainter* painter, const QRect& borderBox, const QBrush& fill, const Edges& edges, const Radii& radii);

        // ---- push buttons: the label of a button whose rule has a position (m_textAlign)
        //! CE_PushButtonLabel of a button whose rule declares `text-align`, as QStyleSheetStyle laid it out (qstylesheetstyle.cpp:3688-3783):
        //! the option's rect is the contents rect of the box, its palette the one of the state. False: the rule has no position and the base
        //! style draws the label.
        bool drawPushButtonLabel(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const;

        // ---- tool buttons: a tool button the element boxes (qstylesheetstyle.cpp:3238-3388, 5963-5974)
        //! CC_ToolButton for a border of its own: the rule over the whole rect, the label in its contents rect, and the drop-down
        //! and the menu indicator of the base style (the rule has no ::menu-button or ::menu-indicator). False: the border is the
        //! style's own, which the style draws.
        bool drawToolButton(const QStyle* style, const QStyleOptionToolButton* option, QPainter* painter, const QWidget* widget) const;
        //! SC_ToolButton (inside the margin) and SC_ToolButtonMenu (right and top of that, as wide as menuWidth: the base style's
        //! PM_MenuButtonIndicator, qstylesheetstyle.cpp:2329-2332)
        QRect toolButtonRect(const QStyleOptionToolButton* option, QStyle::SubControl subControl, int menuWidth) const;

        // ---- combo boxes: a combo box the element boxes (qstylesheetstyle.cpp:3049-3087, 5336-5347, 5799-5830). Where the rule has no
        // ::drop-down width of its own, the drop-down is as wide as Qt makes it.
        static constexpr int DropDownWidth = 16;        //!< qstylesheetstyle.cpp:2302-2305
        //! CT_ComboBox: the contents (within the declared size already) with the drop-down and the 2px the base style leaves in the
        //! label added (:5343-5344), the box around it all
        QSize comboBoxSize(const QStyleOption* option, const QWidget* widget, const QSize& contents) const;
        //! SC_ComboBoxArrow: the drop-down, at the right and top of the padding box (:2426-2433, 5816-5819), as wide as m_dropDownWidth
        //! and m_dropDownPadding say, DropDownWidth if they do not
        QRect dropDownRect(const QStyleOption* option, const QWidget* widget) const;
        //! SC_ComboBoxEditField: the contents rect less the drop-down (:5804-5815)
        QRect editFieldRect(const QStyleOption* option, const QWidget* widget) const;
        //! The arrow the drop-down shows (ComboBox.qss:64-70 `::down-arrow`): 16 x 16, centered in the contents of the drop-down (what
        //! m_dropDownPadding leaves) with the 4px margin it keeps to its right
        QRect arrowRect(const QStyleOption* option, const QWidget* widget) const;
        //! CE_ComboBoxLabel for a rule that draws (:4021-4056): the icon and the text in the edit field, clipped to it, the text after
        //! the icon and the spacing of the state (6px where it is unset, :4029-4031). False: the rule draws nothing and the base style
        //! draws the label.
        bool drawComboBoxLabel(const QStyle* style, const QStyleOptionComboBox* option, QPainter* painter, const QWidget* widget) const;

        // ---- check boxes and radio buttons: a box around the indicator element (qstylesheetstyle.cpp:3789-3808)
        //! CE_CheckBox and CE_RadioButton: the rule over the whole rect, then the indicator element draws the indicator and the label
        //! in the contents rect, the label in the colour of the state and after the spacing the style gives with no option (spacing).
        //! False: the widget has no indicator element.
        bool drawIndicatorButton(const QStyle* style, const QStyleOptionButton* option, QPainter* painter, const QWidget* widget) const;

        // ---- resolution: once per widget at polish, kept on the widget
        using Matcher = bool (*)(const QWidget* widget);
        //! The element for the widgets of widgetClass (and its subclasses) a matcher accepts. Of the registrations that match
        //! a widget the module's come first, in the order they were registered, then the Global ones in theirs. Widgets
        //! polished before the registration are resolved again.
        static void registerElement(const BoxElement* element, const QMetaObject* widgetClass, Matcher matcher, Scope scope = Scope::Module);
        //! From Style::polish(QWidget*), after the base style and the components (the sheet applied its geometry and
        //! properties after them, qstylesheetstyle.cpp:2875-2890): resolve the element (none for a widget that containerOf gives
        //! another widget), keep it, set its limits, properties, hover and styled background.
        static void apply(QWidget* widget);
        //! The element kept on the widget; null: none, or not polished yet.
        static const BoxElement* forWidget(const QWidget* widget);
        //! PE_Widget where no element draws: the widget's background role fills a widget whose element took the autofill from it and
        //! draws nothing, and the viewport of a scroll area whose element has no background in the state of the option
        //! (qstylesheetstyle.cpp:4682-4691)
        static bool fillsBackgroundRole(const QWidget* widget, const QStyleOption* option);
        //! The widget whose element a widget is drawn by: the combo box or spin box a line edit sits in, the scroll area a viewport
        //! belongs to (qstylesheetstyle.cpp:2472-2493), else the widget itself. Such a widget has no element of its own: the sheet
        //! styled it by no rule (:2499-2506).
        static const QWidget* containerOf(const QWidget* widget);
    };
} // namespace AzQtComponents
