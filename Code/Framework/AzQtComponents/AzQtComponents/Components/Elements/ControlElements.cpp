/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/Internal/OverlayWidgetLayer.h>
#include <AzQtComponents/Components/Widgets/Slider.h>

#include <QAbstractAnimation>
#include <QBrush>
#include <QColor>
#include <QDockWidget>
#include <QFrame>
#include <QGroupBox>
#include <QHashFunctions>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionProgressBar>
#include <QToolBar>
#include <QToolButton>
#include <QTransform>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ControlElementsInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- The buttons beside a dock's tabs
        constexpr QMargins TabButtonsMargin(0, 0, 0, 3);            // TitleBar.qss:56 $space.surface.idle.margin-bottom.Alt1
        const QColor TitleBarFill = Foundations::Clear;             // TitleBar.qss:28 $fill.surface.idle.Clear

        // ---- The minimize, maximize and close buttons of a tab widget's action toolbar (a tabbed floating window)
        // TitleBar.qss:100 rgba(255,255,255,0.1): Qt reads an alpha of 1 or less as a fraction and truncates it, 0.1 * 255 = 25
        // (QCss::parseColorValue). TOKENIZE: WindowButtonHoverFill
        const QColor WindowButtonHoverFill(255, 255, 255, 25);
        const QColor WindowCloseHoverFill(0xB8, 0x0D, 0x1C);        // TitleBar.qss:104 TOKENIZE: WindowCloseHoverFill
        constexpr const char* WindowMinimizeIcon = ":/Application/titlebar-minimize.svg";   // TitleBar.qss:134 TOKENIZE: WindowMinimizeIcon
        constexpr const char* WindowMaximizeIcon = ":/Application/titlebar-maximize.svg";   // TitleBar.qss:119 TOKENIZE: WindowMaximizeIcon
        constexpr const char* WindowCloseIcon = ":/Application/titlebar-close.svg";         // TitleBar.qss:109 TOKENIZE: WindowCloseIcon

        // ---- The close button of a filter tag
        const QColor CloseTagFill = Foundations::Clear;             // FilteredSearchWidget.qss:42 $fill.button.idle.fill.Clear
        constexpr int CloseTagMinWidth = 16;                        // FilteredSearchWidget.qss:47 $size.button.idle.min-width.Alt4
        // FilteredSearchWidget.qss:46, :45 $size.button.idle.max-width.Alt3 $size.button.idle.max-height.Alt3
        constexpr QSize CloseTagMaxSize(16, 16);
        constexpr bool CloseTagFlat = true;                         // FilteredSearchWidget.qss:41 $other.button.idle.flat
        constexpr const char* CloseTagIcon = ":/stylesheet/img/close_x.svg";    // FilteredSearchWidget.qss:39 $asset.button.idle.icon.Alt3
        constexpr QSize CloseTagIconSize(10, 10);                   // FilteredSearchWidget.qss:40 $asset.button.idle.iconsize.Alt5

        // ---- The panel of a toolbar, and the button for the actions that do not fit it
        const QColor ToolBarFill = Foundations::Mid;                // ToolBar.qss:20 $fill.bar.idle.Mid
        // ToolBar.qss:33 $line.bar.idle.width.Alt2 $line.bar.idle.color
        const BoxElement::Edge ToolBarBottomLine{ 2, Foundations::FarBG };
        constexpr const char* ToolBarExtensionIcon = ":/stylesheet/img/hidden-icons.svg";   // ToolBar.qss:141 $asset.bar.idle.icon

        // ---- The handle of a movable toolbar: the image, its own size in the file, and the padding of each rule
        // ToolBar.qss:83 $asset.scroll-handle.idle.Alt1
        constexpr const char* HandleImageHorizontal = ":/stylesheet/img/UI20/tear.svg";
        // ToolBar.qss:88 $asset.scroll-handle.idle.Alt2
        constexpr const char* HandleImageVertical = ":/stylesheet/img/UI20/tear-vertical.svg";
        constexpr QSize HandleImageSizeHorizontal(2, 14);           // tear.svg:2, the size of $asset.scroll-handle.idle.Alt1
        constexpr QSize HandleImageSizeVertical(24, 6);             // tear-vertical.svg:2, the size of $asset.scroll-handle.idle.Alt2
        constexpr QMargins MainHandlePadding(4, 1, 4, 1);           // ToolBar.qss:94, :106 $space.scroll-handle.idle.Alt3
        constexpr QMargins LargeHandlePadding(3, 4, 3, 4);          // ToolBar.qss:118 $space.scroll-handle.idle.Alt4
        constexpr QMargins HandlePadding(4, 2, 4, 2);               // ToolBar.qss:130 $space.scroll-handle.idle.Alt2
        constexpr QMargins MainLargeVerticalHandlePadding(1, 2, 1, 2);  // ToolBar.qss:99 $space.scroll-handle.idle.Alt5
        constexpr QMargins VerticalHandlePadding(-2, 2, -2, 2);     // ToolBar.qss:112, :123, :136 $space.scroll-handle.idle.Alt1

        // ---- The buttons of a segment bar
        const QColor SegmentFill = Foundations::Low;                // SegmentControl.qss:16 $fill.button.idle.fill.Low
        const QColor SegmentHoverFill = Foundations::Mid;           // SegmentControl.qss:76 $fill.button.hover+unchecked
        const QColor SegmentPressedFill = Foundations::Mid;         // SegmentControl.qss:76 $fill.button.pressed+unchecked
        const QColor SegmentCheckedFill = Foundations::Highest;     // SegmentControl.qss:70 $fill.button.checked
        const QColor SegmentLine = Foundations::BG;                 // SegmentControl.qss:17 $line.button.idle.color.BG
        constexpr int SegmentLineWidth = 1;                         // SegmentControl.qss:19 $line.button.idle.width.Alt2
        // SegmentControl.qss:31-32 $space.button.idle.padding-left $space.button.idle.padding-right
        constexpr QMargins SegmentPadding(16, 0, 16, 0);
        constexpr int SegmentContentHeight = 28;                    // SegmentControl.qss:23 $size.button.idle.height
        constexpr int SegmentMinContentWidth = 68;                  // SegmentControl.qss:29 $size.button.idle.min-width.Alt3
        constexpr int SegmentMarginLeft = -1;                       // SegmentControl.qss:37, :54 $space.button.idle.margin-left.Alt2
        constexpr int SegmentMarginTop = -1;                        // SegmentControl.qss:65 $space.button.idle.margin-top.Alt1
        // SegmentControl.qss:46-47 $radius.button.idle.top-left.Alt2 $radius.button.idle.bottom-left.Alt2
        constexpr BoxElement::Radii FirstSegmentRadii{ .m_topLeft = 3, .m_bottomLeft = 3 };
        // SegmentControl.qss:40-41 $radius.button.idle.top-right.Alt3 $radius.button.idle.bottom-right.Alt3
        constexpr BoxElement::Radii LastSegmentRadii{ .m_topRight = 3, .m_bottomRight = 3 };
        constexpr Qt::Alignment VerticalSegmentTextAlign = Qt::AlignLeft;   // SegmentControl.qss:58 $other.button.idle.text-align.Alt1

        // ---- The frame of a floating dock, and of breadcrumbs whose path can be typed
        // StyledDockWidget.qss:17 $line.surface.floating.width $line.surface.floating.color
        const BoxElement::Edge FloatingDockLine{ 1, QColor(0x00, 0x00, 0x00) };
        // BreadCrumbs.qss:35 $line.bar.editable.width $line.bar.editable.color
        const BoxElement::Edge EditableBreadCrumbsLine{ 1, QColor(0x2F, 0x2F, 0x2F) };
        constexpr int EditableBreadCrumbsRadius = 2;                // BreadCrumbs.qss:36 $radius.bar.editable

        // ---- The button of breadcrumbs that shows the path that does not fit them
        // BreadCrumbs.qss:22 qproperty-icon. TOKENIZE: BreadCrumbsMenuIcon
        constexpr const char* BreadCrumbsMenuIcon = ":/Breadcrumb/img/UI20/Breadcrumb/dot-dot-dot_with_arrow.svg";
        // BreadCrumbs.qss:23 qproperty-iconSize. TOKENIZE: BreadCrumbsMenuIconSize
        constexpr QSize BreadCrumbsMenuIconSize(22, 11);

        // ---- The labels of a message box: the padding that keeps the text and the icon off the border of the box
        // The margin above and below every label has (Text.qss:93-94) and the fill (BaseStyleSheet.qss:17) are the foundations'
        // (Foundations::Label::MarginV, Foundations::Surface::Fill), which the elements below restate
        // MessageBox.qss:13-15 padding-top, padding-right, padding-bottom. TOKENIZE: MessageBoxTextPadding
        constexpr QMargins MessageBoxTextPadding(0, 20, 20, 20);
        // MessageBox.qss:20-21 padding-left, padding-top. TOKENIZE: MessageBoxIconPadding
        constexpr QMargins MessageBoxIconPadding(20, 20, 0, 0);

        // ---- The tool tip of a slider: its offset from the pointer, added to SliderConfig.ini's (Slider::ShowValueToolTip)
        // Slider.qss:12-13 qproperty-toolTipOffsetX, -Y. TOKENIZE: HorizontalSliderToolTipOffset
        constexpr int HorizontalSliderToolTipOffsetX = -8;
        constexpr int HorizontalSliderToolTipOffsetY = -16;
        // Slider.qss:18-19 qproperty-toolTipOffsetX, -Y. TOKENIZE: VerticalSliderToolTipOffset
        constexpr int VerticalSliderToolTipOffsetX = 8;
        constexpr int VerticalSliderToolTipOffsetY = -24;

        // ---- The progress bar: a square track, a flat chunk, and no text
        const QColor ProgressTrackFill(0x9A, 0x9A, 0x9A);           // ProgressBar.qss:14 background. TOKENIZE: ProgressTrackFill
        const QColor ProgressChunkFill(0x1E, 0x70, 0xEB);           // ProgressBar.qss:23 background. TOKENIZE: ProgressChunkFill
        // ProgressBar.qss:18 qproperty-textVisible. TOKENIZE: ProgressTextVisible
        constexpr bool ProgressTextVisible = false;
        //! The busy chunk moves 8px every step of the sheet style's animation, which runs at the common style's 30 frames a second
        //! (QStyleSheetStyle::drawControl, CE_ProgressBarContents, and QCommonStylePrivate::animationFps)
        constexpr int BusyStepPixels = 8;
        constexpr double BusyStepMilliseconds = 1000.0 / 30.0;
        constexpr const char* BusyAnimationName = "_o3de_progressBusy";

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--TitleBar[drawAsTabBar="true"] #tabButtons`: the frame of a dock's minimize, maximize and close buttons while
        //! the dock draws as a tab bar. Its margin keeps the 26px buttons at the top of the title bar. The frame is polished before its
        //! title bar sets the property, so TitleBar::setDrawAsTabBar resolves it again.
        const BoxElement TabButtons{
            .m_name = "Global.Box.TabButtons",
            .m_margin = TabButtonsMargin,
            .m_idle = {
                .m_fill = QBrush(TitleBarFill),
            },
        };

        //! `AzQtComponents--TabWidget QToolButton#minimizeButton { qproperty-icon }` and `QToolButton#minimizeButton:hover`: the minimize
        //! button the action toolbar of a tab widget makes for the action of its floating window (TabWidgetActionToolBar), which has no
        //! icon of its own. The sheet set the icon at polish, over the action's. The hover fill is painted at PE_Widget: the toolbar
        //! forces the styled background of its buttons (TabWidgetActionToolBar.cpp), and the fill is the state's, enabled and under the
        //! pointer, whether the button is pressed or not. The rule `QToolButton#minimizeButton:hover` reaches no other tool button of
        //! the name: they have no styled background, so the rule paints nothing there (the Style draws a tool button's background in
        //! no state).
        const BoxElement TabWidgetMinimizeButton{
            .m_name = "Global.Box.TabWidget.MinimizeButton",
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = QBrush(WindowButtonHoverFill) } } },
            .m_properties = {
                { "icon", QString(WindowMinimizeIcon) },
            },
        };

        //! `AzQtComponents--TabWidget QToolButton#maximizeButton { qproperty-icon }` and `QToolButton#maximizeButton:hover`: as the
        //! minimize button above, for the maximize action
        const BoxElement TabWidgetMaximizeButton{
            .m_name = "Global.Box.TabWidget.MaximizeButton",
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = QBrush(WindowButtonHoverFill) } } },
            .m_properties = {
                { "icon", QString(WindowMaximizeIcon) },
            },
        };

        //! `AzQtComponents--TabWidget QToolButton#closeButton { qproperty-icon }` and `QToolButton#closeButton:hover`: as the minimize
        //! button above, for the close action, whose hover fill is red. The close button of a console's find bar below a tab widget (a
        //! console docked as a tab) is reached by the icon rule too, and has no styled background to paint the hover fill on.
        const BoxElement TabWidgetCloseButton{
            .m_name = "Global.Box.TabWidget.CloseButton",
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = QBrush(WindowCloseHoverFill) } } },
            .m_properties = {
                { "icon", QString(WindowCloseIcon) },
            },
        };

        //! `AzQtComponents--FilterCriteriaButton QPushButton#closeTag`, and the same rule of FilterTextButton: a flat button with the close
        //! glyph, no margin or padding (FilteredSearchWidget.qss:43-44 $space.button.idle.margin.Alt1 $space.button.idle.padding.Alt1),
        //! and no border: every push button of the search widget has `border: none` (FilteredSearchWidget.qss:18
        //! $line.button.idle.style.Alt1). The rule of FilterTextButton has the same values (FilteredSearchWidget.qss:90-101)
        const BoxElement CloseTag{
            .m_name = "Global.Box.CloseTag",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_minSize = QSize(CloseTagMinWidth, -1),
            .m_maxSize = CloseTagMaxSize,
            .m_idle = {
                .m_fill = QBrush(CloseTagFill),
            },
            .m_properties = {
                { "flat", CloseTagFlat },
                { "icon", QString(CloseTagIcon) },
                { "iconSize", CloseTagIconSize },
            },
        };

        //! `QToolBar { background-color; border: 0 none }`: a border of its own, none (ToolBar.qss:22 $line.bar.idle.style.Alt1), so the
        //! sheet draws no bevel of the base style (QStyleSheetStyle::drawControl, CE_ToolBar)
        const BoxElement ToolBarPanel{
            .m_name = "Global.Box.ToolBar",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),
            },
        };

        //! `MainWindow QToolBar { border-bottom }` over the rule above; a toolbar of a DockMainWindow has none (the later rule
        //! `AzQtComponents--DockMainWindow QToolBar`)
        const BoxElement ToolBarPanelUnderMainWindow{
            .m_name = "Global.Box.ToolBar.MainWindow",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ToolBarFill),
                .m_edges = { {}, {}, ToolBarBottomLine, {} },               // top, right, bottom, left
            },
        };

        //! `QToolBarExtension#qt_toolbar_ext_button { qproperty-icon }`: the double caret of the button at the end of a toolbar too narrow
        //! for its actions, set at polish over the base style's caret (QStyleSheetStyle::setProperties)
        const BoxElement ToolBarExtension{
            .m_name = "Global.Box.ToolBarExtension",
            .m_properties = {
                { "icon", QString(ToolBarExtensionIcon) },
            },
        };

        //! `AzQtComponents--StyledDockWidget[floating="true"] { border }`: a border of its own, so no frame of the base style. The element
        //! is resolved at the dock's polish and so holds the rule the sheet matched then: a dock polished docked has none
        //! (StyledDockWidget.qss:11 `border: none`), whatever it becomes afterwards.
        const BoxElement FloatingDockFrame{
            .m_name = "Global.Box.StyledDockWidget.Floating",
            .m_nativeBorder = false,
            .m_idle = {
                .m_edges = { FloatingDockLine, FloatingDockLine, FloatingDockLine, FloatingDockLine },
            },
        };

        //! `AzQtComponents--BreadCrumbs[editable="true"] { border; border-radius }`: the frame of breadcrumbs whose path can be edited (the
        //! Asset Browser's, the Gallery's). Its layout lies inside the frame, so the border is a frame of 1px; the rule has no fill.
        const BoxElement BreadCrumbsEditable{
            .m_name = "Global.Box.BreadCrumbsEditable",
            .m_nativeBorder = false,
            .m_radius = EditableBreadCrumbsRadius,
            .m_idle = {
                .m_edges = { EditableBreadCrumbsLine, EditableBreadCrumbsLine, EditableBreadCrumbsLine, EditableBreadCrumbsLine },
            },
        };

        //! `AzQtComponents--BreadCrumbs #MenuButton { qproperty-icon; qproperty-iconSize }`: the button at the left of breadcrumbs that
        //! opens the menu of the path, shown while the path is wider than they are (BreadCrumbs::resizeEvent). The sheet gave it the
        //! dots with an arrow, 22 x 11, at polish (QStyleSheetStyle::setProperties). The rule has no box and no fill: the button
        //! is the component's own.
        const BoxElement BreadCrumbsMenuButton{
            .m_name = "Global.Box.BreadCrumbsMenuButton",
            .m_properties = {
                { "icon", QString(BreadCrumbsMenuIcon) },
                { "iconSize", BreadCrumbsMenuIconSize },
            },
        };

        //! `QMessageBox QLabel#qt_msgbox_label { padding-top; padding-right; padding-bottom }`: the text of a message box, 20px in from
        //! the border of the box above, at the right and below (the icon's padding keeps it off the left). The label keeps the margin
        //! above and below that every label has (Text.qss:93-94) and its transparent background (BaseStyleSheet.qss:17): an element is
        //! the whole definition of its widget's box, so it restates them.
        const BoxElement MessageBoxText{
            .m_name = "Global.Box.MessageBoxText",
            .m_margin = QMargins(0, Foundations::Label::MarginV, 0, Foundations::Label::MarginV),
            .m_padding = MessageBoxTextPadding,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! `QMessageBox QLabel#qt_msgboxex_icon_label { padding-left; padding-top }`: the icon of a message box, 20px in from the border
        //! of the box at the left and above. The label's margin and background as above.
        const BoxElement MessageBoxIcon{
            .m_name = "Global.Box.MessageBoxIcon",
            .m_margin = QMargins(0, Foundations::Label::MarginV, 0, Foundations::Label::MarginV),
            .m_padding = MessageBoxIconPadding,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! The text of a message box below a color picker (ColorPicker::done, its import and export errors and the question to close a
        //! palette ask with the picker as the parent): `AzQtComponents--ColorPicker QLabel { margin: 0px }` (ColorPicker.qss:48) has
        //! two type names, more than Text.qss:93-94's `QLabel`, so the label has no margin above and below. The padding of
        //! MessageBox.qss outranks the picker's `padding: 0px` (ColorPicker.qss:47), which only the left of the text label is left to.
        const BoxElement MessageBoxTextInColorPicker{
            .m_name = "Global.Box.MessageBoxTextInColorPicker",
            .m_padding = MessageBoxTextPadding,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! The icon of a message box below a color picker: no margin, as the text above
        const BoxElement MessageBoxIconInColorPicker{
            .m_name = "Global.Box.MessageBoxIconInColorPicker",
            .m_padding = MessageBoxIconPadding,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! `AzQtComponents--Slider.HorizontalSlider { qproperty-toolTipOffsetX; qproperty-toolTipOffsetY }`: the tip of the value of a
        //! horizontal slider, 8px left of the pointer and 16px higher than the offset of SliderConfig.ini puts it, set at polish
        //! (QStyleSheetStyle::setProperties) over the offset the slider has (0, 0)
        const BoxElement HorizontalSliderToolTip{
            .m_name = "Global.Box.HorizontalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", HorizontalSliderToolTipOffsetX },
                { "toolTipOffsetY", HorizontalSliderToolTipOffsetY },
            },
        };

        //! `AzQtComponents--Slider.VerticalSlider { qproperty-toolTipOffsetX; qproperty-toolTipOffsetY }`: the tip of a vertical slider,
        //! 8px right and 24px above the offset of SliderConfig.ini
        const BoxElement VerticalSliderToolTip{
            .m_name = "Global.Box.VerticalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", VerticalSliderToolTipOffsetX },
                { "toolTipOffsetY", VerticalSliderToolTipOffsetY },
            },
        };

        //! `QProgressBar { border: 0px transparent #000000; border-radius: 0px; background: #9A9A9A; qproperty-textVisible: False }`: the
        //! track of every progress bar, square and borderless (ProgressBar.qss:12-13), drawn by the sheet at CE_ProgressBarGroove, and the
        //! text a bar hides when it is polished (the height, 5px, is ProgressBarConfig.ini's, and the 1px font TextElement's). The chunk
        //! has no element of its own: ProgressChunkFill is drawn by drawProgressBar for every bar, as ProjectManager.qss:778-780 repeats.
        const BoxElement ProgressBarTrack{
            .m_name = "Global.Box.ProgressBar",
            // ProgressBar.qss:12-13 border, border-radius: none, square. TOKENIZE: ProgressTrackBorder
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ProgressTrackFill),
            },
            .m_properties = {
                { "textVisible", ProgressTextVisible },
            },
        };

        //! `* { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400 }` on a group box that no rule of
        //! a module reaches (BaseStyleSheet.qss:15-21): a background, a palette and a font of its own are enough for the sheet style to
        //! draw it through the sheet and never leave it to the base style (QStyleSheetStyle::drawComplexControl, CC_GroupBox), which
        //! keeps the frame out of the rect of the title and centers the title in it. BoxElement::drawGroupBox does that for any group
        //! box under an element, so the element is the clear fill and the border of the style alone: no box, so the base style sizes the
        //! contents (QStyleSheetStyle::subControlRect, SC_GroupBoxFrame and SC_GroupBoxContents). The colour and the font are the
        //! palette's and TextElement's. A group box with a rule of its own has the element of its module first.
        const BoxElement PlainGroupBox{
            .m_name = "Global.Box.GroupBox",
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),       // BaseStyleSheet.qss:17. TOKENIZE: GroupBoxFill
            },
        };

        //! `QFrame#icon_frame { background-color }` of the toast (ToastNotification.ui:59 before the flattening): the frame at the left of
        //! the toast that holds its icon, a lighter block on the toast. The sheet had no selector, so it also reached the icon label
        //! inside the frame, which the label's own sheet cancelled (a transparent fill); an element does not cascade to the label, so
        //! the label has none to cancel. The toast sets no tooltip.
        const BoxElement ToastIconFrame{
            .m_name = "Global.Box.ToastIconFrame",
            .m_idle = {
                .m_fill = QBrush(ControlElements::ToastIconFrameFill),
            },
        };

        //! `background-color:rgba(0, 0, 0, 179)` of an overlay layer that dims (OverlayWidgetLayer.cpp:35 LayerStyle, set at :69 and
        //! :103 before the flattening): a translucent black over what the layer covers. The sheet had no selector, so the fill is also
        //! that of the widgets below the layer, which stack their own over it (the elements below: the labels, the frames of the layer's
        //! own form and the tips). Another class below the layer, such as the buttons AddButtons makes in m_controls, has no element.
        const BoxElement OverlayLayer{
            .m_name = "Global.Box.OverlayLayer",
            .m_idle = {
                .m_fill = QBrush(ControlElements::OverlayDimFill),
            },
        };

        //! A label below a layer that dims: the centre widget of the layer, in both places the importer shows one. The label keeps the
        //! margin above and below that every label has (Text.qss:93-94), which an element restates as it is the whole definition of its
        //! widget's box.
        const BoxElement OverlayLayerLabel{
            .m_name = "Global.Box.OverlayLayerLabel",
            .m_margin = QMargins(0, Foundations::Label::MarginV, 0, Foundations::Label::MarginV),
            .m_idle = {
                .m_fill = QBrush(ControlElements::OverlayDimFill),
            },
        };

        //! The frames (QFrame::StyledPanel) of the layer's own form, below a layer that dims when it has no centre widget (m_center and
        //! m_controls, OverlayWidgetLayer.ui): the fill of the layer over the panel fill of every frame (BaseStyleSheet.qss:41-48, the
        //! fill at :47). The style draws the line of the frame over the palette of the fill, as the sheet style did.
        const BoxElement OverlayLayerFrame{
            .m_name = "Global.Box.OverlayLayerFrame",
            .m_idle = {
                .m_fill = QBrush(ControlElements::OverlayDimFill),
            },
        };

        //! The tip of a widget below a layer that dims: the sheet had no selector, so it reached the tip of every widget it was set on
        //! and below (a tip is resolved through the widget it is shown for), and the layer's fill outranks ToolTip.qss's black
        const ToolTipElement OverlayLayerToolTip{
            .m_name = "Global.ToolTip.OverlayLayer",
            .m_fill = ControlElements::OverlayDimFill,
        };

        // ------------------------------------------------------------------------------
        // Helpers: the toast and the overlay layer
        // ------------------------------------------------------------------------------

        //! `#icon_frame` below the toast, the way ToastNotification.ui names it
        bool isToastIconFrame(const QWidget* widget)
        {
            return Selectors::named(widget, "icon_frame") && Selectors::withinClass(widget, "AzQtComponents::ToastNotification");
        }

        //! A layer that dims: OverlayWidgetLayer tags it where the legacy GUI sets the sheet
        bool isDimmingLayer(const QWidget* widget)
        {
            return Selectors::tagged(widget, ControlElements::OverlayDimmedTag);
        }

        bool belowDimmingLayer(const QWidget* widget)
        {
            return Selectors::withinTagged(widget, ControlElements::OverlayDimmedTag);
        }

        //! A label below a layer that dims, a tip excluded: Qt makes the tip a child of the widget it is shown for where it can (not on
        //! Windows), and the sheet's `QLabel` never reached it (a tip matches the type selector `QToolTip` alone,
        //! QStyleSheetStyleSelector::nodeNames), its fill is the tip element's
        bool isLabelBelowDimmingLayer(const QWidget* widget)
        {
            return belowDimmingLayer(widget) && !ToolTipElement::isToolTip(widget);
        }

        //! m_center and m_controls, the frames of the layer's own form, below a layer that dims
        bool isLayerFormFrame(const QWidget* widget)
        {
            return (Selectors::named(widget, "m_center") || Selectors::named(widget, "m_controls")) && belowDimmingLayer(widget);
        }

        // ------------------------------------------------------------------------------
        // Helpers: the handle of a toolbar
        // ------------------------------------------------------------------------------

        struct HandleBox
        {
            const char* m_image;
            QSize m_imageSize;
            QMargins m_padding;                                     //!< left, top, right, bottom
        };

        //! The handle rule that wins for a toolbar's tags: of the rules that hold, the most specific, of equal ones the later
        HandleBox handleBox(const QWidget* toolbar, bool horizontal)
        {
            const bool main = Selectors::tagged(toolbar, "MainToolBar");
            const bool large = Selectors::tagged(toolbar, "IconLarge");
            if (horizontal)
            {
                if (main)
                {
                    return { HandleImageHorizontal, HandleImageSizeHorizontal, MainHandlePadding };
                }
                return { HandleImageHorizontal, HandleImageSizeHorizontal, large ? LargeHandlePadding : HandlePadding };
            }
            return { HandleImageVertical, HandleImageSizeVertical,
                (main && large) ? MainLargeVerticalHandlePadding : VerticalHandlePadding };
        }

        // ------------------------------------------------------------------------------
        // Helpers: the buttons of a segment bar
        // ------------------------------------------------------------------------------

        //! Where a button sits in its bar. SegmentBar::updateTabs tags every button TabOne, TabFirst, TabMiddle or TabLast, and the bar
        //! that owns the orientation is an ancestor of the button, not its parent (`AzQtComponents--SegmentBar QAbstractButton`).
        struct SegmentPlace
        {
            bool m_isButton = false;
            QWidget* m_bar = nullptr;   //!< the SegmentBar the button is in (null for a widget that is no button of a bar)
            bool m_vertical = false;    //!< the bar's orientation is Qt::Vertical
            bool m_first = false;       //!< TabFirst
            bool m_last = false;        //!< TabLast
            bool m_one = false;         //!< TabOne: the base rule alone

            //! TabMiddle and TabLast start on the border of the button before them: the border strip outside the widget is not drawn,
            //! and the neighbour's line is the one shared
            bool overlapsPrevious() const
            {
                return m_isButton && !m_first && !m_one;
            }

            //! The margin of the rule: left, top, right, bottom
            QMargins margin() const
            {
                if (!overlapsPrevious())
                {
                    return QMargins();
                }
                return m_vertical ? QMargins(0, SegmentMarginTop, 0, 0) : QMargins(SegmentMarginLeft, 0, 0, 0);
            }

            //! A horizontal bar rounds the outer corners of its first button and of its last, whatever the layout direction; a vertical
            //! bar rounds none
            BoxElement::Radii radii() const
            {
                BoxElement::Radii radii;
                if (!m_vertical && m_first)
                {
                    radii.m_topLeft = FirstSegmentRadii.m_topLeft;
                    radii.m_bottomLeft = FirstSegmentRadii.m_bottomLeft;
                }
                if (!m_vertical && m_last)
                {
                    radii.m_topRight = LastSegmentRadii.m_topRight;
                    radii.m_bottomRight = LastSegmentRadii.m_bottomRight;
                }
                return radii;
            }

            //! The box around the contents (QRenderRule::boxSize): the margin of the place, the border and the padding
            QMargins around() const
            {
                const QMargins line(SegmentLineWidth, SegmentLineWidth, SegmentLineWidth, SegmentLineWidth);
                return margin() + line + SegmentPadding;
            }
        };

        SegmentPlace segmentPlaceOf(const QWidget* button)
        {
            SegmentPlace place;
            if (!button)
            {
                return place;
            }
            place.m_first = Selectors::tagged(button, "TabFirst");
            place.m_last = Selectors::tagged(button, "TabLast");
            place.m_one = Selectors::tagged(button, "TabOne");
            const bool tagged = place.m_first || place.m_last || place.m_one || Selectors::tagged(button, "TabMiddle");
            for (QWidget* ancestor = tagged ? button->parentWidget() : nullptr; ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::SegmentBar"))
                {
                    place.m_isButton = true;
                    place.m_bar = ancestor;
                    place.m_vertical = ancestor->property("orientation").toInt() == static_cast<int>(Qt::Vertical);
                    break;
                }
            }
            return place;
        }

        //! The widgets above a widget as one number: it changes when QWidget::setParent moves the widget or any widget above it, and in
        //! no other way (a hash of the pointers, which are not reused while the widgets live)
        quint64 ancestryOf(const QWidget* widget)
        {
            size_t hash = 0;
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                hash = qHash(ancestor, hash);
            }
            return static_cast<quint64>(hash);
        }

        //! The dynamic property that keeps the ancestry a bar had when the style last polished one of its buttons
        constexpr const char* SegmentAncestryProperty = "_o3de_segmentBarAncestry";

        //! The fill of the state: :checked, the later rule, whatever else holds; then pressed, or hovered (:hover needs :enabled)
        const QColor& segmentFill(const QStyleOption* option)
        {
            if (option->state.testFlag(QStyle::State_On))
            {
                return SegmentCheckedFill;
            }
            if (option->state.testFlag(QStyle::State_Sunken))
            {
                return SegmentPressedFill;
            }
            const bool hovered = option->state.testFlag(QStyle::State_Enabled) && option->state.testFlag(QStyle::State_MouseOver);
            return hovered ? SegmentHoverFill : SegmentFill;
        }

        //! The rule's min-width with the box around it, which QStyleSheetStyle::setGeometry made the button's minimum width
        int segmentMinimumWidth(const SegmentPlace& place)
        {
            const QMargins around = place.around();
            return SegmentMinContentWidth + around.left() + around.right();
        }

        // ------------------------------------------------------------------------------
        // Helpers: selectors
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--BreadCrumbs[editable="true"]`
        bool isEditableBreadCrumbs(const QWidget* widget)
        {
            return widget->inherits("AzQtComponents::BreadCrumbs") && Selectors::hasProperty(widget, "editable", "true");
        }

        //! `AzQtComponents--BreadCrumbs #MenuButton`
        bool isMenuButtonOfBreadCrumbs(const QWidget* widget)
        {
            return Selectors::named(widget, "MenuButton") && Selectors::withinClass(widget, "AzQtComponents::BreadCrumbs");
        }

        //! `QMessageBox QLabel#name`: the label Qt names so in every message box
        bool isLabelOfMessageBox(const QWidget* widget, const char* name)
        {
            return Selectors::named(widget, name) && Selectors::withinClass(widget, "QMessageBox");
        }

        //! `AzQtComponents--ColorPicker QLabel`: the message boxes of the picker are asked with the picker as their parent
        bool isBelowColorPicker(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::ColorPicker");
        }

        //! `AzQtComponents--TitleBar[drawAsTabBar="true"] W`
        bool inTitleBarDrawnAsTabBar(const QWidget* widget)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::TitleBar") && Selectors::hasProperty(ancestor, "drawAsTabBar", "true"))
                {
                    return true;
                }
            }
            return false;
        }

        //! `AzQtComponents--TabWidget QToolButton#name`, less the dock bar buttons: they draw their glyph from the style whatever their
        //! icon property is (DockBarButton::paintEvent), and the hover fill of the ones in a tab is the tab element's
        bool isWindowButtonOfTabWidget(const QWidget* widget, const char* name)
        {
            return Selectors::named(widget, name) && !widget->inherits("AzQtComponents::DockBarButton")
                && Selectors::withinClass(widget, "AzQtComponents::TabWidget");
        }

        //! `AzQtComponents--FilterCriteriaButton QPushButton#closeTag`, and the rule of FilterTextButton that repeats it
        bool inFilterTag(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::FilterCriteriaButton")
                || Selectors::withinClass(widget, "AzQtComponents::FilterTextButton");
        }

        //! `MainWindow QToolBar` less the toolbars the later `AzQtComponents--DockMainWindow QToolBar` rule left without a border
        bool inMainWindowOutsideDockMainWindow(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "MainWindow") && !Selectors::withinClass(widget, "AzQtComponents::DockMainWindow");
        }

        // ------------------------------------------------------------------------------
        // Helpers: the busy progress bar (QStyleSheetStyle::drawControl, CE_ProgressBarContents)
        // ------------------------------------------------------------------------------

        //! What moves the chunk of a bar with neither a range nor a value: an animation that runs while the bar is drawn busy, a child
        //! of the bar. It repaints the bar every step, and stops once the bar is hidden.
        class BusyAnimation
            : public QAbstractAnimation
        {
        public:
            explicit BusyAnimation(QWidget* bar)
                : QAbstractAnimation(bar)
                , m_bar(bar)
            {
                setObjectName(QString::fromLatin1(BusyAnimationName));
            }

            int duration() const override
            {
                return -1;          // runs until it is stopped
            }

            //! The step of the animation: the chunk is that many steps along
            int step() const
            {
                return static_cast<int>(currentTime() / BusyStepMilliseconds);
            }

        protected:
            void updateCurrentTime(int) override
            {
                if (!m_bar->isVisible())
                {
                    stop();
                }
                else if (const int now = step(); now != m_step)
                {
                    m_step = now;
                    m_bar->update();
                }
            }

        private:
            QWidget* m_bar;
            int m_step = 0;
        };

        //! The animation of a bar, if it has one
        QAbstractAnimation* busyAnimationOf(const QWidget* bar)
        {
            return bar->findChild<QAbstractAnimation*>(QString::fromLatin1(BusyAnimationName), Qt::FindDirectChildrenOnly);
        }

        //! The step a busy bar draws: the animation starts when the bar is first drawn busy (step 0) and runs on
        int busyStepOf(const QWidget* bar)
        {
            QAbstractAnimation* animation = busyAnimationOf(bar);
            if (!animation)
            {
                animation = new BusyAnimation(const_cast<QWidget*>(bar));
            }
            if (animation->state() != QAbstractAnimation::Running)
            {
                animation->start();
            }
            return static_cast<BusyAnimation*>(animation)->step();
        }

        //! A bar with a value draws no animation: the one it had is gone, and starts again from step 0 when it is busy once more
        void stopBusyAnimation(const QWidget* bar)
        {
            delete busyAnimationOf(bar);
        }
    } // namespace ControlElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void ControlElements::registerGlobal()
    {
        using namespace ControlElementsInternal;

        static bool registered = false;
        if (registered || !StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        constexpr ElementScope global = ElementScope::Global;

        BoxElement::registerElement(&TabButtons, &QFrame::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "tabButtons") && inTitleBarDrawnAsTabBar(w); }, global);
        BoxElement::registerElement(&CloseTag, &QPushButton::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "closeTag") && inFilterTag(w); }, global);
        BoxElement::registerElement(&FloatingDockFrame, &StyledDockWidget::staticMetaObject,
            [](const QWidget* w) { const auto* dock = qobject_cast<const QDockWidget*>(w); return dock && dock->isFloating(); }, global);
        BoxElement::registerElement(&ToolBarExtension, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "qt_toolbar_ext_button") && w->inherits("QToolBarExtension"); }, global);
        BoxElement::registerElement(&BreadCrumbsEditable, &QFrame::staticMetaObject, &isEditableBreadCrumbs, global);
        BoxElement::registerElement(&TabWidgetMinimizeButton, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return isWindowButtonOfTabWidget(w, "minimizeButton"); }, global);
        BoxElement::registerElement(&TabWidgetMaximizeButton, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return isWindowButtonOfTabWidget(w, "maximizeButton"); }, global);
        BoxElement::registerElement(&TabWidgetCloseButton, &QToolButton::staticMetaObject,
            [](const QWidget* w) { return isWindowButtonOfTabWidget(w, "closeButton"); }, global);
        BoxElement::registerElement(&BreadCrumbsMenuButton, &QToolButton::staticMetaObject, &isMenuButtonOfBreadCrumbs, global);

        // The labels of a message box below a color picker first: the picker's rule for its labels outranks Text.qss's margin
        BoxElement::registerElement(&MessageBoxTextInColorPicker, &QLabel::staticMetaObject,
            [](const QWidget* w) { return isLabelOfMessageBox(w, "qt_msgbox_label") && isBelowColorPicker(w); }, global);
        BoxElement::registerElement(&MessageBoxIconInColorPicker, &QLabel::staticMetaObject,
            [](const QWidget* w) { return isLabelOfMessageBox(w, "qt_msgboxex_icon_label") && isBelowColorPicker(w); }, global);
        BoxElement::registerElement(&MessageBoxText, &QLabel::staticMetaObject,
            [](const QWidget* w) { return isLabelOfMessageBox(w, "qt_msgbox_label"); }, global);
        BoxElement::registerElement(&MessageBoxIcon, &QLabel::staticMetaObject,
            [](const QWidget* w) { return isLabelOfMessageBox(w, "qt_msgboxex_icon_label"); }, global);

        // The sliders of Slider.qss. Slider::polish tags a slider with its orientation, and the elements are resolved at the end of
        // Style::polish, so the tag is known then
        BoxElement::registerElement(&HorizontalSliderToolTip, &Slider::staticMetaObject,
            [](const QWidget* w) { return Selectors::tagged(w, "HorizontalSlider"); }, global);
        BoxElement::registerElement(&VerticalSliderToolTip, &Slider::staticMetaObject,
            [](const QWidget* w) { return Selectors::tagged(w, "VerticalSlider"); }, global);

        // The track and the hidden text of every progress bar (ProgressBar.qss:10-19); a module's element for its bars comes first
        BoxElement::registerElement(&ProgressBarTrack, &QProgressBar::staticMetaObject, [](const QWidget*) { return true; }, global);

        // Every group box that no module's element covers (BaseStyleSheet.qss:15-21)
        BoxElement::registerElement(&PlainGroupBox, &QGroupBox::staticMetaObject, [](const QWidget*) { return true; }, global);

        // The icon frame of the toast (ToastNotification.ui:59 before the flattening)
        BoxElement::registerElement(&ToastIconFrame, &QFrame::staticMetaObject, &isToastIconFrame, global);
    }

    void ControlElements::registerOverlayLayer()
    {
        using namespace ControlElementsInternal;

        static bool registered = false;
        if (registered)
        {
            return;
        }
        registered = true;

        // The sheet of an overlay layer that dims (OverlayWidgetLayer.cpp:35, :69, :103 before the flattening): the layer, the labels
        // and the frames of its own form below it, and the tips of the widgets below it
        const ElementScope global = ElementScope::Global;
        BoxElement::registerElement(&OverlayLayer, &Internal::OverlayWidgetLayer::staticMetaObject, &isDimmingLayer, global);
        BoxElement::registerElement(&OverlayLayerLabel, &QLabel::staticMetaObject, &isLabelBelowDimmingLayer, global);
        BoxElement::registerElement(&OverlayLayerFrame, &QFrame::staticMetaObject, &isLayerFormFrame, global);
        ToolTipElement::registerElement(&OverlayLayerToolTip, &Internal::OverlayWidgetLayer::staticMetaObject, &isDimmingLayer, global);
    }

    // ----------------------------------------------------------------------------------
    // Toolbars
    // ----------------------------------------------------------------------------------

    bool ControlElements::drawToolBarPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return false;
        }
        // A module's rules for the toolbars below a window (`EditorWindow QToolBar` in Editor.qss) are an element of the toolbar, which
        // draws what the sheet's rules of the toolbar drew, those of ToolBar.qss included
        const BoxElement* own = BoxElement::forWidget(widget);
        const BoxElement& panel = own ? *own : (inMainWindowOutsideDockMainWindow(widget) ? ToolBarPanelUnderMainWindow : ToolBarPanel);
        return panel.draw(painter, option, widget);
    }

    int ControlElements::toolBarHandleExtent(const QStyleOption* option, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return -1;
        }
        // The sheet read the handle rule's box in the option's state, a vertical one with no option (QStyleSheetStyle::pixelMetric):
        // the image and the padding along the toolbar
        const bool horizontal = option && option->state.testFlag(QStyle::State_Horizontal);
        const HandleBox box = handleBox(widget, horizontal);
        return horizontal ? box.m_imageSize.width() + box.m_padding.left() + box.m_padding.right()
                          : box.m_imageSize.height() + box.m_padding.top() + box.m_padding.bottom();
    }

    bool ControlElements::drawToolBarHandle(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QToolBar*>(widget))
        {
            return false;
        }
        // The rule draws its image centered in the handle's contents rect (QStyleSheetStyle::drawPrimitive,
        // PE_IndicatorToolBarHandle; QRenderRule::drawRule)
        const HandleBox box = handleBox(widget, option->state.testFlag(QStyle::State_Horizontal));
        const QRect contents = option->rect.marginsRemoved(box.m_padding);
        QIcon(QString::fromLatin1(box.m_image)).paint(painter, contents, Qt::AlignCenter);
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Segment bars
    // ----------------------------------------------------------------------------------

    QSize ControlElements::segmentButtonSize(const QSize& contents, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QPushButton*>(widget))
        {
            return QSize();
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return QSize();
        }
        // QRenderRule::adjustSize, then the box around it (QRenderRule::boxSize)
        const QMargins around = place.around();
        return QSize(qMax(contents.width(), SegmentMinContentWidth) + around.left() + around.right(),
            SegmentContentHeight + around.top() + around.bottom());
    }

    QRect ControlElements::segmentButtonContents(const QStyleOption* option, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const QPushButton*>(widget))
        {
            return QRect();
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return QRect();
        }
        // The rule's contents rect, mirrored for a right to left option (QStyleSheetStyle::subElementRect)
        return QStyle::visualRect(option->direction, option->rect, option->rect.marginsRemoved(place.around()));
    }

    bool ControlElements::drawSegmentButton(
        const QStyle* style, QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        // Asked for every control element of every widget: the cheap tests first
        const bool pushButton =
            element == QStyle::CE_PushButton || element == QStyle::CE_PushButtonBevel || element == QStyle::CE_PushButtonLabel;
        if (!StyleManager::stylesheetsDisabled() || !pushButton || !qobject_cast<const QPushButton*>(widget))
        {
            return false;
        }
        const SegmentPlace place = segmentPlaceOf(widget);
        if (!place.m_isButton)
        {
            return false;
        }
        switch (element)
        {
        case QStyle::CE_PushButtonBevel:
            // A rule with a border of its own is drawn whole over the widget's rect (QStyleSheetStyle::drawControl,
            // CE_PushButtonBevel): the fill and the border in the border box, which the negative margin pulls out of the widget
            BoxElement::drawFrame(painter, option->rect.marginsRemoved(place.margin()), QBrush(segmentFill(option)),
                BoxElement::border(SegmentLineWidth, SegmentLine), place.radii());
            return true;

        case QStyle::CE_PushButton:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option))
            {
                // QCommonStyle::drawControl: the bevel, then the label in the rule's contents. Fusion would ask its own subElementRect,
                // which is not the rule's. No focus rectangle: the buttons of a bar take no focus.
                style->drawControl(QStyle::CE_PushButtonBevel, button, painter, widget);
                QStyleOptionButton label(*button);
                label.rect = segmentButtonContents(button, widget);
                style->drawControl(QStyle::CE_PushButtonLabel, &label, painter, widget);
                return true;
            }
            return false;

        case QStyle::CE_PushButtonLabel:
            if (const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option);
                button && place.m_vertical && button->icon.isNull())
            {
                // The rule's text-align in its contents, vertically centered, the mnemonic shown unless the style hides it
                // (QStyleSheetStyle::drawControl, CE_PushButtonLabel). A button with an icon keeps the base style's centered group.
                const QRect contents = widget->rect().marginsRemoved(place.around());
                int textFlags = int(VerticalSegmentTextAlign) | Qt::AlignVCenter | Qt::TextShowMnemonic;
                if (!style->styleHint(QStyle::SH_UnderlineShortcut, button, widget, nullptr))
                {
                    textFlags |= Qt::TextHideMnemonic;
                }
                style->drawItemText(painter, contents, textFlags, button->palette, button->state.testFlag(QStyle::State_Enabled),
                    button->text, QPalette::ButtonText);
                return true;
            }
            return false;

        default:
            return false;
        }
    }

    void ControlElements::polishSegmentBar(QWidget* widget)
    {
        using namespace ControlElementsInternal;

        if (!StyleManager::stylesheetsDisabled() || !widget)
        {
            return;
        }
        // The sheet gave a button its minimum width when it polished it (QStyleSheetStyle::setGeometry), and WA_Hover for the
        // :hover of its fill
        const auto polishButton = [](QWidget* button, const SegmentPlace& place)
        {
            button->setMinimumWidth(segmentMinimumWidth(place));
            button->setAttribute(Qt::WA_Hover, true);
        };
        if (qobject_cast<QPushButton*>(widget))
        {
            if (const SegmentPlace place = segmentPlaceOf(widget); place.m_isButton)
            {
                polishButton(widget, place);
                place.m_bar->setProperty(SegmentAncestryProperty, QVariant::fromValue(ancestryOf(place.m_bar)));
            }
            return;
        }

        // SegmentBar::updateTabs sizes every button to the widest after QPushButton::sizeHint polished them, so the minimum width the
        // sheet set is gone. It came back wherever the bar, or a widget above it, was moved into another widget after the tabs were
        // added: the sheet polished a reparented widget again (QWidgetPrivate::setStyle_helper) and set its geometry. The bar, polished
        // at its first show, does the same where a widget above it moved since its buttons were last polished.
        if (qobject_cast<QFrame*>(widget) && widget->inherits("AzQtComponents::SegmentBar"))
        {
            const QVariant polishedAt = widget->property(SegmentAncestryProperty);
            const quint64 ancestry = ancestryOf(widget);
            if (!polishedAt.isValid() || polishedAt.toULongLong() == ancestry)
            {
                return;                 // no button was polished yet, or nothing above the bar was moved since
            }
            widget->setProperty(SegmentAncestryProperty, QVariant::fromValue(ancestry));
            const QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
            bool changed = false;
            for (QPushButton* button : buttons)
            {
                if (const SegmentPlace place = segmentPlaceOf(button); place.m_isButton)
                {
                    polishButton(button, place);
                    changed = true;
                }
            }
            if (changed)
            {
                // The repolish told every widget its style changed, so that it forgot its size hint and its layout laid out again
                // (QWidget::changeEvent): the bar's layout caches the hints of the buttons' container until it is told
                for (QPushButton* button : buttons)
                {
                    if (QWidget* container = button->parentWidget())
                    {
                        container->updateGeometry();
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Floating docks
    // ----------------------------------------------------------------------------------

    int ControlElements::dockFrameWidth(const QWidget* widget)
    {
        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const StyledDockWidget*>(widget))
        {
            return -1;
        }
        // The rule's left border, plus the margin and the padding of a rule that has a box (QStyleSheetStyle::pixelMetric,
        // PM_DockWidgetFrameWidth); a dock polished docked has `border: none`: 0
        const BoxElement* frame = BoxElement::forWidget(widget);
        return frame ? frame->m_idle.m_edges.m_left.m_width + frame->m_margin.left() + frame->m_padding.left() : 0;
    }

    bool ControlElements::drawDockFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        if (!StyleManager::stylesheetsDisabled() || !qobject_cast<const StyledDockWidget*>(widget))
        {
            return false;
        }
        // A rule with a border of its own draws it in the option's border rect; `border: none` draws nothing
        // (QStyleSheetStyle::drawPrimitive, PE_FrameDockWidget)
        if (const BoxElement* frame = BoxElement::forWidget(widget))
        {
            frame->draw(painter, option, widget);
        }
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Progress bars
    // ----------------------------------------------------------------------------------

    QVector<QRect> ControlElements::progressChunkRects(const QStyleOptionProgressBar* option, const QRect& contents, int busyStep)
    {
        using namespace ControlElementsInternal;

        QVector<QRect> chunks;

        // A vertical bar is laid out as a horizontal one turned by 90 degrees, and each rect is turned back
        const qint64 minimum = option->minimum;
        const qint64 maximum = option->maximum;
        const qint64 progress = option->progress;
        const bool vertical = !option->state.testFlag(QStyle::State_Horizontal);
        QTransform turn;
        QRect rect = contents;
        if (vertical)
        {
            rect = QRect(rect.y(), rect.x(), rect.height(), rect.width());
            turn.rotate(90);
            turn.translate(0, -(rect.height() + rect.y() * 2));
        }

        // The chunk grows from the left (the right in a right to left layout and in a vertical bar, which grows upwards), and an
        // inverted bar from the other edge
        bool reverse = (!vertical && option->direction == Qt::RightToLeft) || vertical;
        if (option->invertedAppearance)
        {
            reverse = !reverse;
        }
        const bool noRange = minimum == maximum;
        const double fillRatio = noRange ? 0.5 : double(progress - minimum) / double(maximum - minimum);
        const int chunkWidth = static_cast<int>(rect.width() * fillRatio);
        if (chunkWidth <= 0)
        {
            return chunks;
        }
        const auto place = [&](int x)
        {
            return turn.mapRect(QRectF(x, rect.y(), chunkWidth, rect.height())).toRect();
        };

        if (minimum == 0 && maximum == 0)
        {
            // Busy: a chunk of half the bar moves along it, and the part beyond the far edge comes in again at the near one
            int chunkCount = 1;
            const int offset = (busyStep * BusyStepPixels) % rect.width();
            int x = reverse ? rect.left() + rect.width() - offset - chunkWidth : rect.x() + offset;
            while (chunkCount > 0)
            {
                chunks.append(place(x));
                x += reverse ? -chunkWidth : chunkWidth;
                if (reverse ? x < rect.left() : x > rect.right())
                {
                    break;
                }
                --chunkCount;
            }
            x = reverse ? rect.right() - (rect.left() - x - chunkWidth) : rect.left() + (x - rect.right() - chunkWidth);
            while (chunkCount > 0)
            {
                chunks.append(place(x));
                x += reverse ? -chunkWidth : chunkWidth;
                --chunkCount;
            }
        }
        else
        {
            chunks.append(place(reverse ? rect.left() + rect.width() - chunkWidth : rect.x()));
        }
        return chunks;
    }

    bool ControlElements::drawProgressBar(
        const QStyle* style, const QStyleOptionProgressBar* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ControlElementsInternal;

        const BoxElement* track = (StyleManager::stylesheetsDisabled() && qobject_cast<const QProgressBar*>(widget))
            ? BoxElement::forWidget(widget) : nullptr;
        if (!track)
        {
            return false;
        }

        // QCommonStyle::drawControl, CE_ProgressBar: the groove, the contents and the label, each in the rect of its sub-element. The
        // sheet's are the rule's border rect and contents rect, and it draws the groove in the border rect of the border rect.
        QStyleOptionProgressBar groove(*option);
        groove.rect = track->borderRect(option);
        groove.rect = track->borderRect(&groove);
        track->draw(painter, &groove, widget);

        // The chunk, the rule of ::chunk (a background): flat rects clipped to the contents
        const QRect contents = track->contentsRect(option, widget);
        const bool busy = option->minimum == 0 && option->maximum == 0;
        const int busyStep = busy ? busyStepOf(widget) : 0;
        if (!busy)
        {
            stopBusyAnimation(widget);
        }
        painter->save();
        painter->setClipRect(contents);
        for (const QRect& chunk : progressChunkRects(option, contents, busyStep))
        {
            BoxElement::drawFrame(painter, chunk, QBrush(ProgressChunkFill), BoxElement::Edges(), BoxElement::Radii());
        }
        painter->restore();

        // The label of a bar that shows its text, in the colour of the text and not that of the chunk (SE_ProgressBarLabel: the text
        // aligned in the bar's rect)
        if (option->textVisible)
        {
            const QRect label = QStyle::alignedRect(
                Qt::LeftToRight, option->textAlignment, option->fontMetrics.size(0, option->text), option->rect);
            style->drawItemText(painter, label, option->textAlignment | Qt::TextSingleLine, option->palette,
                option->state.testFlag(QStyle::State_Enabled), option->text, QPalette::Text);
        }
        return true;
    }
} // namespace AzQtComponents
