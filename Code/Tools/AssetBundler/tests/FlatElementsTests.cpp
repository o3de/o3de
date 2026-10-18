/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <source/ui/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabWidget>
#include <QTreeView>
#include <QWidget>

// ----------------------------------------------------------------------------------
// The elements of AssetBundler.qss (source/ui/FlatElements.cpp) on widgets built as the captured ones are: the names, classes
// and parents of the n2 dumps (UiCaptures/n2/AssetBundler/qss), and for each the box Native measured for it (the `contents` of
// its dump). The registrar is called directly and BoxElement::apply, TextElement::apply are called as Style::polish calls
// them, so nothing here depends on the flattened mode of the process. The tests run under the application main() makes.
// ----------------------------------------------------------------------------------

namespace AssetBundler
{
    namespace FlatElementsTestsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::FilteredSearchWidget;
        using AzQtComponents::ItemViewElement;
        using AzQtComponents::TableView;
        using AzQtComponents::TextElement;

        // ------------------------------------------------------------------------------
        // Fixture: the registrar run once for the process, and Fusion, the base style of the legacy GUI, for the frame a style draws
        // ------------------------------------------------------------------------------

        class FlatElementsTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            FlatElementsTests()
            {
                // The registries last for the process: a second call would only add the same rows again
                static bool registered = false;
                if (!registered)
                {
                    registered = true;
                    RegisterFlatElements();
                }
                m_fusion.reset(QStyleFactory::create(QStringLiteral("Fusion")));
            }

            ~FlatElementsTests() override
            {
                m_fusion.reset();
            }

            //! A widget of the class under a parent, named as the captured one is
            template<typename W>
            static W* add(QWidget* parent, const char* name)
            {
                W* widget = new W(parent);
                widget->setObjectName(QString::fromLatin1(name));
                return widget;
            }

            //! A panel frame of the .ui files of the tabs
            QFrame* addPanel(QWidget* parent, const char* name)
            {
                QFrame* frame = add<QFrame>(parent, name);
                frame->setFrameShape(QFrame::StyledPanel);
                frame->setFrameShadow(QFrame::Raised);
                frame->setStyle(m_fusion.get());           // the 1px of the style's own frame is Fusion's
                return frame;
            }

            //! What Style::polish does for the box of a widget, for the widget and everything under it
            static void resolveBoxes(QWidget* root)
            {
                QList<QWidget*> widgets = root->findChildren<QWidget*>();
                widgets.prepend(root);
                for (QWidget* widget : widgets)
                {
                    BoxElement::apply(widget);
                }
            }

            //! The box element of a widget, by name ("" for none)
            static const char* boxNameOf(const QWidget* widget)
            {
                const BoxElement* element = BoxElement::forWidget(widget);
                return element ? element->m_name : "";
            }

            //! The margins the box puts around its contents in a rect of 200x100: the contents margins of the frame it boxes
            static QMargins boxAround(const QWidget* widget)
            {
                const BoxElement* element = BoxElement::forWidget(widget);
                if (!element)
                {
                    return QMargins(-9999, -9999, -9999, -9999);
                }
                QStyleOption option;
                option.rect = QRect(0, 0, 200, 100);
                const QRect contents = element->contentsRect(&option, widget);
                return QMargins(contents.left(), contents.top(), option.rect.right() - contents.right(), option.rect.bottom() - contents.bottom());
            }

            //! The widget has the element, and the box Native measured for it
            static void expectBox(const QWidget* widget, const char* name, const QMargins& native)
            {
                EXPECT_STREQ(boxNameOf(widget), name) << widget->objectName().toStdString();
                EXPECT_TRUE(boxAround(widget) == native) << widget->objectName().toStdString();
            }

            static void expectNoBox(const QWidget* widget)
            {
                EXPECT_STREQ(boxNameOf(widget), "") << widget->objectName().toStdString();
            }

