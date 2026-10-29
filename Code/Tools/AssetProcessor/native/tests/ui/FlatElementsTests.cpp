/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <native/ui/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFrame>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleOption>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

#include <cstring>

// ----------------------------------------------------------------------------------
// The elements of AssetProcessor.qss (native/ui/FlatElements.cpp) on widgets built as the captured ones are: the names, classes,
// class tags and properties of the n2 dumps (UiCaptures/n2/AssetProcessor/qss), and for each the box Native measured for it (the
// `contents` of its dump). The registrar is called directly and BoxElement::apply, TextElement::apply are called as Style::polish
// calls them, so nothing here depends on the flattened mode of the process.
// ----------------------------------------------------------------------------------

namespace AssetProcessor
{
    namespace FlatElementsTestsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::FilteredSearchWidget;
        using AzQtComponents::FrameElement;
        using AzQtComponents::ItemViewElement;
        using AzQtComponents::TableView;
        using AzQtComponents::TextElement;

        // ------------------------------------------------------------------------------
        // Fixture: an application, and the registrar run once for the process
        // ------------------------------------------------------------------------------

        class FlatElementsTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            FlatElementsTests()
            {
                // Before the application: its construction is where Qt warns
                s_previousHandler = qInstallMessageHandler(&FlatElementsTests::messageHandler);
                if (QCoreApplication::instance() == nullptr)
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                }

