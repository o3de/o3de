/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioFlatSheet.h"
#include "EMStudioFlatValues.h"
#include "MainWindow.h"

#include <Editor/LineEditValidatable.h>
#include <MysticQt/Source/DialogStack.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/SplitterElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMargins>
#include <QMenu>
#include <QProgressBar>
#include <QSize>
#include <QSplitter>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

// ----------------------------------------------------------------------------------
// The flattened GUI's carrier of the Animation Editor's inline style sheets
//
// Each look below is the one the legacy GUI drew for the widgets whose site set a sheet of its own, and every value cites the line of the
// sheet it folds: the lines are those of the files before the flattening, where the sheet text was the argument of setStyleSheet and now
// is the argument of StyleManager::setLegacyStyleSheet, and of the application's sheets beneath them (Editor.qss and the sheets it
// imports: BaseStyleSheet.qss, LineEdit.qss, ComboBox.qss, Text.qss, ProgressBar.qss, ToolTip.qss, Menu.qss, Splitter.qss). A sheet
// without a selector is `* { ... }`: it reached the widget it was set on and every widget below it, and the popups Qt makes with the
// widget as their parent (a line edit's menu, a combo box's list, a tooltip). The looks of those are below as well.
//
// A rule of a widget's own sheet outranks every rule of the application's sheets for the declarations it makes, whatever their
// specificity, so the :hover and :focus rules of the application do not reach a declaration the inline sheet made. An element is the
// whole definition of its widget's look, so it restates what the application's sheets gave the widget besides.
//
// The state of a widget (an input field that is not valid, a combo box that warns) is a class tag, which the sites set with the sheet in
// the legacy GUI (EMStudioFlat::SetStateTag). A state that ends on the widget's own text colour has an element for that colour as well,
// as the flattened Style gives no palette back when the colour of a text element ends.
//
// Not carried: the transparent property editors (`QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }`), which
// change only an exact QFrame of a panel shape below them, and no such frame is made there; the transparent check boxes and labels
// the application's sheets make transparent already; `spacing: 0px` of a check box that has no text. Those sites only route their
// sheet (StyleManager::setLegacyStyleSheet).
//
// Not carried either, by the user's ruling (plan D.42): the red 1px border the sheet of an invalid input field gave its context menu,
// and the orange 1px border the motion set combo box's sheet gave the scroll bars of its popup list while it warns. That menu and
// those scroll bars take the general looks the same elements have in every other tool.
// ----------------------------------------------------------------------------------