        protected:
            AZStd::unique_ptr<QStyle> m_fusion;
        };

        // ------------------------------------------------------------------------------
        // The seed tab (n2 AssetBundler_00: AssetBundler::SeedTabWidget#SeedTabWidget)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, SeedTab_FramesLabelsAndBarsHaveTheBoxNativeMeasured)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget tab;
            tab.setObjectName(QStringLiteral("SeedTabWidget"));
            auto* heading = add<QLabel>(&tab, "activeProjectLabel");

            QFrame* tableFrame = addPanel(&tab, "fileTableFrame");
            QFrame* tableHeader = addPanel(tableFrame, "fileTableHeaderFrame");
            auto* tableTitle = add<QLabel>(tableHeader, "seedListsLabel");
            auto* tableBar = add<FilteredSearchWidget>(tableFrame, "fileFilteredSearchWidget");

            QFrame* contentsFrame = addPanel(&tab, "fileContentsFrame");
            QFrame* contentsHeader = addPanel(contentsFrame, "fileContentsHeaderFrame");
            auto* contentsTitle = add<QLabel>(contentsHeader, "seedListContentsLabel");
            auto* pathTitle = add<QLabel>(contentsHeader, "seedListFileAbsolutePathLabel");
            auto* contentsBar = add<FilteredSearchWidget>(contentsFrame, "seedListContentsFilteredSearchWidget");
            resolveBoxes(&tab);

            // Native contents: [0, 1, 0, 5] (the margin above is every label's)
            expectBox(heading, "AssetBundler.Box.ActiveProjectLabel", QMargins(0, 1, 0, 5));

            // [1, 1, 11, 1] with the margin of 10px at the right, and [1, 1, 1, 1]: a border of the frame's own
            expectBox(tableFrame, "AssetBundler.Box.FileTableFrame", QMargins(1, 1, 11, 1));
            expectBox(contentsFrame, "AssetBundler.Box.FileContentsFrame", QMargins(1, 1, 1, 1));
            EXPECT_TRUE(BoxElement::forWidget(tableFrame)->m_idle.m_fill.color() == QColor(0x44, 0x44, 0x44));
            EXPECT_TRUE(BoxElement::forWidget(tableFrame)->m_idle.m_edges.m_left.m_color == QColor(0x1B, 0x1B, 0x1B));

            // [1, 1, 1, 1] too, the 1px of the style's own frame, on #1B1B1B
            expectBox(tableHeader, "AssetBundler.Box.FileHeaderFrame", QMargins(1, 1, 1, 1));
            expectBox(contentsHeader, "AssetBundler.Box.FileHeaderFrame", QMargins(1, 1, 1, 1));
            EXPECT_TRUE(BoxElement::forWidget(tableHeader)->m_idle.m_fill.color() == QColor(0x1B, 0x1B, 0x1B));

            // The titles: [5, 1, 0, 1]
            expectBox(tableTitle, "AssetBundler.Box.FileHeaderLabel", QMargins(5, 1, 0, 1));
            expectBox(contentsTitle, "AssetBundler.Box.FileHeaderLabel", QMargins(5, 1, 0, 1));
            expectBox(pathTitle, "AssetBundler.Box.FileHeaderLabel", QMargins(5, 1, 0, 1));

            // The bars: [5, 0, 5, 0] and [5, 0, 0, 0]
            expectBox(tableBar, "AssetBundler.Box.FileFilteredSearchWidget", QMargins(5, 0, 5, 0));
            expectBox(contentsBar, "AssetBundler.Box.SeedListContentsFilteredSearchWidget", QMargins(5, 0, 0, 0));
        }

        TEST_F(FlatElementsTests, AssetListTab_ItsLabelAndBarAreNarrowerThanTheHeaders)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget tab;
            tab.setObjectName(QStringLiteral("AssetListTabWidget"));
            QFrame* contentsFrame = addPanel(&tab, "fileContentsFrame");
            QFrame* contentsHeader = addPanel(contentsFrame, "fileContentsHeaderFrame");
            auto* listTitle = add<QLabel>(contentsHeader, "assetListContentsLabel");
            auto* pathTitle = add<QLabel>(contentsHeader, "assetListFileAbsolutePathLabel");
            auto* bar = add<FilteredSearchWidget>(contentsFrame, "assetListContentsFilteredSearchWidget");
            auto* loose = add<QLabel>(&tab, "assetListContentsLabel");              // outside the header: its fill and left margin are not its own
            resolveBoxes(&tab);

            // The label with the margin of 10px at the right comes before the titles of a header: [5, 1, 10, 1] against [5, 1, 0, 1]
            expectBox(listTitle, "AssetBundler.Box.AssetListContentsLabel", QMargins(5, 1, 10, 1));
            expectBox(pathTitle, "AssetBundler.Box.FileHeaderLabel", QMargins(5, 1, 0, 1));
            expectBox(bar, "AssetBundler.Box.FileFilteredSearchWidget", QMargins(5, 0, 5, 0));
            expectNoBox(loose);
        }

        TEST_F(FlatElementsTests, Tabs_TheHeadingAndTheButtonsOfEveryTab)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget seeds;
            seeds.setObjectName(QStringLiteral("SeedTabWidget"));
            QWidget assetLists;
            assetLists.setObjectName(QStringLiteral("AssetListTabWidget"));
            QWidget rules;
            rules.setObjectName(QStringLiteral("RulesTabWidget"));
            QFrame* seedContents = addPanel(&seeds, "fileContentsFrame");
            QFrame* ruleContents = addPanel(&rules, "fileContentsFrame");

            auto* generateAssetLists = add<QPushButton>(&seeds, "generateAssetListsButton");
            auto* addSeed = add<QPushButton>(seedContents, "addSeedButton");
            auto* generateBundle = add<QPushButton>(&assetLists, "generateBundleButton");
            auto* runRule = add<QPushButton>(&rules, "runRuleButton");
            auto* addStep = add<QPushButton>(ruleContents, "addComparisonStepButton");
            auto* createSeedList = add<QPushButton>(&seeds, "createNewSeedListButton");     // spaced by no rule
            auto* browse = add<QPushButton>(&assetLists, "browseFileButton");               // named by the seed tab's rule only
            auto* checkBox = add<QCheckBox>(&seeds, "selectDefaultSeedListsCheckBox");
            resolveBoxes(&seeds);
            resolveBoxes(&assetLists);
            resolveBoxes(&rules);

            // Native max [-1, 24] for each: the element sets no limit, the style's own stays
            for (QPushButton* button : { generateAssetLists, addSeed, generateBundle, runRule, addStep })
            {
                EXPECT_STREQ(boxNameOf(button), "AssetBundler.Box.SpacedButton") << button->objectName().toStdString();
                EXPECT_EQ(button->maximumSize(), QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)) << button->objectName().toStdString();
                EXPECT_TRUE(BoxElement::forWidget(button)->m_margin == QMargins(10, 0, 10, 0));
            }
            expectNoBox(createSeedList);
            expectNoBox(browse);
            EXPECT_STREQ(boxNameOf(checkBox), "AssetBundler.Box.SelectDefaultSeedListsCheckBox");
            EXPECT_TRUE(BoxElement::forWidget(checkBox)->m_margin == QMargins(5, 5, 5, 5));
        }

        TEST_F(FlatElementsTests, Heading_IsAmazonEmberLightAt18pxInWhite)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget tab;
            auto* heading = add<QLabel>(&tab, "activeProjectLabel");
            auto* other = add<QLabel>(&tab, "seedListsLabel");
            TextElement::apply(heading);
            TextElement::applyColor(heading);
            TextElement::apply(other);

            // The family is the string of the sheet: Native drew Tahoma for it, as the machine has no such font
            EXPECT_TRUE(heading->font().family() == QStringLiteral("Amazon Ember Light"));
            EXPECT_EQ(heading->font().pixelSize(), 18);
            EXPECT_EQ(heading->font().weight(), QFont::Normal);
            EXPECT_TRUE(heading->palette().color(QPalette::Active, QPalette::WindowText) == QColor(0xFF, 0xFF, 0xFF));
            EXPECT_FALSE(other->font().family() == QStringLiteral("Amazon Ember Light"));
        }

        // ------------------------------------------------------------------------------
        // The tables, and the log under the tabs (n2 AssetBundler_00: QWidget#centralwidget)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, Tables_TheLogHasItsBorderAndMarginsAndEveryTableTheIndentationOfItsStyle)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget central;
            central.setObjectName(QStringLiteral("centralwidget"));
            auto* log = add<TableView>(&central, "logTableView");
            QFrame* tableFrame = addPanel(&central, "fileTableFrame");
            auto* fileTable = add<TableView>(tableFrame, "fileTableView");
            auto* contentsFrame = addPanel(&central, "fileContentsFrame");
            auto* seedTable = add<TableView>(contentsFrame, "seedFileContentsTable");
            auto* list = add<QScrollArea>(contentsFrame, "comparisonDataList");
            auto* plainTree = add<QTreeView>(&central, "someTree");
            log->setStyle(m_fusion.get());
            fileTable->setStyle(m_fusion.get());
            seedTable->setStyle(m_fusion.get());

            // The constructors of the tabs: 0, and 2 for the seed list table
            log->setIndentation(0);
            fileTable->setIndentation(2);
            seedTable->setIndentation(0);
            resolveBoxes(&central);

            // Native: [11, 1, 11, 11] for logTableView (the margin of 10px, its border of 1px), [0, 0, 0, 0] for the other tables
            expectBox(log, "AssetBundler.Box.LogTableView", QMargins(11, 1, 11, 11));
            expectBox(fileTable, "AssetBundler.Box.ItemView", QMargins(0, 0, 0, 0));
            expectBox(seedTable, "AssetBundler.Box.ItemView", QMargins(0, 0, 0, 0));
            expectBox(list, "AssetBundler.Box.ComparisonDataList", QMargins(0, 0, 0, 0));
            expectNoBox(plainTree);
            EXPECT_TRUE(log->testAttribute(Qt::WA_StyledBackground));

            // The rows start at the indentation of the style (20: n2 has x=20 for logTableView and the seed list table), not at the
            // 0 and 2 the constructors set; a tree that is no table of the sheet keeps what its code set
            const int styled = m_fusion->pixelMetric(QStyle::PM_TreeViewIndentation, nullptr, log);
            EXPECT_GT(styled, 2);
            EXPECT_EQ(log->indentation(), styled);
            EXPECT_EQ(fileTable->indentation(), styled);
            EXPECT_EQ(seedTable->indentation(), styled);
        }

        TEST_F(FlatElementsTests, Tables_TheSectionsOfTheHeadersAreTheColorsOfTheirRules)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            // n2: the log table (below the central widget, not in a tab) has the header #1B1B1B, every file table under a tab widget #4D4D4D; the
            // log table of the tabs of the bundler is the same widget, which sits under the main window (the rule of its id wins over the tabs')
            QWidget central;
            central.setObjectName(QStringLiteral("centralwidget"));
            auto* log = add<TableView>(&central, "logTableView");
            auto* tabs = add<QTabWidget>(&central, "tabWidget");
            auto* page = add<QWidget>(tabs, "SeedTabWidget");
            auto* fileTable = add<TableView>(page, "fileTableView");
            auto* seedTable = add<TableView>(page, "seedFileContentsTable");
            auto* tabLog = add<TableView>(page, "logTableView");                    // a log table that has a tab widget above it too
            auto* plainTree = add<QTreeView>(&central, "someTree");                 // a tree outside every tab: the Global header
            auto* tabTree = add<QTreeView>(page, "someTabTree");                    // `QTabWidget QTreeView`: any tree under a tab widget
            for (QTreeView* view : { static_cast<QTreeView*>(log), static_cast<QTreeView*>(fileTable), static_cast<QTreeView*>(seedTable),
                                     static_cast<QTreeView*>(tabLog), plainTree, tabTree })
            {
                ItemViewElement::apply(view);
            }

            const auto fill = [](const QTreeView* view) { return ItemViewElement::headerFillOf(view->header()); };
            EXPECT_TRUE(fill(log) == QColor(0x1B, 0x1B, 0x1B));
            EXPECT_TRUE(fill(tabLog) == QColor(0x1B, 0x1B, 0x1B));
            EXPECT_TRUE(fill(fileTable) == QColor(0x4D, 0x4D, 0x4D));
            EXPECT_TRUE(fill(seedTable) == QColor(0x4D, 0x4D, 0x4D));
            EXPECT_TRUE(fill(tabTree) == QColor(0x4D, 0x4D, 0x4D));
            EXPECT_TRUE(fill(plainTree) == QColor(0x2D, 0x2D, 0x2D));
            EXPECT_STREQ(ItemViewElement::forWidget(log)->m_name, "AssetBundler.ItemView.LogTableHeader");
            EXPECT_STREQ(ItemViewElement::forWidget(tabLog)->m_name, "AssetBundler.ItemView.LogTableHeader");
            EXPECT_STREQ(ItemViewElement::forWidget(fileTable)->m_name, "AssetBundler.ItemView.TabTableHeader");
            EXPECT_STREQ(ItemViewElement::forWidget(plainTree)->m_name, "Global.ItemView.Tree");

            // What the rules do not change is the Global element's (TableView.qss): the roles of a table, its items and its faces
            const ItemViewElement* element = ItemViewElement::forWidget(fileTable);
            EXPECT_TRUE(element->m_alternate == QColor(77, 77, 77));
            EXPECT_TRUE(element->m_selection == QColor(101, 101, 101));
            EXPECT_EQ(element->m_itemHeight, 24);
            EXPECT_TRUE(element->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
        }

        TEST_F(FlatElementsTests, Tables_ARuleRowsFrameHasItsPadding)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget row;
            auto* frame = add<QFrame>(&row, "comparisonDataFrame");
            resolveBoxes(&row);

            expectBox(frame, "AssetBundler.Box.ComparisonDataFrame", QMargins(10, 10, 10, 10));
        }

        // ------------------------------------------------------------------------------
        // The dialogs. Uncaptured: the names are the .ui files'
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, Dialogs_FramesLabelsCheckBoxesAndButtonsOfTheSeedDialogs)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget seeds;
            seeds.setObjectName(QStringLiteral("SeedTabWidget"));
            auto* dialog = add<QDialog>(&seeds, "AddSeedDialog");
            auto* inputs = add<QFrame>(dialog, "inputValuesFrame");
            auto* label = add<QLabel>(inputs, "pathLabel");
            auto* checkBox = add<QCheckBox>(inputs, "recursiveCheckBox");
            auto* line = add<QFrame>(dialog, "lineFrame");
            auto* browse = add<QPushButton>(inputs, "browseFileButton");
            auto* cancel = add<QPushButton>(dialog, "cancelButton");
            auto* strayFrame = add<QFrame>(&seeds, "inputValuesFrame");             // no dialog around it
            auto* strayLabel = add<QLabel>(&seeds, "pathLabel");
            resolveBoxes(&seeds);

            // [20, 20, 20, 20]: the margin of 10px and the padding of 10px; [0, 10, 0, 10]
            expectBox(inputs, "AssetBundler.Box.InputValuesFrame", QMargins(20, 20, 20, 20));
            expectBox(line, "AssetBundler.Box.LineFrame", QMargins(0, 10, 0, 10));
            EXPECT_TRUE(BoxElement::forWidget(inputs)->m_idle.m_fill.color() == QColor(0x55, 0x55, 0x55));
            EXPECT_STREQ(boxNameOf(label), "AssetBundler.Box.DialogLabel");
            EXPECT_STREQ(boxNameOf(checkBox), "AssetBundler.Box.DialogCheckBox");
            EXPECT_STREQ(boxNameOf(browse), "AssetBundler.Box.SpacedButton");
            EXPECT_STREQ(boxNameOf(cancel), "AssetBundler.Box.SpacedButton");
            expectNoBox(strayFrame);
            expectNoBox(strayLabel);
        }

        TEST_F(FlatElementsTests, Dialogs_TheBundleSettingsAndTheNewFileDialog)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget window;
            auto* bundles = add<QDialog>(&window, "GenerateBundlesModal");
            auto* settings = add<QFrame>(bundles, "bundleSettingsFrame");
            auto* settingsLabel = add<QLabel>(settings, "bundleLabel");
            auto* locationFrame = add<QFrame>(settings, "outputBundleLocationFrame");
            auto* fileFrame = add<QFrame>(settings, "bundleSettingsFileFrame");
            auto* bundlesButton = add<QPushButton>(bundles, "cancelButton");
            auto* newFile = add<QDialog>(&window, "NewFileDialog");
            auto* createButton = add<QPushButton>(newFile, "createFileButton");
            auto* otherButton = add<QPushButton>(newFile, "helpButton");
            auto* newFileLabel = add<QLabel>(newFile, "nameLabel");
            resolveBoxes(&window);

            expectBox(settings, "AssetBundler.Box.BundleSettingsFrame", QMargins(0, 0, 0, 10));
            expectBox(locationFrame, "AssetBundler.Box.BundleSettingsFileFrame", QMargins(10, 10, 10, 10));
            expectBox(fileFrame, "AssetBundler.Box.BundleSettingsFileFrame", QMargins(10, 10, 10, 10));
            EXPECT_STREQ(boxNameOf(settingsLabel), "AssetBundler.Box.DialogLabel");
            EXPECT_STREQ(boxNameOf(bundlesButton), "AssetBundler.Box.SpacedButton");
            EXPECT_STREQ(boxNameOf(createButton), "AssetBundler.Box.SpacedButton");
            EXPECT_STREQ(boxNameOf(newFileLabel), "AssetBundler.Box.DialogLabel");
            expectNoBox(otherButton);
        }
    } // namespace FlatElementsTestsInternal
} // namespace AssetBundler
