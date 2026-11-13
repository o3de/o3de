/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorFlatSheet.h>
#include <EditorFlatValues.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzToolsFramework/AssetEditor/AssetEditorHeader.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMargins>
#include <QPlainTextEdit>
#include <QSize>
#include <QToolBar>
#include <QTreeView>
#include <QTreeWidget>
#include <QWidget>

namespace EditorFlatSheet
{
    using AzQtComponents::BoxElement;
    using AzQtComponents::FrameElement;
    using AzQtComponents::ItemViewElement;
    using AzQtComponents::TextElement;
    using AzQtComponents::ToolTipElement;
    using AzQtComponents::Selectors::childOf;
    using AzQtComponents::Selectors::childOfClass;
    using AzQtComponents::Selectors::hasProperty;
    using AzQtComponents::Selectors::named;
    using AzQtComponents::Selectors::within;
    using AzQtComponents::Selectors::withinClass;
    using AzQtComponents::Selectors::withinTagged;

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one
    //! translation unit, where anonymous namespaces merge.
    namespace EditorFlatSheetInternal
    {
        // ------------------------------------------------------------------------------
        // Elements. Each value cites the declaration that grants it, as file:line of Editor.qss or of the sheet beneath it
        // (AzQtComponents Components/Widgets/). The widgets Editor.qss shaped that a style sheet of their own shapes as well
        // are not here: that sheet still runs in the flattened GUI, and it is what shapes them.
        // ------------------------------------------------------------------------------

        //! The status bar of the Asset Editor (AssetEditorWidget.cpp:205 names it), a plain QWidget: a line on its top edge
        //! (`#AssetEditorStatusBar`, Editor.qss:180-183) over the transparent background of '*' (BaseStyleSheet.qss:17).
        const BoxElement AssetEditorStatusBar{
            .m_name = "Editor.Box.AssetEditorStatusBar",
            .m_nativeBorder = false,                                                // border-top is a border of its own (Editor.qss:182)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // BaseStyleSheet.qss:17
                .m_edges = { { 2, QColor(0x11, 0x11, 0x11) }, {}, {}, {} },         // Editor.qss:182. TOKENIZE: AssetEditorStatusBarBorder
            },
        };

        //! The row of the asset browser that holds the selection chip (AzAssetBrowserWindow.ui:377), a QFrame without a frame:
        //! a dark fill and no border (`AzAssetBrowserWindow #searchButtonContainer`, Editor.qss:20-24).
        const BoxElement AssetBrowserSearchButtons{
            .m_name = "Editor.Box.AssetBrowserSearchButtons",
            .m_nativeBorder = false,                                                // border: 0 none (Editor.qss:23)
            .m_idle = {
                .m_fill = QBrush(QColor(0x14, 0x14, 0x14)),                         // Editor.qss:22. TOKENIZE: AssetBrowserSearchButtonsFill
            },
        };

        //! The 1px line under that row (AzAssetBrowserWindow.ui:411), a plain QWidget: a dark fill
        //! (`AzAssetBrowserWindow #m_separator`, Editor.qss:26-29).
        const BoxElement AssetBrowserSeparator{
            .m_name = "Editor.Box.AssetBrowserSeparator",
            .m_idle = {
                .m_fill = QBrush(QColor(0x22, 0x22, 0x22)),                         // Editor.qss:28. TOKENIZE: AssetBrowserSeparatorFill
            },
        };

        //! The tree of the Add Component popup (`AzToolsFramework--ComponentPaletteWidget > QTreeView { background-color: #222222 }`,
        //! Editor.qss:38-41), a QTreeView named Tree (ComponentPaletteWidget.ui). The rule fills the whole scroll area, so the lane of its scroll
        //! bar shows the fill too: n2 03_add_component_palette measures #222222 on the vertical lane (the container of the bar, 8px wide at
        //! the tree's right edge) where f5 shows the popup's #383a3b through it, the viewport only having the fill by the palette that
        //! ComponentPaletteWidget.cpp:88-107 gives the tree. A margin of 0 as every scroll area has (QDockWidget.qss:35), no border
        //! (TableView.qss:28). TOKENIZE: ComponentPaletteTreeFill
        const BoxElement ComponentPaletteTree{
            .m_name = "Editor.Box.ComponentPaletteTree",
            .m_hasBox = true,                                                       // margin: 0, QDockWidget.qss:35 (QAbstractScrollArea)
            .m_nativeBorder = false,                                                // border: none, TableView.qss:28 (QTreeView)
            .m_idle = {
                .m_fill = QBrush(QColor(0x22, 0x22, 0x22)),                         // Editor.qss:40. TOKENIZE: ComponentPaletteTreeFill
            },
        };

        //! The node palettes of the Script Canvas and the Landscape Canvas (`GraphCanvas--NodePaletteTreeView { selection-background-color:
        //! rgba(255, 255, 255, 41) }`, Editor.qss:109-113: "Set the Script Canvas search highlight color"): the role of the tree's selection,
        //! which the highlight of a search match is drawn with. The fill of a selected item stays the Global one, rgb(101,101,101), whose
        //! `::item:selected` rule outranks the role (TableView.qss:42-50). The role is Native's (the n2 palette dumps of both node palettes hold
        //! Highlight #29ffffff, the sheet's rgba(255,255,255,41)).
        //! TOKENIZE: NodePaletteSearchHighlight
        const ItemViewElement NodePaletteTree{
            .m_name = "Editor.ItemView.NodePaletteTree",
            .m_selection = QColor(255, 255, 255, 41),                               // selection-background-color: Editor.qss:112
        };

        //! The template list of the New Level dialog: the dialog sets `QListWidget::item { height: 148px; padding-left: 0px;
        //! padding-right: 0px; background-color: transparent }` on itself in the legacy GUI (NewLevelDialog.cpp:84,
        //! StyleManager::setLegacyStyleSheet). The flattened GUI sets no sheet, and this element is the whole of those items. A sheet set
        //! on a widget outranks the application's whatever the specificity of their rules (the depth of the sheet is weighed before the
        //! specificity: QCss::StyleSelector::matchRule), so the items are 148px high with no padding and no fill in any state: the
        //! transparent background beats the application's `::item:selected` and `::item:hover` rules (TableView.qss:42-60). The Global
        //! list's items have 4px of padding at the left (TableView.qss:31-40), which this rule took away. TOKENIZE: NewLevelTemplateItem
        const ItemViewElement NewLevelTemplateList{
            .m_name = "Editor.ItemView.NewLevelTemplateList",
            .m_itemHeight = 148,                                                    // height: NewLevelDialog.cpp:84
            .m_itemPadding = QMargins(0, 0, 0, 0),                                  // padding-left, padding-right: NewLevelDialog.cpp:84
            .m_faces = {
                { ItemViewElement::Selected, { QColor(0, 0, 0, 0), QColor(), QColor() } },  // background-color: transparent over ::item:selected
                { ItemViewElement::Hover, { QColor(0, 0, 0, 0), QColor(), QColor() } },     // ... and over ::item:hover
            },
        };

        // ------------------------------------------------------------------------------
        // Matchers: the selectors that gave each rule its widgets
        // ------------------------------------------------------------------------------

        //! `#AssetEditorStatusBar` (Editor.qss:180)
        bool isAssetEditorStatusBar(const QWidget* widget)
        {
            return named(widget, "AssetEditorStatusBar");
        }

        //! `AzAssetBrowserWindow #searchButtonContainer` (Editor.qss:20): that name, below the asset browser window
        bool isAssetBrowserSearchButtons(const QWidget* widget)
        {
            return named(widget, "searchButtonContainer") && withinClass(widget, "AzAssetBrowserWindow");
        }

        //! `AzAssetBrowserWindow #m_separator` (Editor.qss:26): that name, below the asset browser window
        bool isAssetBrowserSeparator(const QWidget* widget)
        {
            return named(widget, "m_separator") && withinClass(widget, "AzAssetBrowserWindow");
        }

        //! `AzToolsFramework--ComponentPaletteWidget > QTreeView` (Editor.qss:38): a tree whose parent is the palette widget
        bool isComponentPaletteTree(const QWidget* widget)
        {
            return childOfClass(widget, "AzToolsFramework::ComponentPaletteWidget");
        }

        //! `GraphCanvas--NodePaletteTreeView` (Editor.qss:110): the tree itself, or one of a class derived from it
        bool isNodePaletteTree(const QWidget* widget)
        {
            return widget->inherits("GraphCanvas::NodePaletteTreeView");
        }

        //! The `QListWidget::item` rule that the New Level dialog sets on itself (NewLevelDialog.cpp:84): any list widget below that dialog
        bool isNewLevelTemplateList(const QWidget* widget)
        {
            return withinClass(widget, "CNewLevelDialog");
        }

