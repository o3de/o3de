/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorFlatSheet.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>

#include <QBrush>
#include <QColor>
#include <QListWidget>
#include <QTreeView>
#include <QWidget>

namespace EditorFlatSheet
{
    using AzQtComponents::BoxElement;
    using AzQtComponents::ItemViewElement;
    using AzQtComponents::TextElement;

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

        //! The row of the asset browser that holds the selection chip (AzAssetBrowserWindow.ui:386), a QFrame without a frame:
        //! a dark fill and no border (`AzAssetBrowserWindow #searchButtonContainer`, Editor.qss:20-24).
        const BoxElement AssetBrowserSearchButtons{
            .m_name = "Editor.Box.AssetBrowserSearchButtons",
            .m_nativeBorder = false,                                                // border: 0 none (Editor.qss:23)
            .m_idle = {
                .m_fill = QBrush(QColor(0x14, 0x14, 0x14)),                         // Editor.qss:22. TOKENIZE: AssetBrowserSearchButtonsFill
            },
        };

        //! The 1px line under that row (AzAssetBrowserWindow.ui:420), a plain QWidget: a dark fill
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
                .m_fill = QBrush(QColor(0x22, 0x22, 0x22)),                         // Editor.qss:40
            },
        };

        //! The node palettes of the Script Canvas and the Landscape Canvas (`GraphCanvas--NodePaletteTreeView { selection-background-color:
        //! rgba(255, 255, 255, 41) }`, Editor.qss:109-113: "Set the Script Canvas search highlight color"): the role of the tree's selection,
        //! which the highlight of a search match is drawn with. The fill of a selected item stays the Global one, rgb(101,101,101), whose
        //! `::item:selected` rule outranks the role (TableView.qss:42-50). The role is Native's (the n2 palette dumps of both node palettes hold
        //! Highlight #29ffffff, the sheet's rgba(255,255,255,41)); UNVERIFIED by capture: no capture shows a search highlight drawn with it.
        //! TOKENIZE: NodePaletteSearchHighlight
        const ItemViewElement NodePaletteTree{
            .m_name = "Editor.ItemView.NodePaletteTree",
            .m_selection = QColor(255, 255, 255, 41),                               // selection-background-color: Editor.qss:112
        };

        //! The template list of the New Level dialog: the dialog sets `QListWidget::item { height: 148px; padding-left: 0px; padding-right: 0px;
        //! background-color: transparent }` on itself (NewLevelDialog.cpp:79), and the sheet style still runs that rule in the flattened GUI, over
        //! the flattened base style. A sheet set on a widget outranks the application's whatever the specificity of their rules (the depth of the
        //! sheet is weighed before the specificity: qcssparser.cpp:2220-2222, qstylesheetstyle.cpp:1658, 1689), so the items have no padding and
        //! no fill in any state: the transparent background beats the application's `::item:selected` and `::item:hover` rules (TableView.qss:42-60).
        //! Without the element the sheet style asks the flattened base style for the size and the contents of an item, which gives it the 4px
        //! of padding of the Global list (TableView.qss:31-40) that the legacy base style did not have under this rule: 4px more of width,
        //! and the label and icon that much further in. UNVERIFIED by capture: no capture holds the dialog. TOKENIZE: NewLevelTemplateItem
        const ItemViewElement NewLevelTemplateList{
            .m_name = "Editor.ItemView.NewLevelTemplateList",
            .m_itemHeight = 148,                                                    // height: NewLevelDialog.cpp:79
            .m_itemPadding = QMargins(0, 0, 0, 0),                                  // padding-left, padding-right: NewLevelDialog.cpp:79
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
            return TextElement::named(widget, "AssetEditorStatusBar");
        }

        //! `AzAssetBrowserWindow #searchButtonContainer` (Editor.qss:20): that name, below the asset browser window
        bool isAssetBrowserSearchButtons(const QWidget* widget)
        {
            return TextElement::named(widget, "searchButtonContainer") && TextElement::withinClass(widget, "AzAssetBrowserWindow");
        }

        //! `AzAssetBrowserWindow #m_separator` (Editor.qss:26): that name, below the asset browser window
        bool isAssetBrowserSeparator(const QWidget* widget)
        {
            return TextElement::named(widget, "m_separator") && TextElement::withinClass(widget, "AzAssetBrowserWindow");
        }

        //! `AzToolsFramework--ComponentPaletteWidget > QTreeView` (Editor.qss:38): a tree whose parent is the palette widget
        bool isComponentPaletteTree(const QWidget* widget)
        {
            return widget->parentWidget() && widget->parentWidget()->inherits("AzToolsFramework::ComponentPaletteWidget");
        }

        //! `GraphCanvas--NodePaletteTreeView` (Editor.qss:110): the tree itself, or one of a class derived from it
        bool isNodePaletteTree(const QWidget* widget)
        {
            return widget->inherits("GraphCanvas::NodePaletteTreeView");
        }

        //! The `QListWidget::item` rule that the New Level dialog sets on itself (NewLevelDialog.cpp:79): any list widget below that dialog
        bool isNewLevelTemplateList(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "CNewLevelDialog");
        }
    } // namespace EditorFlatSheetInternal

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
    }
} // namespace EditorFlatSheet
