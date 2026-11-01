/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTreeView>
#include <QWidget>

// ----------------------------------------------------------------------------------
// The flattened GUI's AssetBundler.qss
//
// Every value is the one Native finally rendered for the widgets its element covers, with the file:line of the declaration that
// grants it (AssetBundler.qss, or a sheet of AzQtComponents named in full) and of the Qt code it reproduces (v6.10.2). A value
// no capture shows is marked UNVERIFIED. What the sheets give every widget the same way is not repeated here.
//
// The header sections `QHeaderView::section` (:28-30, :56-58) are item view elements (the log table's and the tables under a tab
// widget). Not carried, as no element of these families holds it: `QTabWidget:pane { border: none }` (AssetBundler.qss:11-13, a tab
// element). Dead: `QWidget#PlatformSelectionWidget
// QLabel#platformSelectionLabel` (:60-62), as no label has that name (the one of PlatformSelectionWidget.ui is platformsLabel, and the
// dialogs name the widget itself platformSelectionWidget, AddSeedDialog.ui:125).
// ----------------------------------------------------------------------------------

namespace AssetBundler
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others into one
    //! translation unit, where anonymous namespaces merge (the AzQtComponents elements do the same).
    namespace FlatElementsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::ItemViewElement;
        using AzQtComponents::TextElement;
        using AzQtComponents::Selectors::named;
        using AzQtComponents::Selectors::within;
        using AzQtComponents::Selectors::withinClass;
        using Widget = const QWidget*;

        // ------------------------------------------------------------------------------
        // Fills and lines shared by several elements
        // ------------------------------------------------------------------------------

        //! `* { background-color: transparent }` (BaseStyleSheet.qss:17): the fill of every widget no other rule fills. It still
        //! hands the widget's background to the rule (QRenderRule::hasBackground, qstylesheetstyle.cpp:507), so the base style
        //! paints none under it. TOKENIZE: WidgetFill
        const QBrush Transparent(QColor(0, 0, 0, 0));

        //! The #1B1B1B of the file tables: the fill of their header frames and of the labels in them (AssetBundler.qss:47, :52) and
        //! the line of the frames and of the log table (:22, :36). TOKENIZE: FileTableHeaderSurface, FileTableLine
        const QColor TableLine(0x1B, 0x1B, 0x1B);

        //! `.QFrame[frameShape="6"] { background-color: #444444 }` (BaseStyleSheet.qss:47): the fill of a panel frame, which is what
        //! the .ui files make of both file frames of every tab (StyledPanel). TOKENIZE: PanelSurface
        const QBrush PanelFill(QColor(0x44, 0x44, 0x44));

        //! The surface of the dialogs' frames, labels and check boxes, #555555 (AssetBundler.qss:110, :147). TOKENIZE: DialogSurface
        const QBrush DialogFill(QColor(0x55, 0x55, 0x55));

        // ------------------------------------------------------------------------------
        // Text
        // ------------------------------------------------------------------------------

        //! `QLabel#activeProjectLabel { font-size: 18px; font-family: "Amazon Ember Light" }` (AssetBundler.qss:16-17): the heading of
        //! every tab, over the '*' rule's weight 400 (BaseStyleSheet.qss:20) and white text (BaseStyleSheet.qss:18). The family is not
        //! installed: the string reaches the font database as it did in Native, which drew Tahoma.
        //! TOKENIZE: ProjectHeadingFamily, ProjectHeadingSize, TextColor
        const TextElement ActiveProjectLabelText{
            .m_name = "AssetBundler.Text.ActiveProjectLabel",
            .m_family = "Amazon Ember Light",
            .m_pixelSize = 18,
            .m_color = QColor(0xFF, 0xFF, 0xFF),
        };

        // ------------------------------------------------------------------------------
        // Boxes: the tabs
        // ------------------------------------------------------------------------------

        //! `QLabel#activeProjectLabel { margin-bottom: 5px }` (AssetBundler.qss:18) over the margin-top of every QLabel (Text.qss:93)
        const BoxElement ActiveProjectLabel{
            .m_name = "AssetBundler.Box.ActiveProjectLabel",
            .m_margin = QMargins(0, 1, 0, 5),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QFrame#fileTableFrame, QFrame#fileContentsFrame { border: 1px solid #1B1B1B }` (AssetBundler.qss:36), a border of their
        //! own (QRenderRule::hasNativeBorder, qstylesheetstyle.cpp:511-514), over the panel fill (BaseStyleSheet.qss:47), and
        //! `QFrame#fileTableFrame { margin-right: 10px }` (:40)
        const BoxElement FileTableFrame{
            .m_name = "AssetBundler.Box.FileTableFrame",
            .m_margin = QMargins(0, 0, 10, 0),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = PanelFill, .m_edges = BoxElement::border(1, TableLine) },
        };
        const BoxElement FileContentsFrame{
            .m_name = "AssetBundler.Box.FileContentsFrame",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = PanelFill, .m_edges = BoxElement::border(1, TableLine) },
        };

        //! `QFrame#fileTableHeaderFrame, QFrame#fileContentsHeaderFrame { background-color: #1B1B1B }` (AssetBundler.qss:47): a panel
        //! frame too (StyledPanel) whose border no rule touches, so the style's own is drawn
        const BoxElement FileHeaderFrame{
            .m_name = "AssetBundler.Box.FileHeaderFrame",
            .m_idle = { .m_fill = QBrush(TableLine) },
        };

        //! `QFrame#fileTableHeaderFrame QLabel, QFrame#fileContentsHeaderFrame QLabel` (AssetBundler.qss:52-53): the title in a header,
        //! on the header's fill and 5px from its left, over the margins of every QLabel (Text.qss:93-94)
        const BoxElement FileHeaderLabel{
            .m_name = "AssetBundler.Box.FileHeaderLabel",
            .m_margin = QMargins(5, 1, 0, 1),
            .m_idle = { .m_fill = QBrush(TableLine) },
        };

        //! The same, and `QLabel#assetListContentsLabel { margin-right: 10px }` (AssetBundler.qss:116)
        const BoxElement AssetListContentsLabel{
            .m_name = "AssetBundler.Box.AssetListContentsLabel",
            .m_margin = QMargins(5, 1, 10, 1),
            .m_idle = { .m_fill = QBrush(TableLine) },
        };

        //! `QWidget#fileFilteredSearchWidget, QWidget#assetListContentsFilteredSearchWidget { margin-left: 5px; margin-right: 5px }`
        //! (AssetBundler.qss:121-122)
        const BoxElement FileFilteredSearchWidget{
            .m_name = "AssetBundler.Box.FileFilteredSearchWidget",
            .m_margin = QMargins(5, 0, 5, 0),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QWidget#seedListContentsFilteredSearchWidget { margin-left: 5px }` (AssetBundler.qss:87)
        const BoxElement SeedListContentsFilteredSearchWidget{
            .m_name = "AssetBundler.Box.SeedListContentsFilteredSearchWidget",
            .m_margin = QMargins(5, 0, 0, 0),
            .m_idle = { .m_fill = Transparent },
        };

        //! The push buttons the sheet spaces `margin-left: 10px; margin-right: 10px` (AssetBundler.qss): a tab's action button (:126-127,
        //! :159-160), the buttons of the seed tab and of the dialogs it opens (:78-79), of the new file dialog (:67-68) and all of the
        //! bundle settings dialog (:151-152). The face stays the style's own, inside the box. Captured: generateAssetListsButton,
        //! addSeedButton, generateBundleButton, runRuleButton, addComparisonStepButton. UNVERIFIED by capture: the dialogs' buttons.
        const BoxElement SpacedButton{
            .m_name = "AssetBundler.Box.SpacedButton",
            .m_margin = QMargins(10, 0, 10, 0),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QCheckBox#selectDefaultSeedListsCheckBox { margin: 5px }` (AssetBundler.qss:83) over the fill of CheckBox.qss:12.
        //! Needs the QCheckBox door.
        const BoxElement SelectDefaultSeedListsCheckBox{
            .m_name = "AssetBundler.Box.SelectDefaultSeedListsCheckBox",
            .m_margin = QMargins(5, 5, 5, 5),
            .m_idle = { .m_fill = Transparent },
        };

        // ------------------------------------------------------------------------------
        // Boxes: the tables
        //
        // Their indentation. The constructors set it (0, and 2 for the seed list table: SeedTabWidget.cpp:72, :100,
        // AssetListTabWidget.cpp:62, :85, BundleListTabWidget.cpp:50, RulesTabWidget.cpp:61, MainWindow.cpp:39), and Native rendered
        // the style's own, 20 (PM_TreeViewIndentation, qcommonstyle.cpp:4837-4839): the first row of logTableView and of the seed
        // list table starts at x=20 in the captures. TableView::unpolish is the only reset of an indentation in the library
        // (TableView.cpp:324-326), and a polished table gets it when another style replaces its own (qwidget.cpp:2692-2696).
        // QTreeView::resetIndentation hands the indentation back to the style (qtreeview.cpp:362-368, :4084-4087). The elements
        // reset the property the same way: an invalid value resets a property that has a reset (QMetaProperty::write,
        // qmetaobject.cpp:4076-4096; qtreeview.h:24). Captured: logTableView and the seed list table. UNVERIFIED by capture:
        // the other tables, set up the same way.
        // ------------------------------------------------------------------------------

        //! `QTreeView#logTableView { border: 1px solid #1B1B1B; margin-left, margin-right, margin-bottom: 10px }` (AssetBundler.qss:22-25),
        //! a margin of 0 above as every scroll area has (QDockWidget.qss:35), over the fill of a view in a widget (`QWidget > QTreeView`,
        //! BaseStyleSheet.qss:37). Needs the door of the view's viewport.
        const BoxElement LogTableView{
            .m_name = "AssetBundler.Box.LogTableView",
            .m_margin = QMargins(10, 0, 10, 10),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = Transparent, .m_edges = BoxElement::border(1, TableLine) },
            .m_properties = { { "indentation", QVariant() } },
        };

        //! The other tables: no border (TableView.qss:28), a margin of 0 (QDockWidget.qss:35), the fill of a view in a widget
        //! (BaseStyleSheet.qss:37). Needs the door of the view's viewport.
        const BoxElement ItemView{
            .m_name = "AssetBundler.Box.ItemView",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = Transparent },
            .m_properties = { { "indentation", QVariant() } },
        };

        //! `QScrollArea#comparisonDataList { border: none }` (AssetBundler.qss:164), the margin of 0 of every scroll area
        //! (QDockWidget.qss:35), over the '*' fill. Needs the door of the scroll area's viewport.
        const BoxElement ComparisonDataList{
            .m_name = "AssetBundler.Box.ComparisonDataList",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = Transparent },
        };

        //! `QFrame#comparisonDataFrame { padding: 10px }` (AssetBundler.qss:168), a frame without a shape (ComparisonDataWidget.ui),
        //! so the panel fill does not reach it. UNVERIFIED by capture.
        const BoxElement ComparisonDataFrame{
            .m_name = "AssetBundler.Box.ComparisonDataFrame",
            .m_padding = QMargins(10, 10, 10, 10),
            .m_idle = { .m_fill = Transparent },
        };

        // ------------------------------------------------------------------------------
        // Item views: the sections of the headers. A module's element lists what its rules change over the Global element of the kind of
        // the view (TableView.qss:62-76: the sections are #2d2d2d). n2 measures the viewport of each header: #1b1b1b for the log tables (all
        // four screens that have one) and #4d4d4d for every file table under the tabs, f5 #2d2d2d for all of them.
        // ------------------------------------------------------------------------------

        //! `QTreeView#logTableView QHeaderView::section { background-color: #1B1B1B }` (AssetBundler.qss:28-30): as dark as the table's
        //! frame, which wins over the rule of every table under a tab (more specific: an id). TOKENIZE: LogTableHeaderSection
        const ItemViewElement LogTableHeader{
            .m_name = "AssetBundler.ItemView.LogTableHeader",
            .m_headerFill = TableLine,                                              // background-color: AssetBundler.qss:29
        };

        //! `QTabWidget QTreeView QHeaderView::section { background-color: rgb(77, 77, 77) }` (AssetBundler.qss:56-58): the tables of the file
        //! tabs, whatever their names. TOKENIZE: FileTableHeaderSection
        const ItemViewElement TabTableHeader{
            .m_name = "AssetBundler.ItemView.TabTableHeader",
            .m_headerFill = QColor(77, 77, 77),                                     // background-color: AssetBundler.qss:57
        };

        // ------------------------------------------------------------------------------
        // Boxes: the dialogs. UNVERIFIED by capture, all of them: no dialog is open in a capture
        // ------------------------------------------------------------------------------

        //! `QDialog QFrame#inputValuesFrame { border: none; margin: 10px; padding: 10px }` (AssetBundler.qss:91-93) on the dialogs'
        //! surface (:102-110)
        const BoxElement InputValuesFrame{
            .m_name = "AssetBundler.Box.InputValuesFrame",
            .m_margin = QMargins(10, 10, 10, 10),
            .m_padding = QMargins(10, 10, 10, 10),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = DialogFill },
        };

        //! `QDialog QFrame#lineFrame { border: none; margin-top: 10px; margin-bottom: 10px }` (AssetBundler.qss:97-99) on the dialogs'
        //! surface (:102-110)
        const BoxElement LineFrame{
            .m_name = "AssetBundler.Box.LineFrame",
            .m_margin = QMargins(0, 10, 0, 10),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = DialogFill },
        };

        //! `QFrame#bundleSettingsFrame { border: none; margin-bottom: 10px }` (AssetBundler.qss:133-134) on the surface of :143-147
        const BoxElement BundleSettingsFrame{
            .m_name = "AssetBundler.Box.BundleSettingsFrame",
            .m_margin = QMargins(0, 0, 0, 10),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = DialogFill },
        };

        //! `QFrame#outputBundleLocationFrame, QFrame#bundleSettingsFileFrame { border: none; margin: 10px }` (AssetBundler.qss:139-140)
        //! on the surface of :143-147
        const BoxElement BundleSettingsFileFrame{
            .m_name = "AssetBundler.Box.BundleSettingsFileFrame",
            .m_margin = QMargins(10, 10, 10, 10),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = DialogFill },
        };

        //! The labels of the dialogs and of the bundle settings frame on the dialogs' surface (AssetBundler.qss:104-110, :146-147), with
        //! the margins of every QLabel (Text.qss:93-94)
        const BoxElement DialogLabel{
            .m_name = "AssetBundler.Box.DialogLabel",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_idle = { .m_fill = DialogFill },
        };

        //! The check boxes of the dialogs on the dialogs' surface (AssetBundler.qss:105-110), over the fill of CheckBox.qss:12.
        //! Needs the QCheckBox door.
        const BoxElement DialogCheckBox{
            .m_name = "AssetBundler.Box.DialogCheckBox",
            .m_idle = { .m_fill = DialogFill },
        };
    } // namespace FlatElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration: the first registered row that matches a widget wins, so the narrower selector comes first
    // ----------------------------------------------------------------------------------

    void RegisterFlatElements()
    {
        using namespace FlatElementsInternal;

        // ---- text
        TextElement::registerElement(&ActiveProjectLabelText, &QLabel::staticMetaObject, [](Widget w) { return named(w, "activeProjectLabel"); });

        // ---- boxes: the tabs
        BoxElement::registerElement(&ActiveProjectLabel, &QLabel::staticMetaObject, [](Widget w) { return named(w, "activeProjectLabel"); });
        BoxElement::registerElement(&FileTableFrame, &QFrame::staticMetaObject, [](Widget w) { return named(w, "fileTableFrame"); });
        BoxElement::registerElement(&FileContentsFrame, &QFrame::staticMetaObject, [](Widget w) { return named(w, "fileContentsFrame"); });
        BoxElement::registerElement(&FileHeaderFrame, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "fileTableHeaderFrame") || named(w, "fileContentsHeaderFrame");
            });

        // assetListContentsLabel is a label of the header too, with a margin more: it comes first
        BoxElement::registerElement(&AssetListContentsLabel, &QLabel::staticMetaObject, [](Widget w)
            {
                return named(w, "assetListContentsLabel") && within(w, "fileContentsHeaderFrame");
            });
        BoxElement::registerElement(&FileHeaderLabel, &QLabel::staticMetaObject, [](Widget w)
            {
                return within(w, "fileTableHeaderFrame") || within(w, "fileContentsHeaderFrame");
            });
        BoxElement::registerElement(&FileFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "fileFilteredSearchWidget") || named(w, "assetListContentsFilteredSearchWidget");
            });
        BoxElement::registerElement(&SeedListContentsFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "seedListContentsFilteredSearchWidget");
            });

        // The spaced buttons: one registration for each rule that spaces them
        BoxElement::registerElement(&SpacedButton, &QPushButton::staticMetaObject, [](Widget w)
            {
                return named(w, "generateBundleButton") || named(w, "runRuleButton") || named(w, "addComparisonStepButton");
            });
        BoxElement::registerElement(&SpacedButton, &QPushButton::staticMetaObject, [](Widget w)
            {
                return within(w, "SeedTabWidget")
                    && (named(w, "generateAssetListsButton") || named(w, "browseFileButton") || named(w, "cancelButton")
                        || named(w, "addSeedButton") || named(w, "applyChangesButton"));
            });
        BoxElement::registerElement(&SpacedButton, &QPushButton::staticMetaObject, [](Widget w)
            {
                return within(w, "NewFileDialog")
                    && (named(w, "browseFileButton") || named(w, "createFileButton") || named(w, "cancelButton"));
            });
        BoxElement::registerElement(&SpacedButton, &QPushButton::staticMetaObject, [](Widget w) { return within(w, "GenerateBundlesModal"); });
        BoxElement::registerElement(&SelectDefaultSeedListsCheckBox, &QCheckBox::staticMetaObject, [](Widget w)
            {
                return named(w, "selectDefaultSeedListsCheckBox");
            });

        // ---- item views: the sections of the headers. The log table comes before the tables under a tab, as its rule is the more specific
        ItemViewElement::registerElement(&LogTableHeader, &QTreeView::staticMetaObject, [](Widget w) { return named(w, "logTableView"); });
        ItemViewElement::registerElement(&TabTableHeader, &QTreeView::staticMetaObject, [](Widget w) { return withinClass(w, "QTabWidget"); });

        // ---- boxes: the tables. The log table comes before the others, as it is one of them
        BoxElement::registerElement(&LogTableView, &QTreeView::staticMetaObject, [](Widget w) { return named(w, "logTableView"); });
        BoxElement::registerElement(&ItemView, &AzQtComponents::TableView::staticMetaObject, [](Widget) { return true; });
        BoxElement::registerElement(&ComparisonDataList, &QScrollArea::staticMetaObject, [](Widget w) { return named(w, "comparisonDataList"); });
        BoxElement::registerElement(&ComparisonDataFrame, &QFrame::staticMetaObject, [](Widget w) { return named(w, "comparisonDataFrame"); });

        // ---- boxes: the dialogs
        BoxElement::registerElement(&InputValuesFrame, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "inputValuesFrame") && withinClass(w, "QDialog");
            });
        BoxElement::registerElement(&LineFrame, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "lineFrame") && withinClass(w, "QDialog");
            });
        BoxElement::registerElement(&BundleSettingsFrame, &QFrame::staticMetaObject, [](Widget w) { return named(w, "bundleSettingsFrame"); });
        BoxElement::registerElement(&BundleSettingsFileFrame, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "outputBundleLocationFrame") || named(w, "bundleSettingsFileFrame");
            });
        BoxElement::registerElement(&DialogLabel, &QLabel::staticMetaObject, [](Widget w)
            {
                return within(w, "AddSeedDialog") || within(w, "EditSeedDialog") || within(w, "NewFileDialog") || within(w, "bundleSettingsFrame");
            });
        BoxElement::registerElement(&DialogCheckBox, &QCheckBox::staticMetaObject, [](Widget w)
            {
                return within(w, "AddSeedDialog") || within(w, "EditSeedDialog") || within(w, "NewFileDialog");
            });
    }
} // namespace AssetBundler