        // ------------------------------------------------------------------------------
        // The Asset Editor's header (Ui::AssetEditorHeader, AssetEditorHeader.cpp): Editor.qss:56-91
        //
        // The header is a QFrame holding one QFrame, #Background, which lays out an icon label (#Icon, 16 x 16, shown once a tab has
        // an asset: AssetEditorTab::SetupHeader) and the name, an ElidingLabel (#Name) that shows where the asset is ("Asset
        // location: " in bold, then its folder). An ElidingLabel gives its name to the hidden label it measures its hint with
        // (ElidingLabel::setObjectName), so the rules of #Name reached that label too.
        //
        // Editor.qss is the main window's sheet (CryEdit.cpp:1571). A widget's sheet is deeper than the application's, and the depth
        // weighs above the specificity (QCss::StyleSelector::matchRule), so these rules outrank the application's: of the label rule
        // (Text.qss:89-95) the labels keep what the header's rules leave alone, the family, the size and the margins.
        //
        // Held as declared, applied by Qt to none of these widgets: `height: 16px` (:65, :81, :88). A height sizes a widget only
        // through sizeFromContents, which neither a QFrame nor a QLabel asks for, and QStyleSheetStyle::setGeometry sets only the
        // min- and max- limits; an element's m_size does the same (BoxElement::adjustSize,
        // applyLimits). `background-image: none` (:75) is no image, as no declaration is. `#Location` (:86-91) reaches no widget:
        // the header names none so (AssetEditorHeader.cpp:17-19).
        // ------------------------------------------------------------------------------

        //! The box of every label (`QLabel { margin-top: 1px; margin-bottom: 1px }`, Text.qss:93-94), which the labels below the
        //! header keep under its rules
        constexpr QMargins HeaderLabelMargin(
            0, AzQtComponents::Foundations::Label::MarginV, 0, AzQtComponents::Foundations::Label::MarginV);

