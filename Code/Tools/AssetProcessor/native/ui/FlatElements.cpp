/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "native/ui/FlatElements.h"

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

// ----------------------------------------------------------------------------------
// The flattened GUI's AssetProcessor.qss
//
// Every value is the one Native finally rendered for the widgets its element covers, with the file:line of the declaration that
// grants it (a .qss file of the legacy sheets) and of the Qt code it reproduces (v6.10.2). A value no capture shows is marked
// UNVERIFIED; one BoxElement has no field for is marked OPEN. What the sheets give a widget and the Global elements already
// give every widget the same way is not repeated here.
//
// The cascade across the sheets: this sheet is set on the main window (and on its detail panels), the global sheets on the
// application, and a rule of a widget's sheet outranks every rule of the application sheet whatever its specificity
// (qstylesheetstyle.cpp:1657-1658 and :1689 give the two their depths, :2220-2222 weighs a rule by it before its specificity).
// So where a rule of this sheet gives a property in every state, the :hover and :disabled rules of the global sheets do not
// reach it: `FilteredSearchWidget QLineEdit { background-color }` keeps its fill when the field is disabled.
//
// Widgets an inline .ui sheet styles (the five Settings dividers `background-color: #616161;`, the labels `font: bold;` and
// `font: 12pt;`) are not rows: QStyleSheetStyle still draws them in the flattened GUI, over what the elements give them
// (FlatFramePaintTests.Sheet_GivingTheFrameABackground_OwnsItsFillAndShades; the n2 and f3 fonts of builderInfoHeaderTitleName
// and assetNameLabel are equal).
//
// The colors of the views (AssetProcessor.qss:50-112, :133-167; AssetsTab.qss:84-105: the alternate and the selection colors, the
// fills of the items and branches by state, the sections of the headers) are item view elements, laid over the Global element of the kind
// of the view as the legacy cascade laid these rules over TableView.qss; the fill of the view itself is its box element (ItemView below).
// No row, as the rule says what '*' or the global sheets already say: `ConnectionEditDialog QLabel { background-color:
// transparent }` (AssetProcessor.qss:129-131) and the same for the labels and check boxes of the new log tab dialog
// (NewLogTabDialog.qss:9-13, the sheet of AzToolsFramework that AssetProcessor.qss imports at :11; BaseStyleSheet.qss:17,
// CheckBox.qss:12), `FilterCriteriaButton { max-height: 22px }` and `... QLabel { padding-left, padding-right: 0px }`
// (AssetsTab.qss:47-59, FilteredSearchWidget.qss:26-35, :50-55). Dead, as no widget has the name or the class:
// `QPushButton#removeButton` (AssetProcessor.qss:203-209) and `.solo` (AssetsTab.qss:115-117).
// ----------------------------------------------------------------------------------