namespace EMStudioFlat
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
    //! where anonymous namespaces merge.
    namespace EMStudioFlatSheetInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::FrameElement;
        using AzQtComponents::IndicatorElement;
        using AzQtComponents::MenuElement;
        using AzQtComponents::SplitterElement;
        using AzQtComponents::TextElement;
        using AzQtComponents::ToolTipElement;
        using AzQtComponents::Selectors::named;
        using AzQtComponents::Selectors::tagged;
        using AzQtComponents::Selectors::withinClass;
        using Widget = const QWidget*;
        namespace Foundations = AzQtComponents::Foundations;
        namespace Field = AzQtComponents::Foundations::Field;
        namespace Label = AzQtComponents::Foundations::Label;
        namespace Values = EMStudioFlatValues;

        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        //! The application's box of a line edit (LineEdit.qss:11-19) over the entry field: no padding left, the text one pixel up
        //! (the padding above is -1 and the one below 1). TOKENIZE: InputFieldPadding
        const QMargins InputFieldPadding(Field::PaddingLeft, Field::PaddingTop, 0, Field::PaddingBottom);

        //! `border-width: 2px` (LineEdit.qss:16): the line of a line edit that its own sheet gives a background and no border of its
        //! own. TOKENIZE: InputFieldLineWidth
        constexpr int InputFieldLineWidth = 2;

        //! The edges of an input field while its text is not valid: `border: 1px solid red;` (EMStudioManager.cpp:262,
        //! LineEditValidatable.cpp:46), in every state of the field, over the 2px line of LineEdit.qss:16
        const BoxElement::Edges InvalidInputEdges = BoxElement::border(Values::InvalidInputLineWidth, Values::InvalidInputLine);

        //! `padding-left: 3px` (RecoverFilesWindow.cpp:118). TOKENIZE: RecoverCheckPaddingLeft
        constexpr int RecoverCheckPaddingLeft = 3;

        //! `max-width: 13px` (RecoverFilesWindow.cpp:118): the check box is at most 16 wide with its padding.
        //! TOKENIZE: RecoverCheckMaxWidth
        constexpr int RecoverCheckMaxWidth = 13;

        //! `padding: 6px` (ResetSettingsDialog.cpp:56, the sheet of the strip behind the title). TOKENIZE: ResetSettingsTopLabelPadding
        constexpr int ResetSettingsTopLabelPadding = 6;

        //! `padding-right: 1px` (CommandBarPlugin.cpp:93). TOKENIZE: CommandBarProgressTextPaddingRight
        constexpr int ProgressTextPaddingRight = 1;

        //! `padding-right: 2px` (CommandBarPlugin.cpp:101). TOKENIZE: CommandBarProgressBarPaddingRight
        constexpr int ProgressBarPaddingRight = 2;

        //! `background: #9A9A9A` of every progress bar (ProgressBar.qss:14), which the sheet of the command bar's bar sits over
        //! (Global.Box.ProgressBar is private to ControlElements.cpp). TOKENIZE: ProgressTrackFill
        const QColor ProgressTrackFill(0x9A, 0x9A, 0x9A);

        //! `qproperty-textVisible: False` (ProgressBar.qss:18). TOKENIZE: ProgressTextVisible
        constexpr bool ProgressTextVisible = false;

        //! The caret of the 'Motion Extraction' check box, right while it is collapsed and down while it is open
        //! (MotionExtractionWindow.cpp:156-165). TOKENIZE: CaretCheckCollapsedImage, CaretCheckExpandedImage
        constexpr const char* CaretRightImage = ":/Cards/img/UI20/Cards/caret-right.svg";
        constexpr const char* CaretDownImage = ":/Cards/img/UI20/Cards/caret-down.svg";

        //! `margin: 0px` of the caret (MotionExtractionWindow.cpp:154). TOKENIZE: CaretCheckMargin
        constexpr int CaretCheckMargin = 0;

        //! `QSplitter::handle{ height: 4px; background: transparent; }` while a dialog of the stack is open (DialogStack.cpp:35, :383),
        //! `height: 1px` while it is closed (:146, :176, :469). TOKENIZE: DialogStackOpenGap, DialogStackClosedGap
        constexpr int DialogStackOpenGap = 4;
        constexpr int DialogStackClosedGap = 1;

        //! `margin: 1px` of a combo box (ComboBox.qss:17), `padding-left: 4px` (:16), and the margin 0 of its :hover, :focus and open
        //! popup (:24, :35). TOKENIZE: ComboBoxMargin, ComboBoxPadding
        constexpr QMargins ComboMargin(1, 1, 1, 1);
        constexpr QMargins ComboPadding(4, 0, 0, 0);
        constexpr QMargins ComboStateMargin(0, 0, 0, 0);

        //! `border: 1px solid orange;` (AnimGraphEditor.cpp:229, :245): the width of the line. TOKENIZE: AnimGraphEditorWarningBorderWidth
        constexpr int WarningLineWidth = 1;

        //! `QComboBox QAbstractItemView { background-color: #222222 }` (ComboBox.qss:81): the fill of the popup list.
        //! TOKENIZE: ComboPopupSurface
        const QColor ComboPopupSurface = Foundations::BG;

        //! `border: 2px dashed` of the preview frame (ParameterCreateEditWidget.cpp:72). TOKENIZE: ParameterPreviewBorderWidth
        constexpr int PreviewLineWidth = 2;

        //! `QToolTip { font-size: 12px; font-family: "Open Sans" }` (ToolTip.qss:16, :19): the text of a tip, over which a `color` of
        //! the sheet of the tip's widget outranks ToolTip.qss:12. TOKENIZE: ToolTipFamily, ToolTipSize
        constexpr const char* ToolTipFamily = "Open Sans";
        constexpr int ToolTipSize = 12;

        // ------------------------------------------------------------------------------
        // The input fields: `border: 1px solid red;` while the text is not valid, `background-color` on a disabled track's name
        // ------------------------------------------------------------------------------

        //! The field of EMStudioManager::SetWidgetAsInvalidInput and of LineEditValidatable: `border: 1px solid red;` (a bare
        //! declaration, EMStudioManager.cpp:262, LineEditValidatable.cpp:46) over a line edit's own box (LineEdit.qss:11-19,
        //! BaseStyleSheet.qss:67-82): the same box and fill in every state, the 1px line replacing the 2px one, and the fill of
        //! :disabled (LineEdit.qss:28-35), which the sheet's declarations do not outrank
        const BoxElement InvalidInput{
            .m_name = "EMotionFX.Box.InvalidInput",
            .m_padding = InputFieldPadding,
            .m_nativeBorder = false,                                        // a border of its own
            .m_radius = Field::Radius,                                      // BaseStyleSheet.qss:79
            .m_size = QSize(BoxElement::Unset, Field::Height),              // LineEdit.qss:13
            .m_idle = { .m_fill = QBrush(Field::Fill), .m_edges = InvalidInputEdges },
            .m_stateFaces = {
                { BoxElement::Disabled, { .m_fill = QBrush(Field::DisabledFill), .m_edges = InvalidInputEdges } },
            },
        };

        //! `background-color: rgb(70, 70, 70);` on the name field of a disabled time track (TrackHeaderWidget.cpp:247): the field's own
        //! box (the application's 2px line, LineEdit.qss:16, BaseStyleSheet.qss:81) with the fill of the sheet, in every state
        const BoxElement DisabledTrackName{
            .m_name = "EMotionFX.Box.DisabledTrackName",
            .m_padding = InputFieldPadding,
            .m_nativeBorder = false,
            .m_radius = Field::Radius,
            .m_size = QSize(BoxElement::Unset, Field::Height),
            .m_idle = { .m_fill = QBrush(Values::DisabledNameEdit), .m_edges = BoxElement::border(InputFieldLineWidth, Field::Line) },
        };

        //! The name fields of the rename dialogs and of a time track, while a state holds: named first, then the tag. The invalid look
        //! is registered before the disabled track's: a sheet replaced the one before it, so an invalid name outranks the track's fill
        bool isInvalidInputField(Widget widget)
        {
            return named(widget, InputFieldName) && tagged(widget, InvalidInputTag);
        }

        bool isDisabledTrackName(Widget widget)
        {
            return named(widget, InputFieldName) && tagged(widget, DisabledTrackTag);
        }

        //! The fields of LineEditValidatable and of what derives from it: the row is on the class, so the tag is the only test
        bool isInvalidValidatable(Widget widget)
        {
            return tagged(widget, InvalidInputTag);
        }

        // ------------------------------------------------------------------------------
        // The keyboard shortcut dialog's labels, and their text while the key is taken
        // ------------------------------------------------------------------------------

        //! `color: rgb(244,156,28);` (KeyboardShortcutsWindow.cpp:426, :427) over the label rule's family and size (Text.qss:91-92).
        //! An element replaces the Global label text, so it restates them
        const TextElement ShortcutConflict{
            .m_name = "EMotionFX.Text.ShortcutConflict",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_color = Values::ConflictText,
        };

        //! The labels' own text colour, `color: white` of '*' (BaseStyleSheet.qss:18), which the labels go back to when the key is free
        //! (KeyboardShortcutsWindow.cpp:419, :420 cleared the sheet): the palette is not given back by itself
        const TextElement ShortcutIdle{
            .m_name = "EMotionFX.Text.ShortcutIdle",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_color = Foundations::Surface::Text,
        };

        bool isShortcutConflict(Widget widget)
        {
            return named(widget, ShortcutLabelName) && tagged(widget, ConflictTag);
        }

        bool isShortcutLabel(Widget widget)
        {
            return named(widget, ShortcutLabelName);
        }

        // ------------------------------------------------------------------------------
        // The error report dialog: `background-color: rgb(30, 30, 30);` on the dialog (MainWindow.cpp:138)
        // ------------------------------------------------------------------------------

        //! The dialog's fill. The sheet had no selector, so it also filled the text edit's margin and field and the lanes of its scroll
        //! bars (ErrorWindow sets the colour of the text edit itself, ScrollBar::setBackgroundColor)
        const BoxElement ErrorWindowBox{
            .m_name = "EMotionFX.Box.ErrorWindow",
            .m_idle = { .m_fill = QBrush(Values::ErrorWindowBackground) },
        };

        //! The Global popup, over which a look gives another surface or another text colour: the Global element is private to
        //! MenuElement.cpp, so a copy is made when first needed
        MenuElement globalMenu(const char* name)
        {
            MenuElement element = MenuElement::forWidget(nullptr);
            element.m_name = name;
            return element;
        }

        //! The text edit's standard context menu, a QMenu the text edit parents (QTextEdit::contextMenuEvent): the dialog's sheet
        //! reached it as a descendant, and its `background-color` outranked `QMenu { background-color: #222222 }` (Menu.qss:12)
        const MenuElement& errorWindowMenu()
        {
            static const MenuElement menu = []
            {
                MenuElement element = globalMenu("EMotionFX.Menu.ErrorWindow");
                element.m_surface = Values::ErrorWindowBackground;
                return element;
            }();
            return menu;
        }

        //! The menus inside an error window, as the sheet reached its descendants (`Class W`). The menu's parent is the text edit
        bool isErrorWindowMenu(Widget menu)
        {
            return qobject_cast<const QTextEdit*>(menu->parentWidget())
                && withinClass(menu, EMStudio::ErrorWindow::staticMetaObject.className());
        }

        // ------------------------------------------------------------------------------
        // The notification window's icon, the recover files dialog's check boxes, the reset workspace dialog's title strip
        // ------------------------------------------------------------------------------

        //! `#NotificationIcon{ background-color: transparent; border: none; }` (NotificationWindow.cpp:55): no border, and the
        //! same transparent look in every state (the inline rule outranks the tool button rules)
        const BoxElement NotificationIcon{
            .m_name = "EMotionFX.Box.NotificationIcon",
            .m_nativeBorder = false,                                        // border: none
            .m_idle = { .m_fill = QBrush(Foundations::Clear) },
        };

        bool isNotificationIcon(Widget widget)
        {
            return named(widget, "NotificationIcon");
        }

        //! `background: transparent; padding-left: 3px; max-width: 13px;` (RecoverFilesWindow.cpp:118): the box of the check box of a
        //! file is at most 16 wide, and the indicator sits inside its padding. The fill was transparent already (CheckBox.qss:11)
        const BoxElement RowCheckBox{
            .m_name = "EMotionFX.Box.RowCheckBox",
            .m_padding = QMargins(RecoverCheckPaddingLeft, 0, 0, 0),
            .m_maxSize = QSize(RecoverCheckMaxWidth, BoxElement::Unset),
            .m_idle = { .m_fill = QBrush(Foundations::Clear) },
        };

        bool isRecoverCheck(Widget widget)
        {
            return named(widget, RecoverCheckName);
        }

        //! `background-color: rgb(40, 40, 40); padding: 6px;` (ResetSettingsDialog.cpp:65): the strip behind the title, over the label
        //! rule's vertical margin (Text.qss:93-94), which the box of a label has
        const BoxElement DialogTopLabel{
            .m_name = "EMotionFX.Box.DialogTopLabel",
            .m_margin = QMargins(0, Label::MarginV, 0, Label::MarginV),
            .m_padding = QMargins(ResetSettingsTopLabelPadding, ResetSettingsTopLabelPadding, ResetSettingsTopLabelPadding,
                ResetSettingsTopLabelPadding),
            .m_idle = { .m_fill = QBrush(Values::ResetSettingsTopLabelBackground) },
        };

        bool isDialogTopLabel(Widget widget)
        {
            return named(widget, DialogTopLabelName);
        }

        // ------------------------------------------------------------------------------
        // The bold labels: `font-weight: bold;` (AnimGraphEditor.cpp:58, ContentHeaderWidget.cpp:21, SimulatedObjectColliderWidget.cpp:56,
        // :78, SimulatedJointWidget.cpp:407)
        // ------------------------------------------------------------------------------

        //! The weight of the sheet's `bold` (QFont::Bold) over the label rule's family and size (Text.qss:91-92)
        const TextElement Bold{
            .m_name = "EMotionFX.Text.Bold",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_weight = QFont::Bold,
        };

        bool isBoldLabel(Widget widget)
        {
            return named(widget, BoldLabelName);
        }

        // ------------------------------------------------------------------------------
        // The motion set combo box of the anim graph editor: `border: 1px solid orange;` while no motion set is chosen
        // (AnimGraphEditor.cpp:239, :255)
        // ------------------------------------------------------------------------------

        //! The line of the warning, in every state: the inline declaration outranks the :hover and :focus rules (ComboBox.qss:22-42)
        const BoxElement::Edges WarningEdges = BoxElement::border(WarningLineWidth, Values::WarningBorder);

        //! :focus, and an open popup (ComboBox.qss:31-42), with the orange line in place of the light blue one
        const BoxElement::Face WarningFocusFace{
            .m_fill = QBrush(Field::FocusFill),
            .m_edges = WarningEdges,
            .m_text = Field::Text,
            .m_margin = ComboStateMargin,
            .m_radius = Field::FocusRadius,
        };

        //! The combo box of ComboBox.qss:13-57 over the entry field (BaseStyleSheet.qss:67-82), as Global.Box.ComboBox restates them,
        //! with the orange line in each state
        const BoxElement MotionSetComboWarning{
            .m_name = "EMotionFX.Box.MotionSetComboWarning",
            .m_margin = ComboMargin,
            .m_padding = ComboPadding,
            .m_nativeBorder = false,                                        // BaseStyleSheet.qss:79-80, and the line
            .m_radius = Field::Radius,
            .m_size = QSize(BoxElement::Unset, Field::Height),
            .m_idle = { .m_fill = QBrush(Field::Fill), .m_edges = WarningEdges, .m_text = Field::Text },
            .m_stateFaces = {
                { BoxElement::Hover, {
                    .m_fill = QBrush(Field::Fill),
                    .m_edges = WarningEdges,
                    .m_text = Field::Text,
                    .m_margin = ComboStateMargin,
                    .m_radius = Field::HoverRadius } },                     // ComboBox.qss:22-29
                { BoxElement::Focus, WarningFocusFace },
                { BoxElement::Checked, WarningFocusFace },                  // [HasPopupOpen=true]: ComboBox.qss:32-33
                { BoxElement::Disabled, { .m_fill = QBrush(Field::DisabledFill), .m_edges = WarningEdges, .m_text = Field::DisabledText } },
            },
        };

        //! The popup list of that combo box: the bare declaration reached the list (a QListView below the combo box's popup container)
        //! as well, over `QComboBox QAbstractItemView { background-color: #222222; padding: 0px }` (ComboBox.qss:80-86). Its items are
        //! the Global popup's (ComboPopupElement)
        const BoxElement MotionSetComboPopup{
            .m_name = "EMotionFX.Box.MotionSetComboPopupWarning",
            .m_hasBox = true,                                               // padding: 0px is declared (ComboBox.qss:82)
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(ComboPopupSurface), .m_edges = WarningEdges },
        };

        //! The combo box while it warns
        bool isMotionSetComboWarning(Widget widget)
        {
            return named(widget, MotionSetComboName) && tagged(widget, MotionSetWarningTag);
        }

        //! The list of the popup of that combo box: the list's parent is the popup container, whose parent is the combo box
        bool isMotionSetComboPopup(Widget widget)
        {
            const QWidget* container = widget->parentWidget();
            const QWidget* combo = container ? container->parentWidget() : nullptr;
            return qobject_cast<const QComboBox*>(combo) && isMotionSetComboWarning(combo);
        }

        // ------------------------------------------------------------------------------
        // The trees of the hierarchy and navigate widgets: `font-size: 11px; color: #e9e9e9;` (AnimGraphHierarchyWidget.cpp:76,
        // NavigateWidget.cpp:80)
        // ------------------------------------------------------------------------------

        //! The colour is put in the palettes of the tree and its viewport, which the item delegate draws its text from. The family
        //! and the weight are the universal rule's (BaseStyleSheet.qss:19-20). The header sections keep the 12px of TableView.qss:70-80:
        //! a rule with no type does not reach a sub-control
        const TextElement GraphTree{
            .m_name = "EMotionFX.Text.GraphTree",
            .m_pixelSize = 11,
            .m_color = Values::GraphTreeText,
        };

        bool isGraphTree(Widget widget)
        {
            return named(widget, GraphTreeName);
        }

        // ------------------------------------------------------------------------------
        // The parameter editor's preview frame: `QFrame#previewFrame { border: 2px dashed #979797; background-color: #85858580; }`
        // (ParameterCreateEditWidget.cpp:83)
        // ------------------------------------------------------------------------------

        //! A frame of the shape Box. The fill is Qt's reading of #85858580: #AARRGGBB (Values::PreviewBackground)
        const BoxElement ParameterPreviewFrame{
            .m_name = "EMotionFX.Box.ParameterPreviewFrame",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(Values::PreviewBackground),
                .m_edges = BoxElement::border(PreviewLineWidth, Values::PreviewBorder, BoxElement::LineStyle::Dashed),
            },
        };

        bool isPreviewFrame(Widget widget)
        {
            return named(widget, "previewFrame");
        }

        // ------------------------------------------------------------------------------
        // The command bar: the progress text and bar, the result field
        // ------------------------------------------------------------------------------

        //! `padding-right: 1px;` of the progress text (CommandBarPlugin.cpp:93), over the label rule's margin (Text.qss:93-94)
        const BoxElement CommandBarProgressTextBox{
            .m_name = "EMotionFX.Box.CommandBarProgressText",
            .m_margin = QMargins(0, Label::MarginV, 0, Label::MarginV),
            .m_padding = QMargins(0, 0, ProgressTextPaddingRight, 0),
            .m_idle = { .m_fill = QBrush(Foundations::Surface::Fill) },
        };

        //! `color: rgb(140, 140, 140);` of the progress text (CommandBarPlugin.cpp:93)
        const TextElement CommandBarProgressTextColor{
            .m_name = "EMotionFX.Text.CommandBarProgressText",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_color = Values::ProgressText,
        };

        bool isProgressText(Widget widget)
        {
            return named(widget, ProgressTextName);
        }

        //! `padding-right: 2px;` of the progress bar (CommandBarPlugin.cpp:101) over the track every progress bar has
        //! (ProgressBar.qss:12-18), as Global.Box.ProgressBar restates it: the chunk and the track stop 2px short
        const BoxElement CommandBarProgress{
            .m_name = "EMotionFX.Box.CommandBarProgress",
            .m_padding = QMargins(0, 0, ProgressBarPaddingRight, 0),
            .m_nativeBorder = false,                                        // ProgressBar.qss:12-13
            .m_idle = { .m_fill = QBrush(ProgressTrackFill) },              // ProgressBar.qss:14
            .m_properties = { { "textVisible", ProgressTextVisible } },     // ProgressBar.qss:18
        };

        bool isProgressBar(Widget widget)
        {
            return named(widget, ProgressBarName);
        }

        //! The result field's text after a command: `color: red;` / `color: rgb(0, 255, 0);` (CommandBarPlugin.cpp:177, :183), over the
        //! field's family and size (BaseStyleSheet.qss:78, :82). The box of the field is the Global one. A line edit draws its text
        //! with roles that every polish builds again, so no element is needed for the field's own colour
        const TextElement CommandResultError{
            .m_name = "EMotionFX.Text.CommandResultError",
            .m_family = Field::Family,
            .m_pixelSize = Field::Size,
            .m_color = Values::ResultErrorText,
        };

        const TextElement CommandResultSuccess{
            .m_name = "EMotionFX.Text.CommandResultSuccess",
            .m_family = Field::Family,
            .m_pixelSize = Field::Size,
            .m_color = Values::ResultSuccessText,
        };

        //! The tip of the result field (CommandBarPlugin.cpp:179 sets it on an error) and its standard context menu reach the colour as
        //! well: `color` outranks ToolTip.qss:12 for the tip, and the text of the menu's items takes it. The tip's own family and size
        //! (ToolTip.qss:16, :19) are kept
        const TextElement CommandResultErrorTipText{
            .m_name = "EMotionFX.Text.CommandResultErrorTip",
            .m_family = ToolTipFamily,
            .m_pixelSize = ToolTipSize,
            .m_color = Values::ResultErrorText,
        };

        const TextElement CommandResultSuccessTipText{
            .m_name = "EMotionFX.Text.CommandResultSuccessTip",
            .m_family = ToolTipFamily,
            .m_pixelSize = ToolTipSize,
            .m_color = Values::ResultSuccessText,
        };

        const ToolTipElement CommandResultErrorToolTip{
            .m_name = "EMotionFX.ToolTip.CommandResultError",
            .m_text = &CommandResultErrorTipText,
        };

        const ToolTipElement CommandResultSuccessToolTip{
            .m_name = "EMotionFX.ToolTip.CommandResultSuccess",
            .m_text = &CommandResultSuccessTipText,
        };

        const MenuElement& commandResultErrorMenu()
        {
            static const MenuElement menu = []
            {
                MenuElement element = globalMenu("EMotionFX.Menu.CommandResultError");
                element.m_text = Values::ResultErrorText;
                return element;
            }();
            return menu;
        }

        const MenuElement& commandResultSuccessMenu()
        {
            static const MenuElement menu = []
            {
                MenuElement element = globalMenu("EMotionFX.Menu.CommandResultSuccess");
                element.m_text = Values::ResultSuccessText;
                return element;
            }();
            return menu;
        }

        //! The result field after a command failed, and after it succeeded: named first, then the tag
        bool isResultError(Widget widget)
        {
            return named(widget, ResultEditName) && tagged(widget, ResultErrorTag);
        }

        bool isResultSuccess(Widget widget)
        {
            return named(widget, ResultEditName) && tagged(widget, ResultSuccessTag);
        }

        //! The standard context menu of the result field, a QMenu the field parents (QLineEdit::contextMenuEvent)
        bool isResultErrorMenu(Widget menu)
        {
            const QWidget* owner = menu->parentWidget();
            return qobject_cast<const QLineEdit*>(owner) && isResultError(owner);
        }

        bool isResultSuccessMenu(Widget menu)
        {
            const QWidget* owner = menu->parentWidget();
            return qobject_cast<const QLineEdit*>(owner) && isResultSuccess(owner);
        }

        // ------------------------------------------------------------------------------
        // The 'Motion Extraction' check box: a caret in place of the check mark (MotionExtractionWindow.cpp:167)
        // ------------------------------------------------------------------------------

        //! `QCheckBox::indicator { width: 16px; height: 16px; border: none; margin: 0px; }` and the images of :checked and :unchecked,
        //! each with :disabled and :focus (the plain entries cover them: the sheet gives the same image in those states)
        const IndicatorElement CaretCheck{
            .m_name = "EMotionFX.Indicator.CaretCheck",
            .m_imageSize = IndicatorElement::ImageSize,                     // MotionExtractionWindow.cpp:155-156 (rest and focused)
            .m_margin = CaretCheckMargin,
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = CaretCheckMargin,
            .m_stateImages = {
                { IndicatorElement::Checked, CaretDownImage },
                { IndicatorElement::Unchecked, CaretRightImage },
            },
        };

        bool isCaretCheck(Widget widget)
        {
            return named(widget, CaretCheckName);
        }

        // ------------------------------------------------------------------------------
        // The dialog stack's splitters: the gap between two cards of a stack (DialogStack.cpp:35, :146, :176, :383, :469)
        // ------------------------------------------------------------------------------

        //! `QSplitter::handle{ height: 4px; background: transparent; }`: a transparent handle shows nothing, in every state (the
        //! inline rule outranked Splitter.qss:9-24, hover included)
        const SplitterElement DialogHandleOpen{
            .m_name = "EMotionFX.Splitter.DialogHandleOpen",
            .m_thickness = DialogStackOpenGap,
            .m_fill = Foundations::Clear,
        };

        //! The same with `height: 1px`, while the dialog above the handle is closed
        const SplitterElement DialogHandleClosed{
            .m_name = "EMotionFX.Splitter.DialogHandleClosed",
            .m_thickness = DialogStackClosedGap,
            .m_fill = Foundations::Clear,
        };

        //! A dialog stack's splitter has no class of its own (DialogStackSplitter has no Q_OBJECT): it is named, and a closed dialog
        //! tags it. A stack's splitter is open until a site closes it
        bool isDialogStackSplitterClosed(Widget widget)
        {
            return named(widget, MysticQt::DialogStackSplitterName) && tagged(widget, MysticQt::DialogStackClosedTag);
        }

        bool isDialogStackSplitter(Widget widget)
        {
            return named(widget, MysticQt::DialogStackSplitterName);
        }

        // ------------------------------------------------------------------------------
        // The notification widget's header: `background-color: rgb(60, 60, 60);` (NotificationWidget.cpp:35)
        // ------------------------------------------------------------------------------

        //! A frame of the shape StyledPanel with no border of its own, over the panel fill the frame rule gives (BaseStyleSheet.qss:41-48)
        const FrameElement NotificationHeader{
            .m_name = "EMotionFX.Frame.NotificationHeader",
            .m_fill = Values::NotificationHeaderBackground,
            .m_fillShapes = FrameElement::PanelShapes,
        };

        //! The frame is named HeaderFrame, which other frames of the engine are too (NewEditorStylesheet.qss, PropertyEditorElements):
        //! the constructor tags it
        bool isNotificationHeader(Widget widget)
        {
            return named(widget, "HeaderFrame") && tagged(widget, NotificationHeaderTag);
        }

        //! The bare sheet filled the icon and the title below the header with the same colour, inside the margin the label rule gives
        //! every label (Text.qss:93-94). The fills lie over the line of the frame where the two meet at a device ratio that is not a
        //! whole number, so the line of the legacy header is a half pixel thinner on its right edge at 150 %: the labels draw it too
        const BoxElement NotificationHeaderLabel{
            .m_name = "EMotionFX.Box.NotificationHeaderLabel",
            .m_margin = QMargins(0, Label::MarginV, 0, Label::MarginV),
            .m_idle = { .m_fill = QBrush(Values::NotificationHeaderBackground) },
        };

        //! The labels Icon and Title of the header (NotificationWidget.cpp names them), by their own name first: CardHeader has labels
        //! of both names too, so the header is the second test
        bool isNotificationHeaderLabel(Widget widget)
        {
            if (!named(widget, "Icon") && !named(widget, "Title"))
            {
                return false;
            }
            const QWidget* header = widget->parentWidget();
            return header && isNotificationHeader(header);
        }

        // ------------------------------------------------------------------------------
        // The blend space motion's position: a bold red X and a bold green Y (BlendSpaceMotionContainerHandler.cpp:51), and the motion's
        // name with a line while the motion set does not contain it (:152)
        // ------------------------------------------------------------------------------

        //! `QLabel { font-weight: bold; color : red; }`. The colour string of the site has two values, so the two looks are two elements
        const TextElement AxisX{
            .m_name = "EMotionFX.Text.AxisX",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_weight = QFont::Bold,
            .m_color = Values::AxisXText,
        };

        const TextElement AxisY{
            .m_name = "EMotionFX.Text.AxisY",
            .m_family = Label::Family,
            .m_pixelSize = Label::Size,
            .m_weight = QFont::Bold,
            .m_color = Values::AxisYText,
        };

        bool isAxisX(Widget widget)
        {
            return named(widget, AxisXName);
        }

        bool isAxisY(Widget widget)
        {
            return named(widget, AxisYName);
        }

        //! `#m_labelMotion { border: 1px solid red; }` over the label rule's vertical margin (Text.qss:93-94): the label shrinks by the
        //! line on each side
        const BoxElement InvalidMotionLabel{
            .m_name = "EMotionFX.Box.InvalidMotionLabel",
            .m_margin = QMargins(0, Label::MarginV, 0, Label::MarginV),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
                .m_edges = BoxElement::border(Values::InvalidMotionLineWidth, Values::InvalidMotionLine),
            },
        };

        //! The label of a motion's name (it is named by BlendSpaceMotionWidget) while the motion is invalid
        bool isInvalidMotionLabel(Widget widget)
        {
            return named(widget, "m_labelMotion") && tagged(widget, InvalidMotionTag);
        }
    } // namespace EMStudioFlatSheetInternal

    // ----------------------------------------------------------------------------------
    // Registration: the first registered row that matches a widget wins, so the narrower selector comes first
    // ----------------------------------------------------------------------------------

    void RegisterElements()
    {
        using namespace EMStudioFlatSheetInternal;

        // The registries last for the process: a second call would only add the same rows again
        static bool registered = false;
        if (registered)
        {
            return;
        }
        registered = true;

        // ---- text: labels
        TextElement::registerElement(&ShortcutConflict, &QLabel::staticMetaObject, &isShortcutConflict);
        TextElement::registerElement(&ShortcutIdle, &QLabel::staticMetaObject, &isShortcutLabel);
        TextElement::registerElement(&Bold, &QLabel::staticMetaObject, &isBoldLabel);
        TextElement::registerElement(&CommandBarProgressTextColor, &QLabel::staticMetaObject, &isProgressText);
        TextElement::registerElement(&AxisX, &QLabel::staticMetaObject, &isAxisX);
        TextElement::registerElement(&AxisY, &QLabel::staticMetaObject, &isAxisY);

        // ---- text: the result field and the trees
        TextElement::registerElement(&CommandResultError, &QLineEdit::staticMetaObject, &isResultError);
        TextElement::registerElement(&CommandResultSuccess, &QLineEdit::staticMetaObject, &isResultSuccess);
        TextElement::registerElement(&GraphTree, &QTreeView::staticMetaObject, &isGraphTree);

        // ---- boxes: the input fields (the invalid look before the disabled track's), and the validatable field on its own class
        BoxElement::registerElement(&InvalidInput, &QLineEdit::staticMetaObject, &isInvalidInputField);
        BoxElement::registerElement(&DisabledTrackName, &QLineEdit::staticMetaObject, &isDisabledTrackName);
        BoxElement::registerElement(&InvalidInput, &EMStudio::LineEditValidatable::staticMetaObject, &isInvalidValidatable);

        // ---- boxes: the dialogs
        BoxElement::registerElement(&ErrorWindowBox, &EMStudio::ErrorWindow::staticMetaObject, [](Widget) { return true; });
        BoxElement::registerElement(&NotificationIcon, &QToolButton::staticMetaObject, &isNotificationIcon);
        BoxElement::registerElement(&RowCheckBox, &QCheckBox::staticMetaObject, &isRecoverCheck);
        BoxElement::registerElement(&DialogTopLabel, &QLabel::staticMetaObject, &isDialogTopLabel);

        // ---- boxes: the anim graph editor, the parameter editor, the command bar and the blend space motions
        BoxElement::registerElement(&MotionSetComboWarning, &QComboBox::staticMetaObject, &isMotionSetComboWarning);
        BoxElement::registerElement(&MotionSetComboPopup, &QListView::staticMetaObject, &isMotionSetComboPopup);
        BoxElement::registerElement(&ParameterPreviewFrame, &QFrame::staticMetaObject, &isPreviewFrame);
        BoxElement::registerElement(&CommandBarProgressTextBox, &QLabel::staticMetaObject, &isProgressText);
        BoxElement::registerElement(&CommandBarProgress, &QProgressBar::staticMetaObject, &isProgressBar);
        BoxElement::registerElement(&InvalidMotionLabel, &QLabel::staticMetaObject, &isInvalidMotionLabel);
        BoxElement::registerElement(&NotificationHeaderLabel, &QLabel::staticMetaObject, &isNotificationHeaderLabel);

        // ---- frames, indicators, splitters
        FrameElement::registerElement(&NotificationHeader, &QFrame::staticMetaObject, &isNotificationHeader);
        IndicatorElement::registerElement(&CaretCheck, &QCheckBox::staticMetaObject, &isCaretCheck);
        SplitterElement::registerElement(&DialogHandleClosed, &QSplitter::staticMetaObject, &isDialogStackSplitterClosed);
        SplitterElement::registerElement(&DialogHandleOpen, &QSplitter::staticMetaObject, &isDialogStackSplitter);

        // ---- menus and tips that a bare sheet reached through their owner
        MenuElement::registerElement(&errorWindowMenu(), &QMenu::staticMetaObject, &isErrorWindowMenu);
        MenuElement::registerElement(&commandResultErrorMenu(), &QMenu::staticMetaObject, &isResultErrorMenu);
        MenuElement::registerElement(&commandResultSuccessMenu(), &QMenu::staticMetaObject, &isResultSuccessMenu);
        ToolTipElement::registerElement(&CommandResultErrorToolTip, &QLineEdit::staticMetaObject, &isResultError);
        ToolTipElement::registerElement(&CommandResultSuccessToolTip, &QLineEdit::staticMetaObject, &isResultSuccess);
    }
} // namespace EMStudioFlat