        //! `Ui--AssetEditorHeader { margin: 0px; padding: 0px }` (:57-61): the header, a box with nothing around its contents, over
        //! the transparent background of '*' (BaseStyleSheet.qss:17). It has no frame, so its contents fill it, as Qt had them.
        const BoxElement AssetEditorHeader{
            .m_name = "Editor.Box.AssetEditorHeader",
            .m_hasBox = true,                                                       // margin: 0px; padding: 0px (:59-60)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // BaseStyleSheet.qss:17
            },
        };

        //! `Ui--AssetEditorHeader #Background` (:69-77): the bar, a QFrame without a frame. The padding and the transparent border
        //! (1px at the top, left and right: `border-bottom: none` makes the bottom edge 0 wide, QRenderRule::fixupBorder) put the
        //! icon and the name 8px in from the sides and 3px from the top and the bottom. The fill covers the border too (a background
        //! is clipped to the border box unless a rule says otherwise) and replaces the fill the frame asks for
        //! (AssetEditorHeader.cpp:34). `Ui--AssetEditorHeader *` (:63-67) gave the bar the same fill, which this rule restates.
        //! TOKENIZE: AssetEditorHeaderFill, AssetEditorHeaderPadding
        const BoxElement AssetEditorHeaderBackground{
            .m_name = "Editor.Box.AssetEditorHeaderBackground",
            .m_padding = QMargins(7, 2, 7, 3),                                      // padding: 2px 7px 3px 7px (:72)
            .m_hasBox = true,                                                       // margin: 0px (:71)
            .m_nativeBorder = false,                                                // border: 1px solid transparent (:73)
            .m_size = QSize(-1, 16),                                                // height: :65 (not applied)
            .m_idle = {
                // background-color: Editor.qss:76 (and :66). TOKENIZE: AssetEditorHeaderFill
                .m_fill = QBrush(QColor(0x27, 0x27, 0x27)),
                // border: 1px solid transparent (:73); border-bottom: none (:74)
                .m_edges = { { 1, QColor(Qt::transparent) }, { 1, QColor(Qt::transparent) }, {}, { 1, QColor(Qt::transparent) } },
            },
        };

        //! `Ui--AssetEditorHeader #Name` (:79-84): the name, and the label it measures its hint with. The transparent background
        //! (:83) outranks the header's fill of :66, so the bar shows through; the box is every label's.
        const BoxElement AssetEditorHeaderName{
            .m_name = "Editor.Box.AssetEditorHeaderName",
            .m_margin = HeaderLabelMargin,                                          // Text.qss:93-94
            .m_size = QSize(-1, 16),                                                // height: :81 (not applied)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // background-color: :83
            },
        };

        //! `Ui--AssetEditorHeader *` (:63-67) on the icon label, the one widget below the header that no rule of its own names: the
        //! header's fill, drawn over the bar's same fill, in the box of every label. TOKENIZE: AssetEditorHeaderFill
        const BoxElement AssetEditorHeaderLabel{
            .m_name = "Editor.Box.AssetEditorHeaderLabel",
            .m_margin = HeaderLabelMargin,                                          // Text.qss:93-94
            .m_size = QSize(-1, 16),                                                // height: :65 (not applied)
            .m_idle = {
                .m_fill = QBrush(QColor(0x27, 0x27, 0x27)),                         // Editor.qss:66. TOKENIZE: AssetEditorHeaderFill
            },
        };

        //! The font of the name: `font: italic` (:82) over the label rule's family and size (Text.qss:91-92). The shorthand sets the
        //! weight too, to normal (QCss::ValueExtractor::extractFont): the universal rule's 400 (BaseStyleSheet.qss:20). The bold of
        //! "Asset location: " is the text's own (AssetEditorTab::SetupHeader). TOKENIZE: AssetEditorHeaderNameStyle
        const TextElement AssetEditorHeaderNameText{
            .m_name = "Editor.Text.AssetEditorHeaderName",
            .m_family = AzQtComponents::Foundations::Label::Family,                 // Text.qss:91
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,                // Text.qss:92
            .m_weight = 400,                                                        // QFont::Normal, by `font: italic` (:82)
            .m_italic = 1,                                                          // font: italic (:82)
        };

        //! `Ui--AssetEditorHeader *` (:63-67) on a tooltip. The sheet style resolves a tip's rules through the widget it is shown for
        //! (the tip's `_q_stylesheet_parent`, which QStyleSheetStyleSelector reads for its parent), so the rule reached the tip of the
        //! header and of every widget below it (the name shows the whole location as its tip once it is elided: ElidingLabel::elide).
        //! The main window's sheet outranks ToolTip.qss's black there, so the tip is filled with the header's colour; its text stays
        //! ToolTip.qss's. TOKENIZE: AssetEditorHeaderFill
        const ToolTipElement AssetEditorHeaderToolTip{
            .m_name = "Editor.ToolTip.AssetEditorHeader",
            .m_fill = QColor(0x27, 0x27, 0x27),                                     // background-color: :66
        };

        //! `Ui--AssetEditorHeader`: the header itself (its rows are on its class, so no other widget is asked)
        bool isAssetEditorHeader(const QWidget* widget)
        {
            return widget->inherits("Ui::AssetEditorHeader");
        }

        //! `Ui--AssetEditorHeader #Background` (:69): that name, below a header. The header makes it as its own child
        //! (AssetEditorHeader.cpp:31), so the parent decides it: a walk to the root would run on every card header's Background
        bool isAssetEditorHeaderBackground(const QWidget* widget)
        {
            return named(widget, "Background") && childOfClass(widget, "Ui::AssetEditorHeader");
        }

        //! Below a header, where its labels lie: in its Background frame (AssetEditorHeader.cpp:36, :43), the icon two steps below
        //! the header and the eliding name's own label three. Fixed steps, not a walk to the root, which every label of the Editor
        //! would run (the rows on QLabel are matched for each label at its polish).
        bool isInAssetEditorHeaderBackground(const QWidget* widget)
        {
            const QWidget* parent = widget->parentWidget();
            return parent
                && (childOfClass(parent, "Ui::AssetEditorHeader")
                    || (parent->parentWidget() && childOfClass(parent->parentWidget(), "Ui::AssetEditorHeader")));
        }

        //! `Ui--AssetEditorHeader #Name` (:79): that name, below a header: the name, and the label it measures with
        bool isAssetEditorHeaderName(const QWidget* widget)
        {
            return named(widget, "Name") && isInAssetEditorHeaderBackground(widget);
        }

        //! `Ui--AssetEditorHeader *` (:63), for a label: any label below a header. The name's row is registered first, so this one
        //! takes the labels that no rule of their own names (the icon).
        bool isAssetEditorHeaderLabel(const QWidget* widget)
        {
            return isInAssetEditorHeaderBackground(widget);
        }

        // ------------------------------------------------------------------------------
        // The rows of the Asset Editor's property editor: Editor.qss:93-107
        //
        // AssetEditorTab names its property editor AssetEditorWidgetPropertyEditor (AssetEditorTab.cpp): a ReflectedPropertyEditor, or,
        // with the cvar ed_enableDPEAssetEditor, a DocumentPropertyEditor in a FilteredDPE. Editor.qss is the main window's sheet
        // (CryEdit.cpp:1571); it reaches the rows below that editor by properties of the row, which Qt reads when it matches the
        // rules of a widget: at each polish of the row (QStyleSheetStyle::polish), and PropertyRowWidget::RefreshStyle polishes a
        // row again. An element resolves in that same polish (BoxElement::resolve), and its matcher reads the same properties.
        //
        // Editor.qss:93-96
        //   `PropertyRowWidget[isTopLevel="true"] { qproperty-appendDefaultLabelToName: 1 }`. Qt sets the property at each polish
        //   (QStyleSheetStyle::setProperties), as BoxElement::applyProperties does. Its setter
        //   (PropertyRowWidget::AppendDefaultLabelToName) puts the number of elements of a container into its name,
        //   "Name (N elements)", and hides the default label that would have shown it. The rule of Editor.qss:98-101 sets the
        //   property to 0 on every other row, which the setter ignores.
        //
        // Editor.qss:103-107
        //   `PropertyRowWidget[getLevel="1"][hasChildRows="true"]` and `DPERowWidget[getLevel="2"][hasChildRows="true"]`
        //   `{ border-top: 1px solid rgba(255, 255, 255, 0.2) }`: a line above the rows that group other rows. Qt reads the alpha as a
        //   fraction of 255 (parseColorValue, QCss::Declaration::colorValue): 0.2 * 255 = 51.
        //
        // A widget has one box element, and a top-level row at level 1 that has child rows is reached by both rules, so the elements
        // are the three combinations. They are registered in this order, and the first that matches wins. Each is the whole box of
        // its row: the fill of the universal rule (BaseStyleSheet.qss:17, transparent), no margin and no padding, and the border of
        // the row's frame (a row has none, as no code gives it a frame shape) unless the rule gives it a line of its own.
        // ------------------------------------------------------------------------------

        //! The row whose name has no row above it with a name (`PropertyRowWidget[isTopLevel="true"]`, Editor.qss:93-96) that is
        //! not also a group at level 1: the property alone. TOKENIZE: AssetEditorAppendDefaultLabel
        const BoxElement AssetEditorTopLevelRow{
            .m_name = "Editor.Box.AssetEditorTopLevelRow",
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill),       // BaseStyleSheet.qss:17
            },
            .m_properties = {
                { "appendDefaultLabelToName", true },                               // qproperty-appendDefaultLabelToName: 1: Editor.qss:95
            },
        };

        //! A row that groups other rows at level 1, below a row with a name (`[getLevel="1"][hasChildRows="true"]`, Editor.qss:103-107):
        //! the line above it alone. The rule's border is the row's own, so the row's contents sit under it, 1px lower.
        //! TOKENIZE: AssetEditorGroupRowLine
        const BoxElement AssetEditorGroupRow{
            .m_name = "Editor.Box.AssetEditorGroupRow",
            .m_nativeBorder = false,                                                // border-top is a border of its own (Editor.qss:106)
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill),       // BaseStyleSheet.qss:17
                // border-top: 1px solid rgba(...): Editor.qss:106. TOKENIZE: AssetEditorGroupRowLine
                .m_edges = { { 1, QColor(255, 255, 255, 51) }, {}, {}, {} },
            },
        };

        //! A top-level row that groups other rows at level 1, which has no named row above it: both rules reach it, the property of
        //! Editor.qss:95 and the line of Editor.qss:106. TOKENIZE: AssetEditorAppendDefaultLabel, AssetEditorGroupRowLine
        const BoxElement AssetEditorTopLevelGroupRow{
            .m_name = "Editor.Box.AssetEditorTopLevelGroupRow",
            .m_nativeBorder = false,                                                // border-top is a border of its own (Editor.qss:106)
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill),       // BaseStyleSheet.qss:17
                // border-top: 1px solid rgba(...): Editor.qss:106. TOKENIZE: AssetEditorGroupRowLine
                .m_edges = { { 1, QColor(255, 255, 255, 51) }, {}, {}, {} },
            },
            .m_properties = {
                { "appendDefaultLabelToName", true },                               // qproperty-appendDefaultLabelToName: 1: Editor.qss:95
            },
        };

        // The rows of the Asset Editor (Editor.qss:93-106). Each test reads the row's attributes first, through the getters that
        // their properties read when the row is polished (PropertyRowWidget.hxx:42-44, DocumentPropertyEditor.h:100-101), and walks to
        // the editor's name last (AssetEditorTab.cpp:227): with the walk first, every row of the Entity Inspector walked to the root.

        //! `#AssetEditorWidgetPropertyEditor`: the editor above the row, by name
        bool inAssetEditor(const QWidget* widget)
        {
            return within(widget, "AssetEditorWidgetPropertyEditor");
        }

        //! `#AssetEditorWidgetPropertyEditor AzToolsFramework--PropertyRowWidget[isTopLevel="true"]` (Editor.qss:93)
        bool isTopLevelAssetEditorRow(const QWidget* widget)
        {
            const auto* row = qobject_cast<const AzToolsFramework::PropertyRowWidget*>(widget);
            return row && row->IsTopLevel() && inAssetEditor(widget);
        }

        //! `#AssetEditorWidgetPropertyEditor AzToolsFramework--PropertyRowWidget[getLevel="1"][hasChildRows="true"]` (Editor.qss:103)
        bool isAssetEditorGroupRow(const QWidget* widget)
        {
            const auto* row = qobject_cast<const AzToolsFramework::PropertyRowWidget*>(widget);
            return row && row->GetLevel() == 1 && row->HasChildRows() && inAssetEditor(widget);
        }

        //! The same row where the rule of Editor.qss:93 reaches it too. The two rules are two selectors of two rules, both matched.
        bool isTopLevelAssetEditorGroupRow(const QWidget* widget)
        {
            const auto* row = qobject_cast<const AzToolsFramework::PropertyRowWidget*>(widget);
            return row && row->IsTopLevel() && row->GetLevel() == 1 && row->HasChildRows() && inAssetEditor(widget);
        }

        //! `#AssetEditorWidgetPropertyEditor AzToolsFramework--DPERowWidget[getLevel="2"][hasChildRows="true"]` (Editor.qss:104): a row
        //! of the document property editor that the Asset Editor shows with ed_enableDPEAssetEditor
        bool isAssetEditorDpeGroupRow(const QWidget* widget)
        {
            const auto* row = qobject_cast<const AzToolsFramework::DPERowWidget*>(widget);
            return row && row->GetLevel() == 2 && row->HasChildRows() && inAssetEditor(widget);
        }

        // ------------------------------------------------------------------------------
        // The Python Console: Editor.qss:187-201
        //
        // The rule is `ConsoleTextEdit, ConsoleTextEdit:hover, ConsoleTextEdit:focus, #SCRIPT_OUTPUT, #SCRIPT_OUTPUT:hover,
        // #SCRIPT_OUTPUT:focus`, one set of declarations for the six selectors.
        //
        // The three `ConsoleTextEdit` selectors reach no widget. A type selector must equal the whole class name, with '::' written
        // '--' (QStyleSheetStyleSelector::nodeNameEquals), and the class is AzToolsFramework::ConsoleTextEdit (ConsoleTextEdit.hxx,
        // the only class of that name), so the Editor's Console (ConsoleSCB.ui, textEdit), a ConsoleTextEdit as well, has the
        // application's rules for a plain text edit in both GUIs.
        //
        // `#SCRIPT_OUTPUT` reaches the output of the Python Console (ScriptTermDialog.ui), a view pane of the Editor. Qt's style
        // sheet style gave it, in the idle state and under the pointer and with the focus alike, the same box: the declarations
        // beat the application's rule for a text edit and its :hover and :focus (BaseStyleSheet.qss:86-115) in each state, as the
        // main window's sheet is deeper than the application's. So the console has no ring in any state, where the application's
        // rule gives a text edit a margin of 1px, a line of 1px and the radius of the entry field while it is hovered or has the
        // focus. The margin above it (Editor.qss:195) is the only space around the field.
        //
        // The fill (Editor.qss:196, #222222) is never what the console shows: ScriptTermDialog::RefreshStyle gives the console its
        // colour for the theme of the Editor, dark or white, with an inline sheet (`background: <colour>`,
        // ScrollBar::setBackgroundColor), the widget's own sheet and so deeper than the main window's. The flattened GUI keeps that
        // colour as the Base of the palette (ScrollBar::applyBackgroundColor), and a text edit whose element has no fill draws it
        // (BoxElement::draw), so the element has none.
        // ------------------------------------------------------------------------------

        //! `#SCRIPT_OUTPUT` (Editor.qss:187-201): the box of the Python Console's output, which is the same in each state. The margin
        //! above (`margin: 0; margin-top: 6px`, Editor.qss:194-195) puts the field, its text and its scroll bars 6px lower, and shows
        //! what is behind the console in that strip. A border of its own (width 0, Editor.qss:198-200) with no radius (Editor.qss:197)
        //! draws nothing: the absence of the ring is the absence of a state face. TOKENIZE: PythonConsoleOutputMargin
        const BoxElement PythonConsoleOutput{
            .m_name = "Editor.Box.PythonConsoleOutput",
            .m_margin = QMargins(0, 6, 0, 0),                                       // margin: 0; margin-top: 6px: Editor.qss:194-195
            .m_nativeBorder = false,                                                // border-width, -color and -style: Editor.qss:198-200
            .m_radius = 0,                                                          // border-radius: 0px: Editor.qss:197
            .m_idle = {
                // border: 0px #e9e9e9 solid: Editor.qss:198-200. TOKENIZE: PythonConsoleOutputBorder
                .m_edges = BoxElement::border(0, QColor(0xE9, 0xE9, 0xE9)),
            },
        };

        //! `#SCRIPT_OUTPUT` (Editor.qss:190-192): the widget of that name. The selector names no type, and the only widget of the name is
        //! the Python Console's output, a ConsoleTextEdit (a QPlainTextEdit).
        bool isPythonConsoleOutput(const QWidget* widget)
        {
            return named(widget, "SCRIPT_OUTPUT");
        }

        // ------------------------------------------------------------------------------
        // The bars of the UI Editor and of the Animation Editor: Editor.qss:145-148 and :160-171
        //
        // Editor.qss is the main window's sheet (CryEdit.cpp), deeper than the application's, so its declarations outrank those of
        // ToolBar.qss, ComboBox.qss and the universal rule for the properties they name, whatever the specificity of their rules.
        //
        // `EditorWindow QToolBar` (:145-148) reaches the toolbars below the window of the UI Editor, the class EditorWindow of the
        // LyShine gem: a type selector equals the whole class name (QStyleSheetStyleSelector::nodeNameEquals), so PhysX's
        // PhysX::Editor::EditorWindow is not reached. The UI Editor is a view pane, in the DockMainWindow of the main window, whose
        // toolbars have no border (`AzQtComponents--DockMainWindow QToolBar { border: 0 none }`, ToolBar.qss:36-39). The rule gives
        // them the 2px line of the toolbars of the main window (ToolBar.qss:31-34) again, along the bottom edge. The line takes no
        // room: a toolbar keeps its items inside the padding of ToolBar.qss:28, whose width is its frame width
        // (QStyleSheetStyle::pixelMetric, PM_ToolBarFrameWidth: the left border and the left padding), and the border is painted
        // over the padding below them. The toolbar draws its panel from its element, in place of the one of ToolBar.qss
        // (ControlElements::drawToolBarPanel).
        //
        // `#EMFX_Menu` (:160-165) is the menu widget of the Animation Editor's window (QMainWindow::setMenuWidget), a plain QWidget
        // that holds the menu bar and the application mode combo box: a second 2px line along its bottom edge, below the menu bar's
        // own (MenuBar.qss:17-19) and below the combo box. A plain widget's border takes no room either: its layout keeps its margins
        // (QStyleSheetStyle has no layout metric). `#EMFX_Menu QComboBox` (:167-171) gives the combo box 4px of margin at its right
        // and 2px below it, which leaves the line visible under it; its box is ComboBox.qss's otherwise, so the element repeats
        // that box (ComboBoxFlat::Combo) with the two margins changed in every state, as the deeper sheet's declarations outrank
        // the margins that ComboBox.qss:24, :35 and :46 take away in the states. The combo box has no error state: nothing sets
        // HasError on it, so the element has none of ComboBox.qss:44-51.
        // ------------------------------------------------------------------------------

        namespace Field = AzQtComponents::Foundations::Field;

        //! `EditorWindow QToolBar { border-bottom: 2px solid #111111 }` (Editor.qss:145-148) over
        //! `QToolBar { background-color; border: 0 none }` (ToolBar.qss:15-29): the panel of the toolbar, a fill and the line on its
        //! bottom edge. An element is the whole definition of its widget's box, so it repeats the fill.
        const BoxElement UiEditorToolBar{
            .m_name = "Editor.Box.UiEditorToolBar",
            .m_nativeBorder = false,                                                // border: 0 none (ToolBar.qss:22)
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Mid),                 // ToolBar.qss:20. TOKENIZE: UiEditorToolBarFill
                // top, right, bottom, left; border-bottom: Editor.qss:147. TOKENIZE: UiEditorToolBarLine
                .m_edges = { {}, {}, { 2, QColor(0x11, 0x11, 0x11) }, {} },
            },
        };

        //! `#EMFX_Menu { border-bottom-width: 2px; border-bottom-color: #111111; border-bottom-style: solid }` (Editor.qss:160-165): the
        //! line on the bottom edge of the menu widget, over the transparent background of '*' (BaseStyleSheet.qss:17)
        const BoxElement AnimationEditorMenu{
            .m_name = "Editor.Box.AnimationEditorMenu",
            .m_nativeBorder = false,                                                // a solid border-bottom is one of its own (:164)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // BaseStyleSheet.qss:17
                // top, right, bottom, left; border-bottom-width, -color: Editor.qss:162-163. TOKENIZE: AnimationEditorMenuLine
                .m_edges = { {}, {}, { 2, QColor(0x11, 0x11, 0x11) }, {} },
            },
        };

        //! The margin of the combo box in its idle state (left, top, right, bottom): ComboBox.qss:17 on the left and the top,
        //! Editor.qss:169-170 at the right and below. TOKENIZE: AnimationEditorModeComboMargin
        constexpr QMargins ModeComboMargin(1, 1, 4, 2);

        //! The margin in the states that take ComboBox.qss's away (:24, :35): 0 on the left and the top, and still Editor.qss:169-170's
        //! at the right and below. TOKENIZE: AnimationEditorModeComboStateMargin
        constexpr QMargins ModeComboStateMargin(0, 0, 4, 2);

        //! padding-left: ComboBox.qss:16. TOKENIZE: AnimationEditorModeComboPadding
        constexpr QMargins ModeComboPadding(4, 0, 0, 0);

        //! The line of the hovered combo box: ComboBox.qss:28. TOKENIZE: AnimationEditorModeComboHoverLine
        const QColor ModeComboHoverLine(0, 0, 0, 50);

        //! :focus, and an open popup (ComboBox.qss:31-42)
        const BoxElement::Face ModeComboFocusFace{
            .m_fill = QBrush(Field::FocusFill),
            .m_edges = BoxElement::border(Field::FocusLineWidth, Field::FocusLine),
            .m_text = Field::Text,
            .m_margin = ModeComboStateMargin,
            .m_radius = Field::FocusRadius,
        };

        //! `#EMFX_Menu QComboBox { margin-right: 4px; margin-bottom: 2px }` (Editor.qss:167-171) over the combo box of ComboBox.qss:13-57
        //! and BaseStyleSheet.qss:68-84, which ComboBoxFlat::Combo carries: the application mode combo box of the Animation Editor.
        const BoxElement AnimationEditorModeCombo{
            .m_name = "Editor.Box.AnimationEditorModeCombo",
            .m_margin = ModeComboMargin,
            .m_padding = ModeComboPadding,
            .m_nativeBorder = false,                                                // BaseStyleSheet.qss:79-80
            .m_radius = Field::Radius,                                              // BaseStyleSheet.qss:79
            .m_size = QSize(BoxElement::Unset, Field::Height),                      // height: ComboBox.qss:15
            .m_idle = { .m_fill = QBrush(Field::Fill), .m_text = Field::Text },     // ComboBox.qss:19, BaseStyleSheet.qss:77
            .m_stateFaces = {
                // :hover (ComboBox.qss:22-29): a line of its own and the margin of the states
                { BoxElement::Hover, {
                    .m_fill = QBrush(Field::Fill),
                    .m_edges = BoxElement::border(Field::HoverLineWidth, ModeComboHoverLine),       // ComboBox.qss:25, :28
                    .m_text = Field::Text,
                    .m_margin = ModeComboStateMargin,
                    .m_radius = Field::HoverRadius } },                             // ComboBox.qss:27
                { BoxElement::Focus, ModeComboFocusFace },
                { BoxElement::Checked, ModeComboFocusFace },                        // [HasPopupOpen=true]: ComboBox.qss:32-33
                { BoxElement::Disabled, { .m_fill = QBrush(Field::DisabledFill), .m_text = Field::DisabledText } },    // ComboBox.qss:53-57
            },
        };

        //! `EditorWindow QToolBar` (Editor.qss:145): a toolbar below a window of the class EditorWindow, at any depth
        bool isUiEditorToolBar(const QWidget* widget)
        {
            return withinClass(widget, "EditorWindow");
        }

        //! `#EMFX_Menu` (Editor.qss:160): the widget of that name. The selector names no type, and the only widget of the name is the menu
        //! widget of the Animation Editor's window.
        bool isAnimationEditorMenu(const QWidget* widget)
        {
            return named(widget, "EMFX_Menu");
        }

        //! `#EMFX_Menu QComboBox` (Editor.qss:167): a combo box below that widget, the mode combo box, its child and the only one
        //! below it (MainWindow.cpp:316-330). The parent's name: one step, where a walk ran for every combo box of the Editor.
        bool isAnimationEditorModeCombo(const QWidget* widget)
        {
            return childOf(widget, "EMFX_Menu");
        }

        // ------------------------------------------------------------------------------
        // The asset browser window's own sheets (AzAssetBrowserWindow.ui)
        //
        // The form gave five of its widgets a sheet. The legacy GUI sets them after setupUi (AzAssetBrowserWindow.cpp); the flattened
        // GUI sets none, and these elements draw what they drew. Each value cites the line of the form as it was before the flattening
        // ("the form before the flattening") and the declaration of the application's sheet beneath it. The last divider (frame, :357)
        // and the left pane (m_leftLayout, :457) have none: `QFrame { color : white;}` is the universal rule's colour
        // (BaseStyleSheet.qss:18), which the Global frame element draws, and `vertical-align` is not a property Qt's style sheet style
        // applies to a widget.
        // ------------------------------------------------------------------------------

        //! The dividers before and after the Create button (framePreCreate, framePostCreate): QFrames (Box, Plain, line width 1) of 1px
        //! by 22px whose sheet `QFrame { color : rgb(34, 34, 34);}` (the form before the flattening, :132, :206) only sets the colour of
        //! the line, which a Plain frame draws in the foreground. The fill behind the line is the one every frame of these shapes has
        //! (`.QFrame[frameShape]`, BaseStyleSheet.qss:41-48): an element is the whole definition of its frame, so it repeats it.
        //! The fill and the shapes are those of PanelFrame (FrameElement.cpp, Global.Frame.Panel), copied by value: when that element
        //! changes, this one changes with it. The line is the constant the legacy sheet's text is written from (EditorFlatValues.h).
        //! TOKENIZE: AssetBrowserCreateDividerLine
        const FrameElement AssetBrowserCreateDivider{
            .m_name = "Editor.Frame.AssetBrowserCreateDivider",
            .m_fill = AzQtComponents::Foundations::Mid,                             // background-color: BaseStyleSheet.qss:47
            .m_fillShapes = FrameElement::PanelShapes,                              // BaseStyleSheet.qss:41-45
            .m_foreground = EditorFlatValues::AssetBrowserCreateDividerLine,        // color: AzAssetBrowserWindow.ui:132, :206
                                                                                    // (the form before the flattening)
        };

        //! The thumbnails (AzQtComponents::AssetFolderThumbnailView, a QAbstractItemView of its own, not a tree, list or table) of the
        //! thumbnail view the window holds (m_thumbnailView). The form gave that view the sheet `AzToolsFramework--AssetBrowser--
        //! AssetBrowserThumbnailView, AzQtComponents--AssetFolderThumbnailView { background-color: #333333; }` (the form before the
        //! flattening, :567). The first selector reaches the view, a widget class Qt's sheet style gives no styled background, which
        //! the thumbnails cover whole; the second reaches the thumbnails, and fills them and the viewport inside them. The area keeps
        //! the native frame Qt's default sheet gave every QFrame, and the margin of 0 every scroll area has (QDockWidget.qss:33-36).
        //! The fill is the constant the legacy sheet's text is written from (EditorFlatValues.h).
        //! TOKENIZE: AssetBrowserThumbnailsFill
        const BoxElement AssetBrowserThumbnails{
            .m_name = "Editor.Box.AssetBrowserThumbnails",
            .m_hasBox = true,                                                       // margin: 0, QDockWidget.qss:35
            .m_idle = {
                .m_fill = QBrush(EditorFlatValues::AssetBrowserThumbnailsFill),     // background-color: AzAssetBrowserWindow.ui:567
                                                                                    // (the form before the flattening)
            },
        };

        //! `QFrame#framePreCreate` and `QFrame#framePostCreate` of the form: those two names, in the asset browser window
        bool isAssetBrowserCreateDivider(const QWidget* widget)
        {
            return (named(widget, "framePreCreate") || named(widget, "framePostCreate")) && withinClass(widget, "AzAssetBrowserWindow");
        }

        //! The thumbnail view the form gave the sheet: the view named m_thumbnailView, of its class, in the asset browser window
        bool isAssetBrowserThumbnailView(const QWidget* widget)
        {
            return named(widget, "m_thumbnailView") && widget->inherits("AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView")
                && withinClass(widget, "AzAssetBrowserWindow");
        }

        //! `AzQtComponents--AssetFolderThumbnailView` of that sheet: the thumbnails inside the view the sheet is on
        bool isAssetBrowserThumbnails(const QWidget* widget)
        {
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (isAssetBrowserThumbnailView(ancestor))
                {
                    return true;
                }
            }
            return false;
        }

        // ------------------------------------------------------------------------------
        // The asset importer's own sheets (ImporterRootDisplay.ui, AssetImporterWindow.qss)
        //
        // The form of the importer's root display gave its python builder frame a sheet. The legacy GUI sets it after setupUi
        // (ImporterRootDisplay.cpp); the flattened GUI sets none, and this element draws what it drew. Each value cites the line of the
        // form as it was before the flattening ("the form before the flattening").
        //
        // NOT here: AssetImporterWindow.qss, the sheet the importer's window sets on itself in the legacy GUI. It styles the check boxes
        // of the items of the importer's trees (a 12px outlined box, a blue fill, hover and disabled borders); its element is in
        // AssetImporterWindow.cpp, beside the window that sets the sheet.
        // ------------------------------------------------------------------------------

        //! The frame that names the python builder script assigned to a scene (the QFrame m_pythonBuilderLayout of the importer's root
        //! display), shown while a script is assigned. The form gave it `QFrame#m_pythonBuilderLayout {background-color: #333333;
        //! margin-bottom: 6px; padding-top: 2px; padding-bottom: 2px; border-top-left-radius: 2px; border-top-right-radius: 2px;}`
        //! (ImporterRootDisplay.ui:79, the form before the flattening), over the transparent background of '*' and the fill of every
        //! StyledPanel QFrame (BaseStyleSheet.qss:17, :41-48), which the inline fill outranks. A radius gives the border a style of its
        //! own: Qt's default sheet gives a QFrame `border: native`, and QRenderRule::fixupBorder turns a native border to none, 0 wide,
        //! where a rule has a radius (qstylesheetstyle.cpp). So the frame is the rounded fill alone, with no Raised panel line, which the
        //! base style would draw over a native border and which would move the contents in by its width.
        //!
        //! The sheet rounded the top two corners and gave the bottom two no radius, so they are square: the radii of each corner.
        //! The fill is the constant the legacy sheet's text is written from (EditorFlatValues.h).
        //! TOKENIZE: AssetImporterPythonBuilderFill
        const BoxElement AssetImporterPythonBuilder{
            .m_name = "Editor.Box.AssetImporterPythonBuilder",
            .m_margin = QMargins(0, 0, 0, 6),                                       // margin-bottom: ImporterRootDisplay.ui:79
            .m_padding = QMargins(0, 2, 0, 2),                                      // padding-top, padding-bottom: :79
            .m_nativeBorder = false,                                                // a radius is a border of its own (fixupBorder), :79
            // border-top-left-radius, border-top-right-radius: :79 (the bottom corners have none)
            .m_radii = { .m_topLeft = 2, .m_topRight = 2 },
            .m_idle = {
                .m_fill = QBrush(EditorFlatValues::AssetImporterPythonBuilderFill), // background-color: ImporterRootDisplay.ui:79
            },
        };

        //! `QFrame#m_pythonBuilderLayout` of the form: that name, in the importer's root display (the only form that builds it)
        bool isAssetImporterPythonBuilder(const QWidget* widget)
        {
            return named(widget, "m_pythonBuilderLayout") && withinClass(widget, "ImporterRootDisplayWidget");
        }

        // ------------------------------------------------------------------------------
        // The welcome dialog's own sheet (WelcomeScreenDialog.ui)
        //
        // The form gave the divider at the foot of the dialog's body (line_4) a sheet. The legacy GUI sets it after setupUi
        // (WelcomeScreenDialog.cpp); the flattened GUI sets none, and this element draws what it drew. The dialog is made after the main
        // window's sheet is set (CryEdit.cpp), so it is registered with the elements of Editor.qss.
        // ------------------------------------------------------------------------------

        //! The divider at the foot of the welcome dialog (the Line line_4: a QFrame, HLine, Plain, line width 0, 1px high). Its sheet
        //! `color: "black"` (WelcomeScreenDialog.ui:613, the form before the flattening) only sets the colour of the line, which a Plain
        //! frame draws in the foreground; Qt reads the quoted name as a colour (QCss::ValueExtractor::extractPalette). The fill behind
        //! the line is the one every frame of these shapes has (`.QFrame[frameShape]`, BaseStyleSheet.qss:41-48): an element is the whole
        //! definition of its frame, so it repeats it, as the dividers of the asset browser do (AssetBrowserCreateDivider).
        //! TOKENIZE: WelcomeDividerLine
        const FrameElement WelcomeDivider{
            .m_name = "Editor.Frame.WelcomeDivider",
            .m_fill = AzQtComponents::Foundations::Mid,                             // background-color: BaseStyleSheet.qss:47
            .m_fillShapes = FrameElement::PanelShapes,                              // BaseStyleSheet.qss:41-45
            // color: "black": WelcomeScreenDialog.ui:613. TOKENIZE: WelcomeDividerLine
            .m_foreground = QColor(Qt::black),
        };

        //! `Line#line_4` of the form: that name, in the welcome dialog
        bool isWelcomeDivider(const QWidget* widget)
        {
            return named(widget, "line_4") && withinClass(widget, "WelcomeScreenDialog");
        }
    } // namespace EditorFlatSheetInternal

    // ----------------------------------------------------------------------------------
    // Flattened GUI: the sheets two of the Editor's dialogs set on themselves
    //
    // StyleManager::setStyleSheet(widget, "style:X.qss") sets no sheet in the flattened GUI; it runs the flat sheet registered for X,
    // and these two sheets had none, so their rules were dropped. CryEdit.cpp registers the two functions below for them. A value
    // cites the line of the sheet it folds, and the rule of the application's sheet beneath it where the element repeats one: an
    // element is the whole definition of the box of its widget, so it carries what the dialog's sheet left alone as well.
    // ----------------------------------------------------------------------------------

    //! Everything private to these two sheets: a named namespace, for the reason EditorFlatSheetInternal gives
    namespace EditorFlatSheetDialogsInternal
    {
        // ------------------------------------------------------------------------------
        // The Editor Preferences dialog: EditorPreferencesDialog.qss, which the dialog sets on itself (EditorPreferencesDialog.cpp:99)
        //
        // The sheet reaches the dialog and what is in it: the list of pages on the left, the property editor on the right with the
        // rows of the page, and the name label of each row. The dialog is modal.
        // The rule `PropertyRowWidget[HasParent="false"] QLabel#Name` (:45-50) reaches nothing: no widget has a property HasParent
        // (PropertyRowWidget has hasChildRows, isTopLevel, getLevel, canBeReordered and appendDefaultLabelToName), so its bold, its
        // padding `0 0 0 5` and its white are in no element. The name of a group row is bold by :59-62, which does reach it.
        // Editor.qss, set on the main window the dialog is a child of, also reaches the dialog: its rules for the name of a row that can
        // be reordered or is overridden by a prefab (Editor.qss:43-53) match no row of a preference page (as far as read: those need
        // the rows of a container or of a prefab instance), and no element carries them; that is for the conversion of Editor.qss.
        // ------------------------------------------------------------------------------

        //! `QWidget#EditorPreferencesDialog` (EditorPreferencesDialog.qss:10-16): the dialog (a QDialog, EditorPreferencesDialog.ui:4),
        //! filled with #444444 as every QDialog is (`QMainWindow, QDialog, QDockWidget`, BaseStyleSheet.qss:60-66). The margin of 0 and
        //! the padding `-4px -5px -4px -5px` make it a box, but the box of a widget that is not a frame only places its fill (the
        //! sheet style has no layout case for it); `border-color` alone is a border of its own with no width.
        //! TOKENIZE: PreferencesDialogFill
        const BoxElement PreferencesDialog{
            .m_name = "Editor.Box.PreferencesDialog",
            .m_padding = QMargins(-5, -4, -5, -4),                  // padding: -4px -5px -4px -5px (top right bottom left): :13
            .m_hasBox = true,                                       // margin: 0px, a margin even at 0 (:12)
            .m_nativeBorder = false,                                // border-color makes the border the rule's own (:15)
            .m_idle = {
                // background-color: EditorPreferencesDialog.qss:14. TOKENIZE: PreferencesDialogFill
                .m_fill = QBrush(QColor(0x44, 0x44, 0x44)),
            },
        };

        //! `QWidget#EditorPreferencesDialog QTreeWidget#pageTree` (EditorPreferencesDialog.qss:18-24): the list of pages. A line of its
        //! own, 1px of #111111 (:21) instead of TableView.qss:28's none, and #333333 (:22) where the application's rule left a view in a
        //! widget transparent (BaseStyleSheet.qss:32-38), with the margin of 0 every scroll area has (:23, QDockWidget.qss:33-36). Its
        //! items are PreferencesPageTreeItems, its text PreferencesPageTreeText.
        //! TOKENIZE: PreferencesPageTreeFill, PreferencesPageTreeLine
        const BoxElement PreferencesPageTree{
            .m_name = "Editor.Box.PreferencesPageTree",
            .m_hasBox = true,                                       // margin: 0px (:23)
            .m_nativeBorder = false,                                // border: 1px solid (:21)
            .m_idle = {
                // background-color: EditorPreferencesDialog.qss:22. TOKENIZE: PreferencesPageTreeFill
                .m_fill = QBrush(QColor(0x33, 0x33, 0x33)),
                // border: 1px solid #111111: EditorPreferencesDialog.qss:21. TOKENIZE: PreferencesPageTreeLine
                .m_edges = BoxElement::border(1, QColor(0x11, 0x11, 0x11)),
            },
        };

        //! `QTreeWidget#pageTree { font-size: 11px }` (EditorPreferencesDialog.qss:20), over the family and weight of the universal rule
        //! (BaseStyleSheet.qss:19-20) that the element carries as every text element does. The custom delegate of the tree
        //! (EditorPreferencesTreeWidgetItemDelegate) measures and draws its text with the font of the view, so the 11px reaches both.
        //! TOKENIZE: PreferencesPageTreeSize
        const TextElement PreferencesPageTreeText{
            .m_name = "Editor.Text.PreferencesPageTree",
            .m_pixelSize = 11,                                      // font-size: :20
        };

        //! `QTreeWidget#pageTree::item { margin-top: 3px; margin-bottom: 2px }` (EditorPreferencesDialog.qss:26-30), over
        //! `QTreeView::item` (TableView.qss:31-40: a height of 24px, 4px of padding at the left, no border), which the Global tree element
        //! keeps. The delegate's own colours for a selected and a hovered page are upstream paint
        //! (EditorPreferencesTreeWidgetItemDelegate.cpp:23-24: I.5's).
        //! TOKENIZE: PreferencesPageTreeItemMargin
        const ItemViewElement PreferencesPageTreeItems{
            .m_name = "Editor.ItemView.PreferencesPageTree",
            .m_itemMargin = QMargins(0, 3, 0, 2),                   // margin-top, margin-bottom: :28-29
        };

        //! `QWidget#EditorPreferencesDialog AzToolsFramework--ReflectedPropertyEditor#propertyEditor`
        //! (EditorPreferencesDialog.qss:32-37): the editor of the page, a QFrame (StyledPanel, Raised: EditorPreferencesDialog.ui:107-112)
        //! whose native frame the rule's own border replaces: 1px of #111111 (:35) around #222222 (:34), 5px from the pages at the left and
        //! 2px at the right (:36, margin: top right bottom left).
        //! TOKENIZE: PreferencesPropertyEditorFill, PreferencesPropertyEditorLine
        const BoxElement PreferencesPropertyEditor{
            .m_name = "Editor.Box.PreferencesPropertyEditor",
            .m_margin = QMargins(5, 0, 2, 0),                       // margin: 0px 2px 0px 5px: :36
            .m_nativeBorder = false,                                // border: 1px solid (:35)
            .m_idle = {
                // background-color: EditorPreferencesDialog.qss:34. TOKENIZE: PreferencesPropertyEditorFill
                .m_fill = QBrush(QColor(0x22, 0x22, 0x22)),
                // border: 1px solid #111111: EditorPreferencesDialog.qss:35. TOKENIZE: PreferencesPropertyEditorLine
                .m_edges = BoxElement::border(1, QColor(0x11, 0x11, 0x11)),
            },
        };

        //! `#EditorPreferencesDialog AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"]`
        //! (EditorPreferencesDialog.qss:52-57): the row of a group, which has a line under it, 1px of white at a fifth of its alpha
        //! (`rgba(255, 255, 255, 0.2)`: Qt reads the fraction as 0.2 * 255 = 51; :54), room above it and below it inside the line
        //! (padding: 4px 0 6px: :55) and 4px of space after (:56), over the transparent fill of the universal rule (BaseStyleSheet.qss:17).
        //! The row paints no frame of its own (its paintEvent draws only the reorder line), so the rule is drawn once, with the styled
        //! background. TOKENIZE: PreferencesGroupRowLine
        const BoxElement PreferencesGroupRow{
            .m_name = "Editor.Box.PreferencesGroupRow",
            .m_margin = QMargins(0, 0, 0, 4),                       // margin-bottom: :56
            .m_padding = QMargins(0, 4, 0, 6),                      // padding: 4px 0 6px (top, left and right, bottom): :55
            .m_nativeBorder = false,                                // border-bottom is a border of its own (:54)
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill),       // BaseStyleSheet.qss:17
                // border-bottom: 1px solid rgba(...): EditorPreferencesDialog.qss:54. TOKENIZE: PreferencesGroupRowLine
                .m_edges = { {}, {}, { 1, QColor(255, 255, 255, 51) }, {} },
            },
        };

        //! `AzToolsFramework--PropertyRowWidget QLabel#Name` (:39-43) over the rule of every row (ReflectedPropertyEditor.qss:9-15, which
        //! PropertyEditorElements.cpp carries as Global.Box.PropertyRowName, copied here by value: when that changes, this changes with
        //! it): 4px at the left to match a card header (:13-14), 16px high whatever the text needs (:11-12), no margin above or below
        //! where Text.qss:93-94 gave every label 1px, and the dialog's 5px of padding at the left (:42). The row's name is an
        //! ElidingLabel, which measures its hint with a hidden QLabel of the same name: the rule reached both. TOKENIZE:
        //! PreferencesRowNamePadding
        const BoxElement PreferencesRowName{
            .m_name = "Editor.Box.PreferencesRowName",
            .m_margin = QMargins(4, 0, 0, 0),                       // margin: 0px, margin-left: 4px: ReflectedPropertyEditor.qss:13-14
            .m_padding = QMargins(5, 0, 0, 0),                      // padding: 0 0 0 5px: EditorPreferencesDialog.qss:42
            .m_minSize = QSize(-1, 16),                             // min-height: ReflectedPropertyEditor.qss:11
            .m_maxSize = QSize(-1, 16),                             // max-height: ReflectedPropertyEditor.qss:12
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill),       // BaseStyleSheet.qss:17
            },
        };

        //! The text of that label: #eeeeee (:41) over the label rule's family and size (Text.qss:91-92). TOKENIZE: PreferencesRowNameColor
        const TextElement PreferencesRowNameText{
            .m_name = "Editor.Text.PreferencesRowName",
            .m_family = AzQtComponents::Foundations::Label::Family,             // Text.qss:91
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,            // Text.qss:92
            // color: EditorPreferencesDialog.qss:41. TOKENIZE: PreferencesRowNameColor
            .m_color = QColor(0xEE, 0xEE, 0xEE),
        };

        //! The same for the name of a group row, bold by `[isTopLevel="true"][hasChildRows="true"] QLabel#Name`
        //! (EditorPreferencesDialog.qss:59-62).
        //! TOKENIZE: PreferencesGroupNameWeight
        const TextElement PreferencesGroupNameText{
            .m_name = "Editor.Text.PreferencesGroupName",
            .m_family = AzQtComponents::Foundations::Label::Family,             // Text.qss:91
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,            // Text.qss:92
            .m_weight = 700,                                                    // font-weight: bold (QFont::Bold): :61
            // color: EditorPreferencesDialog.qss:41. TOKENIZE: PreferencesRowNameColor
            .m_color = QColor(0xEE, 0xEE, 0xEE),
        };

        // ------------------------------------------------------------------------------
        // The Editor Preferences dialog: the matchers
        //
        // A selector of the sheet that does not name the dialog is the sheet's own scope: the sheet is the dialog's, so it reaches the
        // dialog and what is below it, and the names pageTree, propertyEditor and Name are only these below the dialog.
        // ------------------------------------------------------------------------------

        //! `#EditorPreferencesDialog`: the dialog, by its name (the class has no Q_OBJECT, so its class name is QDialog's)
        bool isPreferencesDialog(const QWidget* widget)
        {
            return named(widget, "EditorPreferencesDialog");
        }

        //! Below the dialog
        bool inPreferencesDialog(const QWidget* widget)
        {
            return within(widget, "EditorPreferencesDialog");
        }

        //! `QWidget#EditorPreferencesDialog QTreeWidget#pageTree` (:18)
        bool isPreferencesPageTree(const QWidget* widget)
        {
            return named(widget, "pageTree") && inPreferencesDialog(widget);
        }

        //! `QWidget#EditorPreferencesDialog AzToolsFramework--ReflectedPropertyEditor#propertyEditor` (:32)
        bool isPreferencesPropertyEditor(const QWidget* widget)
        {
            return named(widget, "propertyEditor") && widget->inherits("AzToolsFramework::ReflectedPropertyEditor")
                && inPreferencesDialog(widget);
        }

        //! `AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"]` (:52, :59): a row of a group. Both properties
        //! are read as the selector reads them, when the widget is polished (PropertyRowWidget::RefreshStyle polishes a row again
        //! once the rows of the page are built).
        bool isGroupRow(const QWidget* widget)
        {
            return widget->inherits("AzToolsFramework::PropertyRowWidget") && hasProperty(widget, "isTopLevel", "true")
                && hasProperty(widget, "hasChildRows", "true");
        }

        //! `#EditorPreferencesDialog AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"]` (:52)
        bool isPreferencesGroupRow(const QWidget* widget)
        {
            return isGroupRow(widget) && inPreferencesDialog(widget);
        }

        //! `AzToolsFramework--PropertyRowWidget QLabel#Name` (:39): the name label of a row, and the label an ElidingLabel measures with
        bool isPreferencesRowName(const QWidget* widget)
        {
            return named(widget, "Name") && withinClass(widget, "AzToolsFramework::PropertyRowWidget") && inPreferencesDialog(widget);
        }

        //! `#EditorPreferencesDialog AzToolsFramework--PropertyRowWidget[isTopLevel="true"][hasChildRows="true"] QLabel#Name` (:59)
        bool isPreferencesGroupRowName(const QWidget* widget)
        {
            if (!isPreferencesRowName(widget))
            {
                return false;
            }
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (isGroupRow(ancestor))
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace EditorFlatSheetDialogsInternal

    // ----------------------------------------------------------------------------------
    // Flattened GUI: the sheets of the startup logo dialog
    //
    // CStartupLogoDialog (the splash screen and the About box, StartupLogoDialog.cpp) sets a sheet of its own on itself in each mode, and
    // EditorFlatValues::StartupLogoLoadingSheet and StartupLogoAboutSheet hold their text. Both reach the labels of the dialog, which
    // the application's sheet gave the label rule (Text.qss:89-95) and the universal rule (BaseStyleSheet.qss:15-21) beneath them; the
    // elements below carry only what the dialog's rules changed. The splash's first rule, `QLabel { background: transparent; color:
    // 'white' }` (StartupLogoDialog.cpp:54 before the flattening), changes nothing: the universal rule gives every widget a transparent
    // background and white text (BaseStyleSheet.qss:17-18). The lines of StartupLogoDialog.cpp cited below are those of the file before
    // the flattening, when the sheets were the strings of its constructor. RegisterStartupLogoElements is called by the dialog, as the
    // splash is made before the main window sets Editor.qss. The elements resolve when a label is polished and the form polishes its
    // labels while it is built, so the dialog tags itself in the About mode, which these matchers read, once it has named them, and
    // polishes the labels below it again (StartupLogoDialog.cpp, Style::addClass).
    // ----------------------------------------------------------------------------------

    //! Everything private to this sheet: a named namespace, for the reason EditorFlatSheetInternal gives
    namespace EditorFlatSheetStartupLogoInternal
    {
        //! `QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px }` (StartupLogoDialog.cpp:55 in the splash, :65 in the About box,
        //! before the flattening): the copyright notice, a rich text label. The size is 9px over the label rule's 12px (Text.qss:92) and
        //! the family stays the label rule's (Text.qss:91); the weight is the universal rule's (BaseStyleSheet.qss:20), which the element
        //! carries as every text element does. The sheet gave the colour in every state, the disabled one too, which is what the element
        //! does without a colour of its own for it. TOKENIZE: StartupCopyrightNotice
        const TextElement StartupCopyrightNotice{
            .m_name = "Editor.Text.StartupCopyrightNotice",
            .m_family = AzQtComponents::Foundations::Label::Family,                 // Text.qss:91
            // font-size: StartupLogoDialog.cpp:55, :65 before the flattening. TOKENIZE: StartupCopyrightNoticeSize
            .m_pixelSize = EditorFlatValues::StartupCopyrightNoticeSize,
            // color: StartupLogoDialog.cpp:55, :65 before the flattening. TOKENIZE: StartupCopyrightNoticeText
            .m_color = EditorFlatValues::StartupCopyrightNoticeText,
        };

        //! `QLabel#link { text-decoration: underline; color: #94D2FF }` (StartupLogoDialog.cpp:66 before the flattening): the Terms of Use
        //! link of the About box.
        //! The label rule's family and size stay (Text.qss:91-92) as in the notice's element. The rule's underline is the label font's
        //! (QCss extractFont sets QFont::setUnderline). The label is a rich text label, and the sheet's colour is the one of its
        //! text roles; the colour of the anchor in it is the palette's link role, which no rule sets. TOKENIZE: StartupLink
        const TextElement StartupLink{
            .m_name = "Editor.Text.StartupLink",
            .m_family = AzQtComponents::Foundations::Label::Family,                 // Text.qss:91
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,                // Text.qss:92
            // text-decoration: underline, StartupLogoDialog.cpp:66 before the flattening. TOKENIZE: StartupLinkUnderline
            .m_underline = 1,
            // color: StartupLogoDialog.cpp:66 before the flattening. TOKENIZE: StartupLinkText
            .m_color = EditorFlatValues::StartupLinkText,
        };

        // ------------------------------------------------------------------------------
        // Matchers: the selectors that gave each rule its widgets
        // ------------------------------------------------------------------------------

        //! `QLabel#copyrightNotice`, in the sheet the dialog set on itself: below the dialog, in either mode
        bool isStartupCopyrightNotice(const QWidget* widget)
        {
            return named(widget, "copyrightNotice") && withinClass(widget, "CStartupLogoDialog");
        }

        //! `QLabel#link`, in the sheet of the About box only (the splash has no rule for it): below a dialog tagged as the About box
        bool isStartupLink(const QWidget* widget)
        {
            return named(widget, "link") && withinClass(widget, "CStartupLogoDialog") && withinTagged(widget, StartupLogoAboutTag);
        }
    } // namespace EditorFlatSheetStartupLogoInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void RegisterElements()
    {
        // The selectors name no type, so the lookup is by the widget's name alone
        const QMetaObject* anyWidget = &QWidget::staticMetaObject;

        BoxElement::registerElement(&EditorFlatSheetInternal::AssetEditorStatusBar, anyWidget, &EditorFlatSheetInternal::isAssetEditorStatusBar);
        BoxElement::registerElement(&EditorFlatSheetInternal::AssetBrowserSearchButtons, anyWidget, &EditorFlatSheetInternal::isAssetBrowserSearchButtons);
        BoxElement::registerElement(&EditorFlatSheetInternal::AssetBrowserSeparator, anyWidget, &EditorFlatSheetInternal::isAssetBrowserSeparator);
        BoxElement::registerElement(&EditorFlatSheetInternal::ComponentPaletteTree, &QTreeView::staticMetaObject, &EditorFlatSheetInternal::isComponentPaletteTree);

        // The roles of a view are an item view element's
        ItemViewElement::registerElement(&EditorFlatSheetInternal::NodePaletteTree, &QTreeView::staticMetaObject, &EditorFlatSheetInternal::isNodePaletteTree);
        ItemViewElement::registerElement(&EditorFlatSheetInternal::NewLevelTemplateList, &QListWidget::staticMetaObject, &EditorFlatSheetInternal::isNewLevelTemplateList);

        // The asset browser window's own sheets (AzAssetBrowserWindow.ui): the dividers are QFrames, the thumbnails a view of their own.
        // A frame row on QFrame is for a QFrame itself (FrameElement.cpp, isCandidate): no class derived from QFrame is asked.
        FrameElement::registerElement(
            &EditorFlatSheetInternal::AssetBrowserCreateDivider, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isAssetBrowserCreateDivider);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetBrowserThumbnails, &AzQtComponents::AssetFolderThumbnailView::staticMetaObject,
            &EditorFlatSheetInternal::isAssetBrowserThumbnails);

        // The asset importer's own sheets (ImporterRootDisplay.ui): the python builder frame is a QFrame
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetImporterPythonBuilder, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isAssetImporterPythonBuilder);

        // The Asset Editor's header (Editor.qss:56-91): the header's rows on its own class, the bar a QFrame, the name and the icon
        // QLabels. The name's box first: the universal rule's takes the other labels below the header.
        const QMetaObject* header = &Ui::AssetEditorHeader::staticMetaObject;
        BoxElement::registerElement(&EditorFlatSheetInternal::AssetEditorHeader, header, &EditorFlatSheetInternal::isAssetEditorHeader);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorHeaderBackground, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isAssetEditorHeaderBackground);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorHeaderName, &QLabel::staticMetaObject, &EditorFlatSheetInternal::isAssetEditorHeaderName);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorHeaderLabel, &QLabel::staticMetaObject,
            &EditorFlatSheetInternal::isAssetEditorHeaderLabel);
        TextElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorHeaderNameText, &QLabel::staticMetaObject,
            &EditorFlatSheetInternal::isAssetEditorHeaderName);
        ToolTipElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorHeaderToolTip, header, &EditorFlatSheetInternal::isAssetEditorHeader);

        // The rows of the Asset Editor's property editor (Editor.qss:93-107): the rows of both editors are QFrames. A row reached by
        // both rules has the element that carries both, so that element is registered first, then the top-level rows, then the groups
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorTopLevelGroupRow, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isTopLevelAssetEditorGroupRow);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorTopLevelRow, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isTopLevelAssetEditorRow);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorGroupRow, &QFrame::staticMetaObject, &EditorFlatSheetInternal::isAssetEditorGroupRow);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AssetEditorGroupRow, &QFrame::staticMetaObject,
            &EditorFlatSheetInternal::isAssetEditorDpeGroupRow);

        // The Python Console (Editor.qss:187-201): the output is a ConsoleTextEdit, a QPlainTextEdit
        BoxElement::registerElement(
            &EditorFlatSheetInternal::PythonConsoleOutput, &QPlainTextEdit::staticMetaObject,
            &EditorFlatSheetInternal::isPythonConsoleOutput);

        // The bars of the UI Editor and the Animation Editor (Editor.qss:145-148, :160-171): the panel of a toolbar, drawn from the
        // toolbar's own element (ControlElements::drawToolBarPanel), the menu widget, a plain widget, and the combo box in it
        BoxElement::registerElement(
            &EditorFlatSheetInternal::UiEditorToolBar, &QToolBar::staticMetaObject, &EditorFlatSheetInternal::isUiEditorToolBar);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AnimationEditorMenu, anyWidget, &EditorFlatSheetInternal::isAnimationEditorMenu);
        BoxElement::registerElement(
            &EditorFlatSheetInternal::AnimationEditorModeCombo, &QComboBox::staticMetaObject,
            &EditorFlatSheetInternal::isAnimationEditorModeCombo);

        // The welcome dialog's own sheet (WelcomeScreenDialog.ui): the divider is a QFrame
        FrameElement::registerElement(
            &EditorFlatSheetInternal::WelcomeDivider, &QFrame::staticMetaObject, &EditorFlatSheetInternal::isWelcomeDivider);
    }

    void RegisterPreferencesDialogElements()
    {
        namespace Flat = EditorFlatSheetDialogsInternal;

        // The dialog is a QDialog, the list of pages a QTreeWidget; the property editor and the rows of a group are QFrames
        BoxElement::registerElement(&Flat::PreferencesDialog, &QDialog::staticMetaObject, &Flat::isPreferencesDialog);
        BoxElement::registerElement(&Flat::PreferencesPageTree, &QTreeWidget::staticMetaObject, &Flat::isPreferencesPageTree);
        TextElement::registerElement(&Flat::PreferencesPageTreeText, &QTreeWidget::staticMetaObject, &Flat::isPreferencesPageTree);
        ItemViewElement::registerElement(&Flat::PreferencesPageTreeItems, &QTreeWidget::staticMetaObject, &Flat::isPreferencesPageTree);
        BoxElement::registerElement(&Flat::PreferencesPropertyEditor, &QFrame::staticMetaObject, &Flat::isPreferencesPropertyEditor);
        BoxElement::registerElement(&Flat::PreferencesGroupRow, &QFrame::staticMetaObject, &Flat::isPreferencesGroupRow);

        // The name labels: one box, and a text element for the group's (the more specific rule first) and one for the others
        BoxElement::registerElement(&Flat::PreferencesRowName, &QLabel::staticMetaObject, &Flat::isPreferencesRowName);
        TextElement::registerElement(&Flat::PreferencesGroupNameText, &QLabel::staticMetaObject, &Flat::isPreferencesGroupRowName);
        TextElement::registerElement(&Flat::PreferencesRowNameText, &QLabel::staticMetaObject, &Flat::isPreferencesRowName);
    }

    void RegisterStartupLogoElements()
    {
        namespace Flat = EditorFlatSheetStartupLogoInternal;

        // A registration lives for the whole run, and the registries do not look for repeats: the dialog calls this at every
        // construction, in the splash screen and in each About box
        static bool registered = false;
        if (registered || !AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        // The labels of the dialog, in either mode: the notice, and the link of the About box
        TextElement::registerElement(&Flat::StartupCopyrightNotice, &QLabel::staticMetaObject, &Flat::isStartupCopyrightNotice);
        TextElement::registerElement(&Flat::StartupLink, &QLabel::staticMetaObject, &Flat::isStartupLink);
    }
} // namespace EditorFlatSheet