namespace AssetProcessor
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others into one
    //! translation unit, where anonymous namespaces merge (the AzQtComponents elements do the same).
    namespace FlatElementsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::FrameElement;
        using AzQtComponents::ItemViewElement;
        using AzQtComponents::TextElement;
        using Widget = const QWidget*;

        // ------------------------------------------------------------------------------
        // Selectors: the QSS each function stands for
        // ------------------------------------------------------------------------------

        //! `#name`: the object name (qstylesheetstyle.cpp:1604-1605)
        bool named(Widget widget, const char* objectName)
        {
            return TextElement::named(widget, objectName);
        }

        //! `.tag`: a whole word of the class property, compared with case (qcssparser.cpp:2166-2178)
        bool tagged(Widget widget, const char* tag)
        {
            return TextElement::tagged(widget, tag);
        }

        //! `#name W`: any ancestor of the widget has the object name (qstylesheetstyle.cpp:1608-1609)
        bool within(Widget widget, const char* ancestorName)
        {
            return TextElement::within(widget, ancestorName);
        }

        //! `Class W`: any ancestor is of the class or of one derived from it (qstylesheetstyle.cpp:1579-1600)
        bool withinClass(Widget widget, const char* ancestorClass)
        {
            return TextElement::withinClass(widget, ancestorClass);
        }

        //! `[name="value"]`: the property read as text (qstylesheetstyle.cpp:1529-1577; qcssparser.cpp:2163-2165)
        bool hasValue(Widget widget, const char* property, const char* value)
        {
            return widget->property(property).toString() == QLatin1String(value);
        }

        // ------------------------------------------------------------------------------
        // Fills shared by several elements
        // ------------------------------------------------------------------------------

        //! `* { background-color: transparent }` (BaseStyleSheet.qss:17): the fill of every widget no other rule fills. It still
        //! hands the widget's background to the rule (QRenderRule::hasBackground, qstylesheetstyle.cpp:507), so the base style
        //! paints none under it. TOKENIZE: WidgetFill
        const QBrush Transparent(QColor(0, 0, 0, 0));

        //! The lists, tables and trees: AssetProcessor.qss:65 (the asset trees), :134 (QTableWidget), :152 (QListWidget) and
        //! AssetsTab.qss:87 (TableView, #builderList), rgb(45,45,45). TOKENIZE: ItemViewSurface
        const QBrush ItemViewFill(QColor(45, 45, 45));

        //! The filter bars: AssetsTab.qss:19, :25 and :32, rgb(40,40,40). TOKENIZE: FilterBarSurface
        const QBrush FilterBarFill(QColor(40, 40, 40));

        // ------------------------------------------------------------------------------
        // Text: the status row
        // ------------------------------------------------------------------------------

        //! `.APTimer { font-size: 18px; font-family: "Open Sans Light" }` (AssetProcessor.qss:14, :17): the three timers of the status
        //! row (MainWindow.ui:139, 181, 223 give them the class), over the '*' rule's weight 400 (BaseStyleSheet.qss:20) and white
        //! text (BaseStyleSheet.qss:18). TOKENIZE: TimerTextFamily, TimerTextSize, TextColor
        const TextElement TimerText{
            .m_name = "AssetProcessor.Text.APTimer",
            .m_family = "Open Sans Light",
            .m_pixelSize = 18,
            .m_color = QColor(0xFF, 0xFF, 0xFF),
        };

        // ------------------------------------------------------------------------------
        // Frames
        // ------------------------------------------------------------------------------

        //! AssetProcessor.qss:20-22 `.TimerLine { color: #888888 }`: the three vertical lines between the timers of the
        //! status row (MainWindow.ui:160-162, 202-204, 244-246 give them the class). A QFrame with a class tag is not
        //! matched by `.QFrame[frameShape]`, so the lines have no fill of their own. TOKENIZE: TimerLineColor
        const FrameElement TimerLine{ .m_name = "AssetProcessor.Frame.TimerLine", .m_foreground = QColor(0x88, 0x88, 0x88) };

        // ------------------------------------------------------------------------------
        // Boxes: the status row
        // ------------------------------------------------------------------------------

        //! Widgets whose rules declare a margin of 0 on every side they name, over the '*' fill: the three timers `.APTimer
        //! { margin-top: 0px; margin-bottom: 0px }` (AssetProcessor.qss:15-16), the labels beside them `QLabel#lastScanLabel,
        //! #analysisLabel, #processingLabel { margin-top, margin-left, margin-bottom: 0px }` (:32-34), and the log page's stack
        //! `QStackedWidget#jobLogStackedWidget { margin-bottom: 0px }` (AssetsTab.qss:112). The 0 is a declared margin, which
        //! hands the box to the rule (qcssparser.cpp:570-596, QRenderRule::hasBox, qstylesheetstyle.cpp:531), and it wins over
        //! the 1px top and bottom of every QLabel (Text.qss:93-94). Captured: all seven sites.
        const BoxElement NoMargin{
            .m_name = "AssetProcessor.Box.NoMargin",
            .m_hasBox = true,
            .m_idle = { .m_fill = Transparent },
        };

        //! `QLabel#projectLabel, QLabel#rootLabel { margin-top: 0px; margin-bottom: 0px; margin-left: 8px }` (AssetProcessor.qss:39-41)
        const BoxElement ProjectLabel{
            .m_name = "AssetProcessor.Box.ProjectLabel",
            .m_margin = QMargins(8, 0, 0, 0),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QLabel#portLabel { margin-top: 0px; margin-bottom: 24px; margin-left: 8px }` (AssetProcessor.qss:45-47)
        const BoxElement PortLabel{
            .m_name = "AssetProcessor.Box.PortLabel",
            .m_margin = QMargins(8, 0, 0, 24),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QLabel#APStatusValueLabel { margin-top: 12px; margin-left: 8px }` (AssetProcessor.qss:25-26) over the margin-bottom of
        //! the `.Title` it is tagged with (Text.qss:69; its margin-top, :68, lost to the id)
        const BoxElement APStatusValueLabel{
            .m_name = "AssetProcessor.Box.APStatusValueLabel",
            .m_margin = QMargins(8, 12, 0, 7),
            .m_idle = { .m_fill = Transparent },
        };

        //! The help buttons: `QPushButton#sharedCacheSupport, #supportButton, #MissingProductDependenciesSupport` (AssetProcessor.qss:
        //! 114-127). A box 24 high with a 10px margin above and 16px at the right (:117-121; the margin is not respected without a
        //! min and a max height, :117), no border (:123), and the help icon at 24px (:125-126, qproperty-icon / qproperty-iconSize).
        //! The three are flat buttons, which PushButton.qss:48-52 leaves transparent (the '*' fill does as well). Captured:
        //! supportButton and sharedCacheSupport; UNVERIFIED by capture: MissingProductDependenciesSupport (the Product tab).
        const BoxElement SupportButton{
            .m_name = "AssetProcessor.Box.SupportButton",
            .m_margin = QMargins(0, 10, 16, 0),
            .m_nativeBorder = false,
            .m_minSize = QSize(-1, 24),
            .m_maxSize = QSize(-1, 24),
            .m_idle = { .m_fill = Transparent },
            .m_properties = { { "icon", QStringLiteral(":/stylesheet/img/help.svg") }, { "iconSize", QSize(24, 24) } },
        };

        // ------------------------------------------------------------------------------
        // Boxes: the shared cache page
        // ------------------------------------------------------------------------------

        //! `QLabel#sharedCacheStatus[highlight = "blue"|"green"|"red"]` (AssetProcessor.qss:174-193): the status line of the page
        //! (MainWindow.cpp:949-989 sets the property and repolishes the label). A 1px border of its own over a tint of 10% of its
        //! colour (rgba(r, g, b, 0.1): the alpha reads as 0.1 * 255 = 25, qcssparser.cpp:830-836), and the 1px top and bottom
        //! margin of every QLabel (Text.qss:93-94). Its text is a plain label's: white, weight normal (AssetProcessor.qss:175-176,
        //! :182-183, :189-190; BaseStyleSheet.qss:18, :20). A label with no highlight is a plain label too: its rule
        //! (AssetProcessor.qss:169-172) says what '*' says.
        //! Captured: blue (the Notice level). UNVERIFIED by capture: green (Active) and red (Error).
        //! TOKENIZE: StatusNoticeFill, StatusNoticeLine, StatusActiveFill, StatusActiveLine, StatusErrorFill, StatusErrorLine
        const BoxElement SharedCacheStatusNotice{
            .m_name = "AssetProcessor.Box.SharedCacheStatus.Notice",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(QColor(88, 140, 188, 25)), .m_edges = BoxElement::border(1, QColor(0x42, 0x85, 0xF4)) },   // AssetProcessor.qss:177-178
        };
        const BoxElement SharedCacheStatusActive{
            .m_name = "AssetProcessor.Box.SharedCacheStatus.Active",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(QColor(88, 188, 97, 25)), .m_edges = BoxElement::border(1, QColor(0x58, 0xBC, 0x61)) },   // AssetProcessor.qss:184-185
        };
        const BoxElement SharedCacheStatusError{
            .m_name = "AssetProcessor.Box.SharedCacheStatus.Error",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(QColor(255, 0, 0, 25)), .m_edges = BoxElement::border(1, QColor(0xFA, 0x27, 0x27)) },   // AssetProcessor.qss:191-192
        };

        //! `QToolButton#serverAddressToolButton` (AssetProcessor.qss:195-201): no border (border-width :197), no radius (:198), no
        //! padding (:200); a declared 0 is a box (qcssparser.cpp:570-596). `min-width: -1px` (:196) sets nothing (qstylesheetstyle.cpp:
        //! 2596-2615 reads a limit only above -1). Over the '*' fill, the border image of :199 is the look of the button:
        //! `border-image: url(":/Browse_on.png") 0 0 0 0 stretch stretch`, which Qt stretches over the whole border box, whatever the size
        //! of the image (QRenderRule::drawBorderImage, qstylesheetstyle.cpp:1167-1188; Face::m_borderImage). The resource is AssetProcessor.qrc's
        //! (:22), and the button has the same file as its icon (MainWindow.cpp:753). n2 SharedCachePage7: the button is 19x19 at [1171,308] at
        //! 150%; the sheet has no rule for any other state of it. TOKENIZE: ServerAddressImage
        const BoxElement ServerAddressToolButton{
            .m_name = "AssetProcessor.Box.ServerAddressToolButton",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = Transparent, .m_borderImage = ":/Browse_on.png" },     // border-image: AssetProcessor.qss:199
        };

        //! `QTableWidget { background-color: rgb(45,45,45) }` (AssetProcessor.qss:134) over the borderless views of TableView.qss:28
        //! and the margin of every scroll area (QDockWidget.qss:35); `QTableView { qproperty-showGrid: false }` (TableView.qss:11).
        //! The rule names the class, so the matcher takes every QTableWidget: the window has one (MainWindow.ui:2339, captured) and
        //! its detail panels five more (SourceAssetDetailsPanel.ui:353, 420, 481, 543, ProductAssetDetailsPanel.ui:903; UNVERIFIED by
        //! capture). Needs the door of the view's viewport.
        const BoxElement SharedCacheTable{
            .m_name = "AssetProcessor.Box.SharedCacheTable",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = ItemViewFill },
            .m_properties = { { "showGrid", false } },
        };

        //! `QWidget#SharedCachePage, QWidget#AssetRelocationPage { background-color: rgba(34, 34, 34, 0.5); border: none }`
        //! (AssetProcessor.qss:212-213, :217-218): the two pages over the dialog stack. The alpha reads as 0.5 * 255 = 127
        //! (qcssparser.cpp:830-836). TOKENIZE: PageOverlaySurface
        const BoxElement PageOverlay{
            .m_name = "AssetProcessor.Box.PageOverlay",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(QColor(34, 34, 34, 127)) },
        };

        // ------------------------------------------------------------------------------
        // Boxes: the jobs, assets and logs pages
        // ------------------------------------------------------------------------------

        //! `QSplitter#jobDialogSplitter { qproperty-childrenCollapsible: False }` (AssetsTab.qss:10), over the '*' fill
        const BoxElement JobDialogSplitter{
            .m_name = "AssetProcessor.Box.JobDialogSplitter",
            .m_idle = { .m_fill = Transparent },
            .m_properties = { { "childrenCollapsible", false } },
        };

        //! `QFrame#jobLogLabel, QFrame#jobContextLogLabel { margin-top: 12px }` (AssetsTab.qss:121) over the margin-bottom of every
        //! QLabel (Text.qss:94). Captured: jobLogLabel. UNVERIFIED by capture: jobContextLogLabel.
        const BoxElement JobLogLabel{
            .m_name = "AssetProcessor.Box.JobLogLabel",
            .m_margin = QMargins(0, 12, 0, 1),
            .m_idle = { .m_fill = Transparent },
        };

        //! `QLabel#jobLogPlaceholderLabel { background-color: rgb(34,34,34); padding: 8px; padding-top: 16px }` (AssetsTab.qss:125-127)
        //! over the margins of every QLabel (Text.qss:93-94). UNVERIFIED by capture (shown when no row is selected).
        //! TOKENIZE: PlaceholderSurface
        const BoxElement JobLogPlaceholderLabel{
            .m_name = "AssetProcessor.Box.JobLogPlaceholderLabel",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_padding = QMargins(8, 16, 8, 8),
            .m_idle = { .m_fill = QBrush(QColor(34, 34, 34)) },
        };

        //! `QFrame#logButtonWrapper { margin-top: 8px }` (LogsTab.qss:13)
        const BoxElement LogButtonWrapper{
            .m_name = "AssetProcessor.Box.LogButtonWrapper",
            .m_margin = QMargins(0, 8, 0, 0),
            .m_idle = { .m_fill = Transparent },
        };

        //! The lists, tables and trees of the sheets: `AzQtComponents--TableView, #builderList` (AssetsTab.qss:87), `QListWidget`
        //! (AssetProcessor.qss:152) and the five asset trees (:65) are filled rgb(45,45,45). No view has a border (TableView.qss:28,
        //! `border: none`) or a margin (QDockWidget.qss:35, `QAbstractScrollArea { margin: 0 }`: a declared 0, which is a box).
        //! Captured: the TableViews, builderList, SourceAssetsTreeView and missingDependencyScanResults. UNVERIFIED by capture: the
        //! other four trees. Needs the door of the view's viewport.
        //! OPEN: builderInfoPatternsTableView gets its indentation 0 from MainWindow.ui:1358, which Native may have reset to the
        //! style's as it did the tables of AssetBundler (TableView.cpp:324-326); no row of it is captured, so none is carried.
        const BoxElement ItemView{
            .m_name = "AssetProcessor.Box.ItemView",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = ItemViewFill },
        };

        // ------------------------------------------------------------------------------
        // Item views: the colors of the lists, tables and trees. An element lists what its rules change over the Global element of
        // the kind of the view (TableView.qss: alternate rgb(77,77,77), selection rgb(101,101,101), hover rgb(90,90,90), header #2d2d2d),
        // a face after the Global ones replacing them for the states it names.
        // ------------------------------------------------------------------------------

        //! rgb(34,34,34): the even rows, and the sections of every header of the sheet (AssetProcessor.qss:135, :148, :153, :166;
        //! AssetsTab.qss:88, :104). The header of the sheet's views is this dark: n2 measures #222222 on every header of the window (the
        //! job, log, connection, builder, shared cache and event log tables), f5 #2d2d2d. TOKENIZE: ItemViewEvenRow, ItemViewHeaderSection
        const QColor ViewDark(34, 34, 34);

        //! rgb(73,73,73): the selection, as the role of the view and as the fill of a selected item and branch (AssetProcessor.qss:81-97,
        //! :136, :140, :154, :158; AssetsTab.qss:90, :95). TOKENIZE: ItemViewSelection
        const QColor ViewSelected(73, 73, 73);

        //! rgb(60,60,60): the item and branch under the pointer, and a selected item while its window is inactive (AssetProcessor.qss:68-79,
        //! :99-110, :143-145, :161-163; AssetsTab.qss:98-101). TOKENIZE: ItemViewHover, ItemViewInactiveSelection
        const QColor ViewHover(60, 60, 60);

        //! rgb(192,192,192): the text of a selected item while its window is inactive (AssetProcessor.qss:111). TOKENIZE: ItemViewInactiveSelectionText
        const QColor ViewInactiveSelectedText(192, 192, 192);

        //! `AzQtComponents--TableView` (AssetsTab.qss:86-105), the jobs, logs, connections, builders and shared cache tables of the window
        //! and the log tabs, which are TableViews: alternate rgb(34,34,34) and selection rgb(73,73,73) as the roles of the view (:88, :90),
        //! the selected item and branch rgb(73,73,73) (:93-96), the hovered ones rgb(60,60,60) (:98-101), the sections rgb(34,34,34) (:103-105).
        //! The fill of the view itself, rgb(45,45,45) (:87), is the box element's.
        const ItemViewElement TableViewRows{
            .m_name = "AssetProcessor.ItemView.TableView",
            .m_alternate = ViewDark,                                                // alternate-background-color: AssetsTab.qss:88
            .m_selection = ViewSelected,                                            // selection-background-color: AssetsTab.qss:90
            .m_faces = {
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },    // ::item:selected, ::branch:selected: AssetsTab.qss:93-96
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },          // ::item:hover, ::branch:hover: AssetsTab.qss:98-101
            },
            .m_headerFill = ViewDark,                                               // QHeaderView::section background-color: AssetsTab.qss:103-105
        };

        //! `QTableWidget` (AssetProcessor.qss:133-149), the tables of the window and of its detail panels: the same colors as the TableView's
        //! (:135, :136, :139-141, :143-145, :147-149); the fill rgb(45,45,45) (:134) is the box element's.
        const ItemViewElement TableWidgetRows{
            .m_name = "AssetProcessor.ItemView.TableWidget",
            .m_alternate = ViewDark,                                                // alternate-background-color: AssetProcessor.qss:135
            .m_selection = ViewSelected,                                            // selection-background-color: AssetProcessor.qss:136
            .m_faces = {
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },    // ::item:selected: AssetProcessor.qss:139-141
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },          // ::item:hover: AssetProcessor.qss:143-145
            },
            .m_headerFill = ViewDark,                                               // QHeaderView::section background-color: AssetProcessor.qss:147-149
        };

        //! The tables and the list of the detail panels that the sheet names (AssetProcessor.qss:104-109 `QTableWidget#productTable:item:selected:!active`
        //! and its five twins): a selected item of an inactive window is rgb(60,60,60) with rgb(192,192,192) text (:110-111). The selectors write
        //! `:item` with one colon, and a name Qt does not know after a single colon is a pseudo-element (Selector::pseudoElement, qcssparser.cpp:
        //! 1994-1999), so they are `::item:selected:!active`. UNVERIFIED by capture: no capture holds an inactive window of the panels.
        const ItemViewElement DetailTableRows{
            .m_name = "AssetProcessor.ItemView.DetailTable",
            .m_alternate = ViewDark,                                                // QTableWidget: AssetProcessor.qss:135
            .m_selection = ViewSelected,                                            // QTableWidget: AssetProcessor.qss:136
            .m_faces = {
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },    // QTableWidget::item:selected: AssetProcessor.qss:139-141
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },          // QTableWidget::item:hover: AssetProcessor.qss:143-145
                { ItemViewElement::Selected | ItemViewElement::Inactive,
                    { ViewHover, QColor(), ViewInactiveSelectedText }, ItemViewElement::OnItem },   // :item:selected:!active: :104-111
            },
            .m_headerFill = ViewDark,                                               // QTableWidget QHeaderView::section: AssetProcessor.qss:147-149
        };
        const ItemViewElement DetailListRows{
            .m_name = "AssetProcessor.ItemView.DetailList",
            .m_alternate = ViewDark,                                                // QListWidget: AssetProcessor.qss:153
            .m_selection = ViewSelected,                                            // QListWidget: AssetProcessor.qss:154
            .m_faces = {
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },    // QListWidget::item:selected: AssetProcessor.qss:157-159
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },          // QListWidget::item:hover: AssetProcessor.qss:161-163
                { ItemViewElement::Selected | ItemViewElement::Inactive,
                    { ViewHover, QColor(), ViewInactiveSelectedText }, ItemViewElement::OnItem },   // :item:selected:!active: :105, :110-111
            },
        };

        //! `QListWidget` (AssetProcessor.qss:151-167): the same, over a list without a header
        const ItemViewElement ListWidgetRows{
            .m_name = "AssetProcessor.ItemView.ListWidget",
            .m_alternate = ViewDark,                                                // alternate-background-color: AssetProcessor.qss:153
            .m_selection = ViewSelected,                                            // selection-background-color: AssetProcessor.qss:154
            .m_faces = {
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },    // ::item:selected: AssetProcessor.qss:157-159
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },          // ::item:hover: AssetProcessor.qss:161-163
            },
        };

        //! `#builderList` (AssetsTab.qss:86-91, the selector shared with the TableView's): the roles of the table and no rule for its items,
        //! which have the Global element's
        const ItemViewElement BuilderListRows{
            .m_name = "AssetProcessor.ItemView.BuilderList",
            .m_alternate = ViewDark,                                                // alternate-background-color: AssetsTab.qss:88
            .m_selection = ViewSelected,                                            // selection-background-color: AssetsTab.qss:90
        };

        //! The five asset trees (`QTreeView#SourceAssetsTreeView` and its four twins, AssetProcessor.qss:50-112): their items and branches
        //! are rgb(45,45,45) in every state a rule does not change (:50-66), rgb(60,60,60) under the pointer (:68-79), rgb(73,73,73) while
        //! selected (:81-97), and an item selected in an inactive window rgb(60,60,60) with rgb(192,192,192) text (:99-112: a rule for the
        //! item alone, the branch keeps its :selected fill, :81-82). The entries are in the order of the sheet: the hovered row of a selected
        //! item is the selected color, as :81 follows :68. TOKENIZE: AssetTreeSurface
        const ItemViewElement AssetTreeRows{
            .m_name = "AssetProcessor.ItemView.AssetTree",
            .m_faces = {
                { 0, { QColor(45, 45, 45), QColor(), QColor() } },                                  // ::item, ::branch: AssetProcessor.qss:50-66
                { ItemViewElement::Hover, { ViewHover, QColor(), QColor() } },                      // ::item:hover, ::branch:hover: :68-79
                { ItemViewElement::Selected, { ViewSelected, QColor(), QColor() } },                // :selected, :selected:active: :81-97
                { ItemViewElement::Selected | ItemViewElement::Inactive,
                    { ViewHover, QColor(), ViewInactiveSelectedText }, ItemViewElement::OnItem },   // ::item:selected:!active: :99-103, :110-111
            },
        };

        //! The tree of the type popup of a filter bar (`#jobFilteredSearchWidget QTreeView, #detailsFilterWidget QTreeView`,
        //! AssetsTab.qss:67-77): one category only, so no indentation (`qproperty-indentation: 0`, :75), which the Global tree sets again
        //! when the tree is shown unless its element says otherwise. UNVERIFIED by capture (a popup is never open in a capture).
        const ItemViewElement FilterPopupRows{
            .m_name = "AssetProcessor.ItemView.FilterPopup",
            .m_indentation = 0,                                                     // qproperty-indentation: AssetsTab.qss:75
        };

        // ------------------------------------------------------------------------------
        // Boxes: the filter bars (AzQtComponents::FilteredSearchWidget)
        // ------------------------------------------------------------------------------

        //! `FilteredSearchWidget#jobFilteredSearchWidget, #detailsFilterWidget` (AssetsTab.qss:15-21): padding 8px, the bar's
        //! fill, and the text filter beside its type filter rather than across the widget (qproperty-textFilterFillsWidth: False)
        const BoxElement DetailsFilterWidget{
            .m_name = "AssetProcessor.Box.DetailsFilterWidget",
            .m_padding = QMargins(8, 8, 8, 8),
            .m_idle = { .m_fill = FilterBarFill },
            .m_properties = { { "textFilterFillsWidth", false } },
        };

        //! `FilteredSearchWidget#assetDataFilteredSearchWidget` (AssetsTab.qss:23-27): the same, and the text filter across the widget
        //! (:26, the later rule wins over :20)
        const BoxElement AssetDataFilteredSearchWidget{
            .m_name = "AssetProcessor.Box.AssetDataFilteredSearchWidget",
            .m_padding = QMargins(8, 8, 8, 8),
            .m_idle = { .m_fill = FilterBarFill },
            .m_properties = { { "textFilterFillsWidth", true } },
        };

        //! `FilteredSearchWidget #textSearchContainer { background-color: rgb(40,40,40) }` (AssetsTab.qss:29-33), which holds the
        //! search field and the type button
        const BoxElement TextSearchContainer{
            .m_name = "AssetProcessor.Box.TextSearchContainer",
            .m_idle = { .m_fill = FilterBarFill },
        };

        //! `FilteredSearchWidget #filteredParent` (AssetsTab.qss:29-33, :42-44): the row of filter chips, shown once a filter is on.
        //! The bar's fill and 6px above. UNVERIFIED by capture (a chip is never on in a capture).
        const BoxElement FilteredParent{
            .m_name = "AssetProcessor.Box.FilteredParent",
            .m_margin = QMargins(0, 6, 0, 0),
            .m_idle = { .m_fill = FilterBarFill },
        };

        //! `FilteredSearchWidget #filteredLayout` (AssetsTab.qss:29-33, :62-65): the chips' own frame. The bar's fill and a margin of
        //! -4px, which negates the flow layout's (AssetsTab.qss:63-64). UNVERIFIED by capture.
        const BoxElement FilteredLayout{
            .m_name = "AssetProcessor.Box.FilteredLayout",
            .m_margin = QMargins(-4, -4, -4, -4),
            .m_idle = { .m_fill = FilterBarFill },
        };

        //! The search field of a bar, a line edit with the search action (HasSearchAction): `FilteredSearchWidget QLineEdit
        //! { background-color: #e9e9e9 }` (AssetsTab.qss:37-39) over the global line edit: a 2px solid border (LineEdit.qss:16-17) of
        //! #e9e9e9 (BaseStyleSheet.qss:81), rounded 2px (BaseStyleSheet.qss:79), the text pulled 6px left by the search action
        //! (`QLineEdit[HasSearchAction=true] { padding-left: -6px }`, LineEdit.qss:22-26, over :15) and 1px below, -1px above
        //! (:18-19), 16px high (:13) and 260px wide (FilteredSearchWidget.qss:12), with a frame (qproperty-frame,
        //! FilteredSearchWidget.qss:11). The same in every state: `QLineEdit:disabled { background-color: #666666 }` (LineEdit.qss:28-35)
        //! is a rule of the application sheet, which the fill of AssetsTab.qss:38 outranks (see the top of this file): the n2 dump of a
        //! search field has Base, Button and Window #e9e9e9 in its disabled colour group as well (its disabled text is #999999, the
        //! text of that rule). Captured: idle, and the disabled group of the palette. Needs the QLineEdit door.
        //! TOKENIZE: SearchFieldSurface, SearchFieldRing
        const BoxElement TextSearch{
            .m_name = "AssetProcessor.Box.TextSearch",
            .m_padding = QMargins(-6, -1, 0, 1),
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(260, 16),
            .m_idle = { .m_fill = QBrush(QColor(0xE9, 0xE9, 0xE9)), .m_edges = BoxElement::border(2, QColor(0xE9, 0xE9, 0xE9)) },
            .m_properties = { { "frame", true } },
        };

        //! `#jobFilteredSearchWidget QTreeView, #detailsFilterWidget QTreeView` (AssetsTab.qss:67-77): the tree of a bar's type popup
        //! (AzQtComponents::SearchTypeSelector) stands out with a 1px border rgb(70,70,70) (:71), rounded 2px (:72), over rgb(34,34,34)
        //! (:70); one category only, so no indentation and no expanding on a double click (:75-76). A margin of 0 as every scroll
        //! area has (QDockWidget.qss:35). UNVERIFIED by capture (a popup is never open in a capture). Needs the door of the
        //! view's viewport. TOKENIZE: FilterPopupSurface, FilterPopupLine
        const BoxElement FilterPopupTree{
            .m_name = "AssetProcessor.Box.FilterPopupTree",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_idle = { .m_fill = QBrush(QColor(34, 34, 34)), .m_edges = BoxElement::border(1, QColor(70, 70, 70)) },
            .m_properties = { { "indentation", 0 }, { "expandsOnDoubleClick", false } },
        };

        //! `#jobFilteredSearchWidget AzQtComponents--SearchTypeSelector, #detailsFilterWidget AzQtComponents--SearchTypeSelector
        //! { qproperty-lineEditSearchVisible: 0 }` (AssetsTab.qss:79-82): the type popup of these two bars has no text filter of its
        //! own (one category to choose from). A property and no box. UNVERIFIED by capture (a popup is never open in a capture).
        const BoxElement FilterPopup{
            .m_name = "AssetProcessor.Box.FilterPopup",
            .m_properties = { { "lineEditSearchVisible", false } },
        };
    } // namespace FlatElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration: the first registered row that matches a widget wins, so the narrower selector comes first
    // ----------------------------------------------------------------------------------

    void RegisterFlatElements()
    {
        using namespace FlatElementsInternal;

        // ---- text
        // `.APTimer` is a class tag of any widget; the three timers are labels
        TextElement::registerElement(&TimerText, &QLabel::staticMetaObject, [](Widget w) { return tagged(w, "APTimer"); });

        // ---- frames
        FrameElement::registerElement(&TimerLine, &QFrame::staticMetaObject, [](Widget w) { return tagged(w, "TimerLine"); });

        // ---- boxes: the status row
        BoxElement::registerElement(&NoMargin, &QLabel::staticMetaObject, [](Widget w)
            {
                return tagged(w, "APTimer") || named(w, "lastScanLabel") || named(w, "analysisLabel") || named(w, "processingLabel");
            });
        BoxElement::registerElement(&NoMargin, &QStackedWidget::staticMetaObject, [](Widget w) { return named(w, "jobLogStackedWidget"); });
        BoxElement::registerElement(&ProjectLabel, &QLabel::staticMetaObject, [](Widget w) { return named(w, "projectLabel") || named(w, "rootLabel"); });
        BoxElement::registerElement(&PortLabel, &QLabel::staticMetaObject, [](Widget w) { return named(w, "portLabel"); });
        BoxElement::registerElement(&APStatusValueLabel, &QLabel::staticMetaObject, [](Widget w)
            {
                return named(w, "APStatusValueLabel") && tagged(w, "Title");
            });
        BoxElement::registerElement(&SupportButton, &QPushButton::staticMetaObject, [](Widget w)
            {
                return named(w, "supportButton") || named(w, "sharedCacheSupport") || named(w, "MissingProductDependenciesSupport");
            });

        // ---- boxes: the shared cache page
        BoxElement::registerElement(&SharedCacheStatusNotice, &QLabel::staticMetaObject, [](Widget w)
            {
                return named(w, "sharedCacheStatus") && hasValue(w, "highlight", "blue");
            });
        BoxElement::registerElement(&SharedCacheStatusActive, &QLabel::staticMetaObject, [](Widget w)
            {
                return named(w, "sharedCacheStatus") && hasValue(w, "highlight", "green");
            });
        BoxElement::registerElement(&SharedCacheStatusError, &QLabel::staticMetaObject, [](Widget w)
            {
                return named(w, "sharedCacheStatus") && hasValue(w, "highlight", "red");
            });
        BoxElement::registerElement(&ServerAddressToolButton, &QToolButton::staticMetaObject, [](Widget w)
            {
                return named(w, "serverAddressToolButton");
            });
        BoxElement::registerElement(&SharedCacheTable, &QTableWidget::staticMetaObject, [](Widget) { return true; });
        BoxElement::registerElement(&PageOverlay, &QWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "SharedCachePage") || named(w, "AssetRelocationPage");
            });

        // ---- boxes: the jobs, assets and logs pages
        BoxElement::registerElement(&JobDialogSplitter, &QSplitter::staticMetaObject, [](Widget w) { return named(w, "jobDialogSplitter"); });
        BoxElement::registerElement(&JobLogLabel, &QLabel::staticMetaObject, [](Widget w) { return named(w, "jobLogLabel") || named(w, "jobContextLogLabel"); });
        BoxElement::registerElement(&JobLogPlaceholderLabel, &QLabel::staticMetaObject, [](Widget w) { return named(w, "jobLogPlaceholderLabel"); });
        BoxElement::registerElement(&LogButtonWrapper, &QFrame::staticMetaObject, [](Widget w) { return named(w, "logButtonWrapper"); });

        // The views: a TableView (and its subclasses, the log tabs), `#builderList`, every QListWidget and the five asset trees
        BoxElement::registerElement(&ItemView, &AzQtComponents::TableView::staticMetaObject, [](Widget) { return true; });
        BoxElement::registerElement(&ItemView, &QListView::staticMetaObject, [](Widget w) { return named(w, "builderList"); });
        BoxElement::registerElement(&ItemView, &QListWidget::staticMetaObject, [](Widget) { return true; });
        BoxElement::registerElement(&ItemView, &QTreeView::staticMetaObject, [](Widget w)
            {
                return named(w, "SourceAssetsTreeView") || named(w, "IntermediateAssetsTreeView") || named(w, "ProductAssetsTreeView")
                    || named(w, "OutgoingProductDependenciesTreeView") || named(w, "IncomingProductDependenciesTreeView");
            });

        // ---- item views: the colors of the views. The asset trees and the filter popup trees are plain QTreeViews, named; a TableView, a
        // QTableWidget, a QListWidget and the builder list are the classes the sheets' selectors name
        ItemViewElement::registerElement(&AssetTreeRows, &QTreeView::staticMetaObject, [](Widget w)
            {
                return named(w, "SourceAssetsTreeView") || named(w, "IntermediateAssetsTreeView") || named(w, "ProductAssetsTreeView")
                    || named(w, "OutgoingProductDependenciesTreeView") || named(w, "IncomingProductDependenciesTreeView");
            });
        ItemViewElement::registerElement(&FilterPopupRows, &QTreeView::staticMetaObject, [](Widget w)
            {
                return within(w, "jobFilteredSearchWidget") || within(w, "detailsFilterWidget");
            });
        ItemViewElement::registerElement(&TableViewRows, &AzQtComponents::TableView::staticMetaObject, [](Widget) { return true; });
        // the named tables and list of the detail panels, before the others of their classes
        ItemViewElement::registerElement(&DetailTableRows, &QTableWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "outgoingProductDependenciesTable") || named(w, "incomingProductDependenciesTable") || named(w, "productTable")
                    || named(w, "outgoingSourceDependenciesTable") || named(w, "incomingSourceDependenciesTable");
            });
        ItemViewElement::registerElement(&DetailListRows, &QListWidget::staticMetaObject,
            [](Widget w) { return named(w, "outgoingUnmetPathProductDependenciesList"); });
        ItemViewElement::registerElement(&TableWidgetRows, &QTableWidget::staticMetaObject, [](Widget) { return true; });
        ItemViewElement::registerElement(&ListWidgetRows, &QListWidget::staticMetaObject, [](Widget) { return true; });
        ItemViewElement::registerElement(&BuilderListRows, &QListView::staticMetaObject, [](Widget w) { return named(w, "builderList"); });

        // ---- boxes: the filter bars
        BoxElement::registerElement(&DetailsFilterWidget, &AzQtComponents::FilteredSearchWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "jobFilteredSearchWidget") || named(w, "detailsFilterWidget");
            });
        BoxElement::registerElement(&AssetDataFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::staticMetaObject, [](Widget w)
            {
                return named(w, "assetDataFilteredSearchWidget");
            });
        BoxElement::registerElement(&TextSearchContainer, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "textSearchContainer") && withinClass(w, "AzQtComponents::FilteredSearchWidget");
            });
        BoxElement::registerElement(&FilteredParent, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "filteredParent") && withinClass(w, "AzQtComponents::FilteredSearchWidget");
            });
        BoxElement::registerElement(&FilteredLayout, &QFrame::staticMetaObject, [](Widget w)
            {
                return named(w, "filteredLayout") && withinClass(w, "AzQtComponents::FilteredSearchWidget");
            });
        BoxElement::registerElement(&TextSearch, &QLineEdit::staticMetaObject, [](Widget w)
            {
                return w->property("HasSearchAction").toBool() && withinClass(w, "AzQtComponents::FilteredSearchWidget");
            });
        BoxElement::registerElement(&FilterPopup, &AzQtComponents::SearchTypeSelector::staticMetaObject, [](Widget w)
            {
                return within(w, "jobFilteredSearchWidget") || within(w, "detailsFilterWidget");
            });
        BoxElement::registerElement(&FilterPopupTree, &QTreeView::staticMetaObject, [](Widget w)
            {
                return within(w, "jobFilteredSearchWidget") || within(w, "detailsFilterWidget");
            });
    }
} // namespace AssetProcessor