                // The registries last for the process: a second call would only add the same rows again
                static bool registered = false;
                if (!registered)
                {
                    registered = true;
                    RegisterFlatElements();
                }
            }

            ~FlatElementsTests() override
            {
                m_application.reset();
                qInstallMessageHandler(s_previousHandler);
            }

            //! The test environment fails a test on any Qt warning (BaseAssetProcessorTest.h:20-36). Every test here makes an
            //! application of its own, and from the second one on Qt's Windows platform warns that the window class of its theme
            //! observer exists already (it registers it once per application): Qt's own, nothing the elements did. That one
            //! warning is let through; every other message goes to the environment's handler.
            static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
            {
                if (type == QtWarningMsg && message.contains(QLatin1String("ThemeChangeObserverWindow")))
                {
                    return;
                }
                if (s_previousHandler)
                {
                    s_previousHandler(type, context, message);
                }
            }

            static inline QtMessageHandler s_previousHandler = nullptr;

            //! A widget of the class under a parent, named as the captured one is
            template<typename W>
            static W* add(QWidget* parent, const char* name)
            {
                W* widget = new W(parent);
                widget->setObjectName(QString::fromLatin1(name));
                return widget;
            }

            static void tag(QWidget* widget, const char* classTag)
            {
                widget->setProperty("class", QString::fromLatin1(classTag));
            }

            //! What Style::polish does for the frame and the box of a widget, for the widget and everything under it
            static void resolveBoxes(QWidget* root)
            {
                QList<QWidget*> widgets = root->findChildren<QWidget*>();
                widgets.prepend(root);
                for (QWidget* widget : widgets)
                {
                    FrameElement::resolve(widget);
                    BoxElement::resolve(widget);
                }
            }

            //! What Style::polish does for the item views of a widget and everything under it: the element of the view, its roles, its headers
            static void resolveViews(QWidget* root)
            {
                const QList<QAbstractItemView*> views = root->findChildren<QAbstractItemView*>();
                for (QAbstractItemView* view : views)
                {
                    ItemViewElement::resolve(view);
                }
            }

            //! What Style::polish does for the text of a label: the element over the font, then its colour
            static void resolveText(QLabel* label)
            {
                TextElement::resolve(label);
                TextElement::applyColor(label);
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

            //! The name of the frame element a widget has, "" for none
            static const char* frameNameOf(QWidget* widget)
            {
                const FrameElement* element = FrameElement::forWidget(widget);
                return element ? element->m_name : "";
            }

            //! The box of a widget painted by its element onto an image of one colour, at device ratio 1: a logical pixel is a
            //! device pixel, whatever the screen of the machine is
            static QImage paintBox(const QWidget* widget, const QSize& size, QRgb background)
            {
                QImage image(size, QImage::Format_RGB32);
                image.fill(QColor::fromRgb(background));
                QPainter painter(&image);
                QStyleOption option;
                option.rect = QRect(QPoint(0, 0), size);
                option.state = QStyle::State_Enabled;
                const BoxElement* element = BoxElement::forWidget(widget);
                EXPECT_TRUE(element != nullptr && element->draw(&painter, &option, widget)) << widget->objectName().toStdString();
                return image;
            }

            static QRgb rgbAt(const QImage& image, int x, int y)
            {
                return image.pixel(x, y) & 0xFFFFFF;
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
        };

        // ------------------------------------------------------------------------------
        // The status row (n2 AssetProcessor_00: MainWindow#MainWindow > QWidget#centralWidget)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, StatusRow_LabelsTimersLinesAndSupportButtonHaveTheBoxNativeMeasured)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget central;
            central.setObjectName(QStringLiteral("centralWidget"));
            auto* status = add<QLabel>(&central, "APStatusValueLabel");
            tag(status, "Title");
            auto* project = add<QLabel>(&central, "projectLabel");
            tag(project, "secondaryText");
            auto* root = add<QLabel>(&central, "rootLabel");
            tag(root, "secondaryText");
            auto* port = add<QLabel>(&central, "portLabel");
            tag(port, "secondaryText");
            auto* support = add<QPushButton>(&central, "supportButton");
            support->setFlat(true);
            auto* missingSupport = add<QPushButton>(&central, "MissingProductDependenciesSupport");      // uncaptured: the product tab's
            missingSupport->setFlat(true);

            // The three groups of the timer row: a title, a timer and a line
            auto* timers = add<QWidget>(&central, "timerContainerWidget");
            QLabel* titles[3] = { add<QLabel>(timers, "lastScanLabel"), add<QLabel>(timers, "analysisLabel"), add<QLabel>(timers, "processingLabel") };
            QLabel* clocks[3] = { add<QLabel>(timers, "lastScanTimer"), add<QLabel>(timers, "analysisTimer"), add<QLabel>(timers, "processingTimer") };
            QFrame* lines[3] = { add<QFrame>(timers, "lastScanSeparator"), add<QFrame>(timers, "analysisSeparator"), add<QFrame>(timers, "processingSeparator") };
            for (int group = 0; group < 3; ++group)
            {
                tag(titles[group], "Title");
                tag(clocks[group], "APTimer");
                tag(lines[group], "TimerLine");
                lines[group]->setFrameShape(QFrame::VLine);
            }
            resolveBoxes(&central);
            for (QLabel* clock : clocks)
            {
                resolveText(clock);
            }

            // Native contents: [8, 12, 0, 7] (a Title's 7px under it), [8, 0, 0, 0] twice, [8, 0, 0, 24]
            expectBox(status, "AssetProcessor.Box.APStatusValueLabel", QMargins(8, 12, 0, 7));
            expectBox(project, "AssetProcessor.Box.ProjectLabel", QMargins(8, 0, 0, 0));
            expectBox(root, "AssetProcessor.Box.ProjectLabel", QMargins(8, 0, 0, 0));
            expectBox(port, "AssetProcessor.Box.PortLabel", QMargins(8, 0, 0, 24));

            for (int group = 0; group < 3; ++group)
            {
                // [0, 0, 0, 0]: the titles and the timers have no margin of their own, and the lines no box
                expectBox(titles[group], "AssetProcessor.Box.NoMargin", QMargins(0, 0, 0, 0));
                expectBox(clocks[group], "AssetProcessor.Box.NoMargin", QMargins(0, 0, 0, 0));
                expectNoBox(lines[group]);
                EXPECT_STREQ(frameNameOf(lines[group]), "AssetProcessor.Frame.TimerLine");

                // The timers: Open Sans Light 18px, white in every state
                EXPECT_TRUE(clocks[group]->font().family() == QStringLiteral("Open Sans Light"));
                EXPECT_EQ(clocks[group]->font().pixelSize(), 18);
                EXPECT_EQ(clocks[group]->font().weight(), QFont::Normal);
                EXPECT_TRUE(clocks[group]->palette().color(QPalette::Active, QPalette::WindowText) == QColor(0xFF, 0xFF, 0xFF));
                EXPECT_TRUE(clocks[group]->palette().color(QPalette::Disabled, QPalette::WindowText) == QColor(0xFF, 0xFF, 0xFF));
            }

            // The help buttons: n2 min [0, 34] and max [-1, 34] (their 24px and the 10px above), the icon at 24px
            for (QPushButton* button : { support, missingSupport })
            {
                EXPECT_STREQ(boxNameOf(button), "AssetProcessor.Box.SupportButton") << button->objectName().toStdString();
                EXPECT_EQ(button->minimumSize(), QSize(0, 34)) << button->objectName().toStdString();
                EXPECT_EQ(button->maximumSize(), QSize(QWIDGETSIZE_MAX, 34)) << button->objectName().toStdString();
                EXPECT_EQ(button->iconSize(), QSize(24, 24)) << button->objectName().toStdString();
            }
        }

        TEST_F(FlatElementsTests, StatusRow_AWidgetThatOnlyLooksLikeOneDoesNotMatch)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget central;
            auto* untagged = add<QLabel>(&central, "APStatusValueLabel");       // the 7px under it are the Title's: no tag, no element
            auto* plain = add<QLabel>(&central, "somethingElse");
            auto* aTimer = add<QLabel>(&central, "aTimer");
            tag(aTimer, "APTimer");                                             // the margin and the font are the tag's
            auto* label = add<QLabel>(&central, "supportButton");               // a label is no button
            auto* button = add<QPushButton>(&central, "okButton");
            auto* tagged = add<QFrame>(&central, "aLine");
            tag(tagged, "TimerLine");
            auto* untaggedFrame = add<QFrame>(&central, "aFrame");
            resolveBoxes(&central);
            resolveText(aTimer);
            resolveText(plain);

            expectNoBox(untagged);
            expectNoBox(plain);
            expectNoBox(label);
            expectNoBox(button);
            EXPECT_STREQ(boxNameOf(aTimer), "AssetProcessor.Box.NoMargin");
            EXPECT_TRUE(aTimer->font().family() == QStringLiteral("Open Sans Light"));
            EXPECT_FALSE(plain->font().family() == QStringLiteral("Open Sans Light"));
            EXPECT_STREQ(frameNameOf(tagged), "AssetProcessor.Frame.TimerLine");
            EXPECT_TRUE(std::strncmp(frameNameOf(untaggedFrame), "AssetProcessor.", 15) != 0);
        }

        // ------------------------------------------------------------------------------
        // The shared cache page (n2 AssetProcessor_00 SharedCachePage7)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, SharedCache_TheStatusLabelFollowsItsHighlight)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget page;
            auto* status = add<QLabel>(&page, "sharedCacheStatus");
            status->setFrameShape(QFrame::StyledPanel);

            // Native, blue: contents [1, 2, 1, 2] (the 1px border, and the margin of 1px above and below it) and a 10% tint of #588CBC
            status->setProperty("highlight", QStringLiteral("blue"));
            BoxElement::resolve(status);
            expectBox(status, "AssetProcessor.Box.SharedCacheStatus.Notice", QMargins(1, 2, 1, 2));
            EXPECT_TRUE(BoxElement::forWidget(status)->m_idle.m_fill.color() == QColor(88, 140, 188, 25));

            // The label is repolished when the property changes (MainWindow.cpp:949-989): the element follows
            status->setProperty("highlight", QStringLiteral("green"));
            BoxElement::resolve(status);
            expectBox(status, "AssetProcessor.Box.SharedCacheStatus.Active", QMargins(1, 2, 1, 2));
            EXPECT_TRUE(BoxElement::forWidget(status)->m_idle.m_fill.color() == QColor(88, 188, 97, 25));

            status->setProperty("highlight", QStringLiteral("red"));
            BoxElement::resolve(status);
            expectBox(status, "AssetProcessor.Box.SharedCacheStatus.Error", QMargins(1, 2, 1, 2));
            EXPECT_TRUE(BoxElement::forWidget(status)->m_idle.m_fill.color() == QColor(255, 0, 0, 25));

            // No highlight, or one the sheet does not know, is a plain label
            status->setProperty("highlight", QStringLiteral("yellow"));
            BoxElement::resolve(status);
            expectNoBox(status);
            status->setProperty("highlight", QVariant());
            BoxElement::resolve(status);
            expectNoBox(status);

            // Another label with the property
            auto* other = add<QLabel>(&page, "somethingElse");
            other->setProperty("highlight", QStringLiteral("blue"));
            BoxElement::resolve(other);
            expectNoBox(other);
        }

        TEST_F(FlatElementsTests, SharedCache_PagesTableAndToolButton)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget dialogStack;
            auto* shared = add<QWidget>(&dialogStack, "SharedCachePage");
            auto* relocation = add<QWidget>(&dialogStack, "AssetRelocationPage");
            auto* other = add<QWidget>(&dialogStack, "SettingsPage");
            auto* table = add<QTableWidget>(shared, "sharedCacheTable");
            auto* browse = add<QToolButton>(shared, "serverAddressToolButton");
            auto* otherButton = add<QToolButton>(shared, "toolButton");
            ASSERT_TRUE(table->showGrid());
            resolveBoxes(&dialogStack);

            // The pages: #222222 at half, no border of the style's (n2: the labels on them measure #333333 over #444444)
            EXPECT_STREQ(boxNameOf(shared), "AssetProcessor.Box.PageOverlay");
            EXPECT_STREQ(boxNameOf(relocation), "AssetProcessor.Box.PageOverlay");
            expectNoBox(other);
            EXPECT_TRUE(BoxElement::forWidget(shared)->m_idle.m_fill.color() == QColor(34, 34, 34, 127));
            QStyleOption option;
            option.state = QStyle::State_Enabled;
            EXPECT_TRUE(BoxElement::forWidget(shared)->ownsBox(&option));
            EXPECT_FALSE(BoxElement::forWidget(shared)->hasNativeBorder(&option));

            // The table: n2 contents [0, 0, 0, 0], the fill #2D2D2D, and QTableView { qproperty-showGrid: false }
            expectBox(table, "AssetProcessor.Box.SharedCacheTable", QMargins(0, 0, 0, 0));
            EXPECT_TRUE(BoxElement::forWidget(table)->m_idle.m_fill.color() == QColor(45, 45, 45));
            EXPECT_FALSE(table->showGrid());

            // The tool button: no border, no padding, no margin, and the image of its border-image (AssetProcessor.qss:199), stretched over it
            expectBox(browse, "AssetProcessor.Box.ServerAddressToolButton", QMargins(0, 0, 0, 0));
            EXPECT_STREQ(BoxElement::forWidget(browse)->m_idle.m_borderImage, ":/Browse_on.png");
            expectNoBox(otherButton);
        }

        TEST_F(FlatElementsTests, SharedCache_TheTranslucentBoxesPaintTheColoursTheCaptureMeasured)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            // n2 SharedCachePage7, in device pixels: the page is #333333 (the overlay's rgba(34, 34, 34, 0.5) over the dialog's
            // #444444), the border of the status label #4285F4 and what is inside it #373C40 (rgba(88, 140, 188, 0.1) over the page)
            QWidget stack;
            auto* status = add<QLabel>(&stack, "sharedCacheStatus");
            status->setFrameShape(QFrame::StyledPanel);
            status->setProperty("highlight", QStringLiteral("blue"));
            auto* overlay = add<QWidget>(&stack, "SharedCachePage");
            BoxElement::resolve(status);
            BoxElement::resolve(overlay);

            const QImage label = paintBox(status, QSize(60, 20), 0x333333);
            EXPECT_EQ(rgbAt(label, 30, 0), 0x333333u);          // the margin of 1px above the label shows the page
            EXPECT_EQ(rgbAt(label, 30, 1), 0x4285F4u);          // its border
            EXPECT_EQ(rgbAt(label, 0, 10), 0x4285F4u);
            EXPECT_EQ(rgbAt(label, 30, 10), 0x373C40u);         // its tint

            const QImage page = paintBox(overlay, QSize(20, 20), 0x444444);
            EXPECT_EQ(rgbAt(page, 10, 10), 0x333333u);
        }

        // ------------------------------------------------------------------------------
        // The jobs, assets and logs pages (n2 jobDialogSplitter1 and LogDialog3)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, JobsPage_SplitterLabelsStackAndWrapper)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget dialogStack;
            auto* splitter = add<QSplitter>(&dialogStack, "jobDialogSplitter");
            auto* logContainer = add<QWidget>(splitter, "jobLogContainer");
            auto* logLabel = add<QLabel>(logContainer, "jobLogLabel");
            auto* stack = add<QStackedWidget>(logContainer, "jobLogStackedWidget");
            auto* contextLabel = add<QLabel>(&dialogStack, "jobContextLogLabel");
            auto* placeholder = add<QLabel>(&dialogStack, "jobLogPlaceholderLabel");
            auto* wrapper = add<QFrame>(&dialogStack, "logButtonWrapper");
            auto* statusLabel = add<QLabel>(&dialogStack, "assetStatusLabel");
            ASSERT_TRUE(splitter->childrenCollapsible());
            resolveBoxes(&dialogStack);

            // The splitter keeps its children (qproperty-childrenCollapsible: False) and has no margin
            expectBox(splitter, "AssetProcessor.Box.JobDialogSplitter", QMargins(0, 0, 0, 0));
            EXPECT_FALSE(splitter->childrenCollapsible());

            // Native: [0, 12, 0, 1] for jobLogLabel, [0, 0, 0, 0] for the stack, [0, 8, 0, 0] for the wrapper
            expectBox(logLabel, "AssetProcessor.Box.JobLogLabel", QMargins(0, 12, 0, 1));
            expectBox(stack, "AssetProcessor.Box.NoMargin", QMargins(0, 0, 0, 0));
            expectBox(wrapper, "AssetProcessor.Box.LogButtonWrapper", QMargins(0, 8, 0, 0));

            // Uncaptured: the label of the context log has jobLogLabel's rule, the placeholder a padding of 8px and 16px above it,
            // inside the margins of a label
            expectBox(contextLabel, "AssetProcessor.Box.JobLogLabel", QMargins(0, 12, 0, 1));
            expectBox(placeholder, "AssetProcessor.Box.JobLogPlaceholderLabel", QMargins(8, 17, 8, 9));

            // A label of the page the sheet says nothing of
            expectNoBox(statusLabel);
        }

        TEST_F(FlatElementsTests, ItemViews_TheViewsOfTheSheetsShareOneBoxAndTheOthersHaveNone)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget page;
            auto* jobTree = add<TableView>(&page, "jobTreeView");              // [0, 0, 0, 0] in n2, whatever its name
            auto* builders = add<QListView>(&page, "builderList");
            auto* results = add<QListWidget>(&page, "missingDependencyScanResults");
            auto* sourceTree = add<QTreeView>(&page, "SourceAssetsTreeView");
            auto* productTree = add<QTreeView>(&page, "OutgoingProductDependenciesTreeView");   // uncaptured, as the other three
            auto* plainTree = add<QTreeView>(&page, "someTree");
            auto* plainList = add<QListView>(&page, "someList");
            resolveBoxes(&page);

            for (QWidget* view : { static_cast<QWidget*>(jobTree), static_cast<QWidget*>(builders), static_cast<QWidget*>(results),
                                   static_cast<QWidget*>(sourceTree), static_cast<QWidget*>(productTree) })
            {
                expectBox(view, "AssetProcessor.Box.ItemView", QMargins(0, 0, 0, 0));
                EXPECT_TRUE(BoxElement::forWidget(view)->m_idle.m_fill.color() == QColor(45, 45, 45));
                EXPECT_TRUE(view->testAttribute(Qt::WA_StyledBackground));
            }
            expectNoBox(plainTree);
            expectNoBox(plainList);
        }

        // ------------------------------------------------------------------------------
        // The filter bars (n2 jobDialogSplitter1 and assetDetailsDialogSplitter2)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, FilterBars_BarContainerFieldAndPopupHaveTheSheetsBoxAndProperties)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            FilteredSearchWidget details;
            details.setObjectName(QStringLiteral("detailsFilterWidget"));
            FilteredSearchWidget job;
            job.setObjectName(QStringLiteral("jobFilteredSearchWidget"));
            FilteredSearchWidget assetData;
            assetData.setObjectName(QStringLiteral("assetDataFilteredSearchWidget"));
            FilteredSearchWidget foreign;                                       // a bar of AssetBundler.qss is not this sheet's
            foreign.setObjectName(QStringLiteral("fileFilteredSearchWidget"));
            ASSERT_TRUE(assetData.textFilterFillsWidth());                      // the widget's own default

            for (FilteredSearchWidget* bar : { &details, &job, &assetData, &foreign })
            {
                // The search field of a bar has the search action, which the flattened GUI's polish of the bar adds (n2: HasSearchAction)
                QLineEdit* field = bar->findChild<QLineEdit*>(QStringLiteral("textSearch"));
                ASSERT_TRUE(field != nullptr);
                field->setProperty("HasSearchAction", true);
                resolveBoxes(bar);
            }

            // Native: [8, 8, 8, 8] and the text filter beside the type filter, except on the assets page
            expectBox(&details, "AssetProcessor.Box.DetailsFilterWidget", QMargins(8, 8, 8, 8));
            expectBox(&job, "AssetProcessor.Box.DetailsFilterWidget", QMargins(8, 8, 8, 8));
            expectBox(&assetData, "AssetProcessor.Box.AssetDataFilteredSearchWidget", QMargins(8, 8, 8, 8));
            expectNoBox(&foreign);
            EXPECT_FALSE(details.textFilterFillsWidth());
            EXPECT_FALSE(job.textFilterFillsWidth());
            EXPECT_TRUE(assetData.textFilterFillsWidth());

            // The container ([0, 0, 0, 0], #282828) and the field of every bar, which has the frame the sheet gives it
            // (`FilteredSearchWidget #textSearchContainer` and `FilteredSearchWidget QLineEdit` name no bar)
            for (FilteredSearchWidget* bar : { &details, &job, &assetData, &foreign })
            {
                QFrame* container = bar->findChild<QFrame*>(QStringLiteral("textSearchContainer"));
                ASSERT_TRUE(container != nullptr);
                expectBox(container, "AssetProcessor.Box.TextSearchContainer", QMargins(0, 0, 0, 0));
                EXPECT_TRUE(BoxElement::forWidget(container)->m_idle.m_fill.color() == QColor(40, 40, 40));
                QLineEdit* field = bar->findChild<QLineEdit*>(QStringLiteral("textSearch"));
                EXPECT_STREQ(boxNameOf(field), "AssetProcessor.Box.TextSearch");
                EXPECT_TRUE(field->hasFrame());
            }

            // The field: #E9E9E9 in every state, 260x16 of contents. The n2 dump of a search field has Base, Button and Window
            // #ffe9e9e9 in its Active and in its Disabled colour group: the fill of AssetsTab.qss:38 outranks the #666666 that
            // `QLineEdit:disabled` (LineEdit.qss:33, the application sheet) would give a disabled field
            const BoxElement* search = BoxElement::forWidget(details.findChild<QLineEdit*>(QStringLiteral("textSearch")));
            ASSERT_TRUE(search != nullptr);
            QStyleOption enabled;
            enabled.state = QStyle::State_Enabled;
            QStyleOption disabled;
            disabled.state = QStyle::State_None;
            EXPECT_TRUE(search->face(&enabled).m_fill.color() == QColor(0xE9, 0xE9, 0xE9));
            EXPECT_TRUE(search->face(&disabled).m_fill.color() == QColor(0xE9, 0xE9, 0xE9));
            EXPECT_TRUE(search->face(&disabled).m_edges.m_left.m_color == QColor(0xE9, 0xE9, 0xE9));
            EXPECT_EQ(search->m_size, QSize(260, 16));

            // A field without the search action is not the sheet's search field
            QLineEdit plain;
            plain.setObjectName(QStringLiteral("textSearch"));
            BoxElement::resolve(&plain);
            expectNoBox(&plain);

            // The container outside a bar is no container of one
            QFrame loose;
            loose.setObjectName(QStringLiteral("textSearchContainer"));
            BoxElement::resolve(&loose);
            expectNoBox(&loose);

            // The tree of the type popup of a bar under the two ids (uncaptured), none under the assets page's bar
            QTreeView* detailsTree = details.findChild<QTreeView*>();
            QTreeView* jobTree = job.findChild<QTreeView*>();
            QTreeView* assetTree = assetData.findChild<QTreeView*>();
            ASSERT_TRUE(detailsTree != nullptr && jobTree != nullptr && assetTree != nullptr);
            EXPECT_STREQ(boxNameOf(detailsTree), "AssetProcessor.Box.FilterPopupTree");
            EXPECT_STREQ(boxNameOf(jobTree), "AssetProcessor.Box.FilterPopupTree");
            expectNoBox(assetTree);
            EXPECT_EQ(detailsTree->indentation(), 0);
            EXPECT_FALSE(detailsTree->expandsOnDoubleClick());

            // The popup itself has no text filter of its own under the same two ids
            auto* detailsPopup = details.findChild<AzQtComponents::SearchTypeSelector*>();
            auto* assetPopup = assetData.findChild<AzQtComponents::SearchTypeSelector*>();
            ASSERT_TRUE(detailsPopup != nullptr && assetPopup != nullptr);
            EXPECT_STREQ(boxNameOf(detailsPopup), "AssetProcessor.Box.FilterPopup");
            EXPECT_FALSE(detailsPopup->lineEditSearchVisible());
            expectNoBox(assetPopup);
            EXPECT_TRUE(assetPopup->lineEditSearchVisible());
        }

        TEST_F(FlatElementsTests, FilterBars_TheChipRowsAreUncapturedAndHaveTheirMargins)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            FilteredSearchWidget bar;
            bar.setObjectName(QStringLiteral("jobFilteredSearchWidget"));
            resolveBoxes(&bar);

            QFrame* parent = bar.findChild<QFrame*>(QStringLiteral("filteredParent"));
            QFrame* layout = bar.findChild<QFrame*>(QStringLiteral("filteredLayout"));
            ASSERT_TRUE(parent != nullptr && layout != nullptr);

            // 6px above the row; a margin of -4px around the chips' frame, which negates the flow layout's
            expectBox(parent, "AssetProcessor.Box.FilteredParent", QMargins(0, 6, 0, 0));
            expectBox(layout, "AssetProcessor.Box.FilteredLayout", QMargins(-4, -4, -4, -4));
        }

        // ------------------------------------------------------------------------------
        // The colors of the views (AssetProcessor.qss:50-167, AssetsTab.qss:67-105; n2: the headers of every table are #222222)
        // ------------------------------------------------------------------------------

        TEST_F(FlatElementsTests, ItemViews_TheTablesListsAndTreesHaveTheColorsOfTheirRules)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QWidget page;
            auto* jobTree = add<TableView>(&page, "jobTreeView");
            auto* logTable = add<TableView>(&page, "jobLogTableView");
            auto* shared = add<QTableWidget>(&page, "sharedCacheTable");
            auto* productTable = add<QTableWidget>(&page, "productTable");
            auto* incomingTable = add<QTableWidget>(&page, "incomingSourceDependenciesTable");
            auto* unmetList = add<QListWidget>(&page, "outgoingUnmetPathProductDependenciesList");
            auto* otherList = add<QListWidget>(&page, "someListWidget");
            auto* builders = add<QListView>(&page, "builderList");
            auto* sourceTree = add<QTreeView>(&page, "SourceAssetsTreeView");
            auto* incomingTree = add<QTreeView>(&page, "IncomingProductDependenciesTreeView");
            auto* plainTree = add<QTreeView>(&page, "someTree");
            auto* plainList = add<QListView>(&page, "someList");
            auto* relocation = add<QListView>(&page, "AssetRelocationExtensionListView");
            auto* plainTable = add<QTableView>(&page, "someTable");
            for (QTableWidget* table : { shared, productTable, incomingTable })
            {
                table->setColumnCount(2);
            }
            resolveViews(&page);

            const QColor dark(34, 34, 34);          // rgb(34,34,34)
            const QColor selected(73, 73, 73);      // rgb(73,73,73)
            const QColor hover(60, 60, 60);         // rgb(60,60,60)
            const QColor inactiveText(192, 192, 192);
            const auto name = [](const QWidget* view) { return QByteArray(ItemViewElement::forWidget(view)->m_name); };

            // The TableViews (AssetsTab.qss:86-105) and the QTableWidgets (AssetProcessor.qss:133-149): the same colors
            for (QAbstractItemView* view : { static_cast<QAbstractItemView*>(jobTree), static_cast<QAbstractItemView*>(logTable),
                                              static_cast<QAbstractItemView*>(shared) })
            {
                const ItemViewElement* element = ItemViewElement::forWidget(view);
                ASSERT_TRUE(element != nullptr) << view->objectName().toStdString();
                EXPECT_TRUE(element->m_alternate == dark) << view->objectName().toStdString();
                EXPECT_TRUE(element->m_selection == selected) << view->objectName().toStdString();
                EXPECT_TRUE(element->m_headerFill == dark) << view->objectName().toStdString();
                EXPECT_TRUE(element->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == selected) << view->objectName().toStdString();
                EXPECT_TRUE(element->fillOf(ItemViewElement::Hover, ItemViewElement::OnItem) == hover) << view->objectName().toStdString();
                // an inactive window changes nothing for these: the selected item is rgb(73,73,73) while inactive too
                EXPECT_TRUE(element->fillOf(ItemViewElement::Selected | ItemViewElement::Inactive, ItemViewElement::OnItem) == selected)
                    << view->objectName().toStdString();
            }
            EXPECT_EQ(name(jobTree), QByteArray("AssetProcessor.ItemView.TableView"));
            EXPECT_EQ(name(shared), QByteArray("AssetProcessor.ItemView.TableWidget"));
            EXPECT_TRUE(ItemViewElement::headerFillOf(jobTree->header()) == dark);
            EXPECT_TRUE(ItemViewElement::headerFillOf(shared->horizontalHeader()) == dark);

            // A TableView and its branches: the selected and hovered fills reach the branch cell of the row too (`::branch:selected`, `::branch:hover`)
            const ItemViewElement* tableView = ItemViewElement::forWidget(jobTree);
            EXPECT_TRUE(tableView->fillOf(ItemViewElement::Selected, ItemViewElement::OnBranch) == selected);
            EXPECT_TRUE(tableView->fillOf(ItemViewElement::Hover, ItemViewElement::OnBranch) == hover);
            // QTableWidget::item has no branch rule: a table has no branches, and the element's faces have none of its own either
            EXPECT_EQ(name(productTable), QByteArray("AssetProcessor.ItemView.DetailTable"));
            EXPECT_EQ(name(incomingTable), QByteArray("AssetProcessor.ItemView.DetailTable"));

            // The detail panels' tables and list, named by the sheet: a selected item of an inactive window is rgb(60,60,60), its text rgb(192,192,192)
            for (QAbstractItemView* view : { static_cast<QAbstractItemView*>(productTable), static_cast<QAbstractItemView*>(incomingTable),
                                              static_cast<QAbstractItemView*>(unmetList) })
            {
                const ItemViewElement* element = ItemViewElement::forWidget(view);
                ASSERT_TRUE(element != nullptr) << view->objectName().toStdString();
                const unsigned inactiveSelected = ItemViewElement::Selected | ItemViewElement::Inactive;
                EXPECT_TRUE(element->fillOf(inactiveSelected, ItemViewElement::OnItem) == hover) << view->objectName().toStdString();
                EXPECT_TRUE(element->selectedTextOf(inactiveSelected) == inactiveText) << view->objectName().toStdString();
                EXPECT_TRUE(element->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == selected) << view->objectName().toStdString();
                EXPECT_TRUE(element->m_alternate == dark) << view->objectName().toStdString();
                EXPECT_TRUE(element->m_selection == selected) << view->objectName().toStdString();
            }
            EXPECT_EQ(name(unmetList), QByteArray("AssetProcessor.ItemView.DetailList"));

            // Any other QListWidget has the colors of QListWidget (AssetProcessor.qss:151-167), and no inactive look of its own
            EXPECT_EQ(name(otherList), QByteArray("AssetProcessor.ItemView.ListWidget"));
            const ItemViewElement* listWidget = ItemViewElement::forWidget(otherList);
            EXPECT_TRUE(listWidget->m_alternate == dark);
            EXPECT_TRUE(listWidget->m_selection == selected);
            EXPECT_TRUE(listWidget->fillOf(ItemViewElement::Selected | ItemViewElement::Inactive, ItemViewElement::OnItem) == selected);

            // The builder list has the roles of the table and the items of the Global element (no `#builderList::item` rule): rgb(101,101,101)
            EXPECT_EQ(name(builders), QByteArray("AssetProcessor.ItemView.BuilderList"));
            const ItemViewElement* builderList = ItemViewElement::forWidget(builders);
            EXPECT_TRUE(builderList->m_alternate == dark);
            EXPECT_TRUE(builderList->m_selection == selected);
            EXPECT_TRUE(builderList->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
            EXPECT_TRUE(builderList->fillOf(ItemViewElement::Hover, ItemViewElement::OnItem) == QColor(90, 90, 90));

            // The five asset trees (AssetProcessor.qss:50-112): rgb(45,45,45) by itself, hover, selected, and the item of an inactive window
            for (QTreeView* tree : { sourceTree, incomingTree })
            {
                const ItemViewElement* element = ItemViewElement::forWidget(tree);
                ASSERT_TRUE(element != nullptr) << tree->objectName().toStdString();
                EXPECT_EQ(name(tree), QByteArray("AssetProcessor.ItemView.AssetTree")) << tree->objectName().toStdString();
                for (const auto scope : { ItemViewElement::OnItem, ItemViewElement::OnBranch })
                {
                    EXPECT_TRUE(element->fillOf(0, scope) == QColor(45, 45, 45)) << tree->objectName().toStdString();
                    EXPECT_TRUE(element->fillOf(ItemViewElement::Hover, scope) == hover) << tree->objectName().toStdString();
                    EXPECT_TRUE(element->fillOf(ItemViewElement::Selected | ItemViewElement::Active, scope) == selected) << tree->objectName().toStdString();
                    // a selected item and its hover: the rule of :selected follows the one of :hover in the sheet
                    EXPECT_TRUE(element->fillOf(ItemViewElement::Selected | ItemViewElement::Hover, scope) == selected) << tree->objectName().toStdString();
                }
                // the item of an inactive window: rgb(60,60,60), rgb(192,192,192); its branch keeps the selected fill (`::item:selected:!active` alone)
                const unsigned inactiveSelected = ItemViewElement::Selected | ItemViewElement::Inactive;
                EXPECT_TRUE(element->fillOf(inactiveSelected, ItemViewElement::OnItem) == hover) << tree->objectName().toStdString();
                EXPECT_TRUE(element->selectedTextOf(inactiveSelected) == inactiveText) << tree->objectName().toStdString();
                EXPECT_TRUE(element->fillOf(inactiveSelected, ItemViewElement::OnBranch) == selected) << tree->objectName().toStdString();
                // the header of a tree is the global one: no rule of the sheet gives the sections of a plain tree a color
                EXPECT_TRUE(element->m_headerFill == QColor(0x2D, 0x2D, 0x2D)) << tree->objectName().toStdString();
            }

            // The views the sheets say nothing of have the Global element of their kind
            EXPECT_EQ(name(plainTree), QByteArray("Global.ItemView.Tree"));
            EXPECT_EQ(name(plainList), QByteArray("Global.ItemView.List"));
            EXPECT_EQ(name(plainTable), QByteArray("Global.ItemView.Table"));

            // The list of the Asset Relocation page ("No types are enabled for asset relocation." is its only row when no type is) is a QListView no
            // rule of the sheets names: it has the Global list's items, `QListView::item { padding-left: 4px }` (TableView.qss:31-40). n2: the first ink
            // of that row is at device column 12 of the list's text rect, f5 had it at 6 (the 4px at 150%)
            EXPECT_EQ(name(relocation), QByteArray("Global.ItemView.List"));
            EXPECT_EQ(ItemViewElement::forWidget(relocation)->m_itemPadding.left(), 4);
        }

        TEST_F(FlatElementsTests, ItemViews_TheTreeOfAFilterPopupHasNoIndentationAndEveryOtherTreeKeepsItsOwn)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            FilteredSearchWidget details;
            details.setObjectName(QStringLiteral("detailsFilterWidget"));
            FilteredSearchWidget job;
            job.setObjectName(QStringLiteral("jobFilteredSearchWidget"));
            FilteredSearchWidget assetData;
            assetData.setObjectName(QStringLiteral("assetDataFilteredSearchWidget"));
            QTreeView* detailsTree = details.findChild<QTreeView*>();
            QTreeView* jobTree = job.findChild<QTreeView*>();
            QTreeView* assetTree = assetData.findChild<QTreeView*>();
            ASSERT_TRUE(detailsTree != nullptr && jobTree != nullptr && assetTree != nullptr);
            for (FilteredSearchWidget* bar : { &details, &job, &assetData })
            {
                resolveViews(bar);
            }

            // AssetsTab.qss:67-77 names two bars: their tree has the indentation 0, and the Global tree's 20 is what the third bar's has
            for (QTreeView* tree : { detailsTree, jobTree })
            {
                const ItemViewElement* element = ItemViewElement::forWidget(tree);
                ASSERT_TRUE(element != nullptr);
                EXPECT_STREQ(element->m_name, "AssetProcessor.ItemView.FilterPopup");
                EXPECT_EQ(element->m_indentation, 0);
                EXPECT_EQ(tree->indentation(), 0);
            }
            const ItemViewElement* other = ItemViewElement::forWidget(assetTree);
            ASSERT_TRUE(other != nullptr);
            EXPECT_STREQ(other->m_name, "Global.ItemView.Tree");
            EXPECT_EQ(assetTree->indentation(), 20);
        }
    } // namespace FlatElementsTestsInternal
} // namespace AssetProcessor
