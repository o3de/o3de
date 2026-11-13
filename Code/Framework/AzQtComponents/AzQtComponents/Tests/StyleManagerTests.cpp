/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyleSheetCache.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QStringList>
#include <QStyle>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Fixture: an application, and the lines StyleManager logs. Everything the tests share
    // is a member of it: the test files of this module are built as one translation unit.
    // ----------------------------------------------------------------------------------

    class StyleManagerTests
        : public ::UnitTest::LeakDetectionFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        StyleManagerTests()
        {
            for (int& runs : s_runs)
            {
                runs = 0;
            }
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
            BusConnect();
        }

        ~StyleManagerTests() override
        {
            BusDisconnect();
            m_application.reset();
        }

        // ---- registrars are plain functions: each test counts the runs of its own in a slot
        enum RunSlot
        {
            Once,
            Several1,
            Several2,
            Extension,
            Duplicate,
            Late,
            Chained,
            RunSlotCount
        };

        inline static int s_runs[RunSlotCount] = {};

        template<int Slot>
        static void countRun()
        {
            ++s_runs[Slot];
        }

        inline static QString s_chainedSheet;

        //! Registers enough sheets, while the registrars of another sheet run, to move the registry
        static void registerSheets()
        {
            for (int i = 0; i < 64; ++i)
            {
                StyleManager::registerFlatSheet(newSheet("Many"), &countRun<Chained>);
            }
            StyleManager::registerFlatSheet(s_chainedSheet, &countRun<Chained>);
        }

        //! The registry lives as long as the process: every test names its sheets anew, so that no other
        //! test (nor an earlier run of the same test) has set them before
        static QString newSheet(const char* what)
        {
            static int number = 0;
            return QStringLiteral("style:StyleManagerTests_%1_%2.qss").arg(QString::fromUtf8(what)).arg(++number);
        }

        //! The flat branch of setStyleSheet (private: this fixture is a friend of StyleManager). setStyleSheet
        //! itself needs the StyleManager instance, and the environment variable that selects the flattened GUI
        //! is read once per process.
        static void applyFlatSheet(QWidget* widget, const QString& styleFileName)
        {
            StyleManager::applyFlatSheet(widget, styleFileName);
        }

        //! The text StyleManager sets for a sheet it loads (private: this fixture is a friend of StyleManager)
        static QString loadedSheet(StyleManager& manager, const QString& styleFileName)
        {
            return manager.m_stylesheetCache->loadStyleSheet(styleFileName);
        }

        //! The rung of the process for the life of the object, put back after it. The process runs at the flat rung (the environment
        //! sets it, AzQtComponentTests.cpp), so the legacy GUI is a rung a test steps into.
        class RungOfTheProcess
        {
        public:
            explicit RungOfTheProcess(GuiPerfFloor::Rung rung)
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(rung);
            }

            ~RungOfTheProcess()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

            RungOfTheProcess(const RungOfTheProcess&) = delete;
            RungOfTheProcess& operator=(const RungOfTheProcess&) = delete;

        private:
            GuiPerfFloor::Rung m_before;
        };

        //! The sheet of a sample page of the Gallery (StyleSheetPage.qss), as a resource of the tests (qrc1.qrc)
        static QString sampleSheet()
        {
            return QStringLiteral(":/StyleSheetTests1/pagesheet.qss");
        }

        //! Counts the StyleChange events of the widget it is installed on
        class StyleChangeCounter
            : public QObject
        {
        public:
            bool eventFilter(QObject*, QEvent* event) override
            {
                m_count += (event->type() == QEvent::StyleChange) ? 1 : 0;
                return false;
            }

            int m_count = 0;
        };

        // ---- AZ::Debug::TraceMessageBus::Handler
        bool OnPrintf(const char* window, const char* message) override
        {
            if (AZStd::string_view(window) == "StyleManager")
            {
                m_logged.append(QString::fromUtf8(message));
            }
            return false;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        QStringList m_logged;
    };

    // ----------------------------------------------------------------------------------
    // T1: the widget's own sheet
    // ----------------------------------------------------------------------------------

    TEST_F(StyleManagerTests, ApplyFlatSheet_ClearsTheOwnSheetOfTheWidget)
    {
        // An AP details panel: a sheet of its .ui that legacy replaced (SourceAssetDetailsPanel.ui:19-21 before the flattening; the
        // panel's constructor sets it now, in the legacy GUI only)
        QFrame panel;
        panel.setStyleSheet(QStringLiteral("background-color: rgb(87, 87, 87);"));
        QLabel* label = new QLabel(&panel);
        ASSERT_TRUE(label->style()->inherits("QStyleSheetStyle"));

        applyFlatSheet(&panel, newSheet("Clears"));

        EXPECT_TRUE(panel.styleSheet().isEmpty());
        EXPECT_FALSE(panel.style()->inherits("QStyleSheetStyle"));
        EXPECT_FALSE(label->style()->inherits("QStyleSheetStyle"));     // nothing inside is drawn through a sheet any more
    }

    TEST_F(StyleManagerTests, ApplyFlatSheet_KeepsTheSheetOfADescendant)
    {
        // assetNameLabel has a sheet of its own, which legacy kept as well (SourceAssetDetailsPanel.ui:40-42 before the flattening;
        // the panel's constructor sets it now, in the legacy GUI only)
        QFrame panel;
        panel.setStyleSheet(QStringLiteral("background-color: rgb(87, 87, 87);"));
        QLabel* label = new QLabel(&panel);
        label->setStyleSheet(QStringLiteral("font: 12pt;"));

        applyFlatSheet(&panel, newSheet("Descendant"));

        EXPECT_TRUE(panel.styleSheet().isEmpty());
        EXPECT_TRUE(label->styleSheet() == QStringLiteral("font: 12pt;"));
        EXPECT_TRUE(label->style()->inherits("QStyleSheetStyle"));
    }

    TEST_F(StyleManagerTests, ApplyFlatSheet_LeavesAWidgetThatOnlyFollowsASheetAlone)
    {
        QFrame parent;
        parent.setStyleSheet(QStringLiteral("background-color: rgb(87, 87, 87);"));
        QLabel* child = new QLabel(&parent);
        StyleChangeCounter counter;
        child->installEventFilter(&counter);

        applyFlatSheet(child, newSheet("Follows"));

        EXPECT_EQ(counter.m_count, 0);
        EXPECT_TRUE(child->style()->inherits("QStyleSheetStyle"));
    }

    TEST_F(StyleManagerTests, QtRule_ClearingAWidgetThatFollowsASheetRepolishesIt)
    {
        // Why applyFlatSheet looks at the widget's own sheet first (qwidget.cpp:2595-2600, 2753-2764). If this
        // fails, Qt no longer repolishes a widget in setStyleSheet, and the check is not needed.
        QFrame parent;
        parent.setStyleSheet(QStringLiteral("background-color: rgb(87, 87, 87);"));
        QLabel* child = new QLabel(&parent);
        StyleChangeCounter counter;
        child->installEventFilter(&counter);

        child->setStyleSheet(QString());

        EXPECT_EQ(counter.m_count, 1);
    }

    // ----------------------------------------------------------------------------------
    // T2: the registrars
    // ----------------------------------------------------------------------------------

    TEST_F(StyleManagerTests, ApplyFlatSheet_RunsTheRegistrarOncePerSheet)
    {
        const QString sheet = newSheet("Once");
        StyleManager::registerFlatSheet(sheet, &countRun<Once>);
        EXPECT_EQ(s_runs[Once], 0);     // legacy never reaches applyFlatSheet: a registration alone runs nothing

        QWidget first;
        QWidget second;
        applyFlatSheet(&first, sheet);
        applyFlatSheet(&second, sheet);
        applyFlatSheet(&first, newSheet("Other"));      // another sheet is not this registrar's

        EXPECT_EQ(s_runs[Once], 1);
    }

    TEST_F(StyleManagerTests, ApplyFlatSheet_RunsEveryRegistrarOfTheSheet)
    {
        // Editor.qss is set by the Editor, AzToolsFramework and gems: each module registers its own
        const QString sheet = newSheet("Several");
        StyleManager::registerFlatSheet(sheet, &countRun<Several1>);
        StyleManager::registerFlatSheet(sheet, &countRun<Several2>);

        QWidget widget;
        applyFlatSheet(&widget, sheet);

        EXPECT_EQ(s_runs[Several1], 1);
        EXPECT_EQ(s_runs[Several2], 1);
    }

    TEST_F(StyleManagerTests, RegisterFlatSheet_TheExtensionIsOptional)
    {
        const QString sheet = newSheet("Extension");
        StyleManager::registerFlatSheet(sheet.chopped(4), &countRun<Extension>);    // without ".qss"

        QWidget widget;
        applyFlatSheet(&widget, sheet);

        EXPECT_EQ(s_runs[Extension], 1);
    }

    TEST_F(StyleManagerTests, RegisterFlatSheet_TheSameRegistrarTwiceRunsOnce)
    {
        // A module may register from a constructor that runs more than once
        const QString sheet = newSheet("Duplicate");
        StyleManager::registerFlatSheet(sheet, &countRun<Duplicate>);
        StyleManager::registerFlatSheet(sheet, &countRun<Duplicate>);

        QWidget widget;
        applyFlatSheet(&widget, sheet);

        EXPECT_EQ(s_runs[Duplicate], 1);
    }

    TEST_F(StyleManagerTests, RegisterFlatSheet_AfterTheSheetWasSetRunsAtOnce)
    {
        const QString sheet = newSheet("Late");
        QWidget widget;
        applyFlatSheet(&widget, sheet);

        StyleManager::registerFlatSheet(sheet, &countRun<Late>);
        EXPECT_EQ(s_runs[Late], 1);

        applyFlatSheet(&widget, sheet);     // the sheet is not set again
        EXPECT_EQ(s_runs[Late], 1);
    }

    TEST_F(StyleManagerTests, ApplyFlatSheet_ARegistrarMayRegisterSheets)
    {
        const QString sheet = newSheet("Chain");
        s_chainedSheet = newSheet("Chained");
        StyleManager::registerFlatSheet(sheet, &registerSheets);

        QWidget widget;
        applyFlatSheet(&widget, sheet);
        applyFlatSheet(&widget, s_chainedSheet);

        EXPECT_EQ(s_runs[Chained], 1);
    }

    // ----------------------------------------------------------------------------------
    // T3: the log of the sheets nobody registered (compiled out with the trace macros)
    // ----------------------------------------------------------------------------------

#if defined(AZ_ENABLE_TRACING)
    TEST_F(StyleManagerTests, ApplyFlatSheet_LogsASheetWithoutRegistrarOncePerSheet)
    {
        const QString sheet = newSheet("Census");
        QFrame first;
        first.setObjectName(QStringLiteral("censusFirst"));
        QWidget second;

        applyFlatSheet(&first, sheet);
        applyFlatSheet(&second, sheet);

        ASSERT_EQ(m_logged.size(), 1);
        EXPECT_TRUE(m_logged[0].contains(sheet));
        EXPECT_TRUE(m_logged[0].contains(QStringLiteral("QFrame 'censusFirst'")));
    }

    TEST_F(StyleManagerTests, ApplyFlatSheet_LogsNothingForASheetWithARegistrar)
    {
        const QString sheet = newSheet("Carried");
        StyleManager::registerFlatSheet(sheet, &countRun<Once>);

        QWidget widget;
        applyFlatSheet(&widget, sheet);

        EXPECT_TRUE(m_logged.isEmpty());
    }
#endif // defined(AZ_ENABLE_TRACING)

    // ----------------------------------------------------------------------------------
    // T4: the flat gate that Python reads. The Python tools have no binding for StyleManager::stylesheetsDisabled and cannot see the
    // command-line flag, so initialize publishes the answer as a dynamic property of the QApplication, which PySide reads as
    // QApplication.instance().property("o3deFlatGui").
    // ----------------------------------------------------------------------------------

    TEST_F(StyleManagerTests, FlatGate_TheApplicationHasThePropertyInTheFlatGui)
    {
        const RungOfTheProcess flatGui(GuiPerfFloor::Rung::Flat);
        ASSERT_FALSE(m_application->property(StyleManager::FlatGuiProperty).isValid());

        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        const QVariant gate = m_application->property(StyleManager::FlatGuiProperty);
        ASSERT_TRUE(gate.isValid());
        EXPECT_EQ(gate.typeId(), QMetaType::Bool);      // what Python gets is a bool
        EXPECT_TRUE(gate.toBool());
    }

    TEST_F(StyleManagerTests, FlatGate_TheApplicationHasNoPropertyInTheLegacyGui)
    {
        const RungOfTheProcess legacyGui(GuiPerfFloor::Rung::Qss);
        ASSERT_FALSE(StyleManager::stylesheetsDisabled());

        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_FALSE(m_application->property(StyleManager::FlatGuiProperty).isValid());
        EXPECT_FALSE(m_application->dynamicPropertyNames().contains(QByteArray(StyleManager::FlatGuiProperty)));
    }

    TEST_F(StyleManagerTests, FlatGate_TheLegacyGuiTakesBackAGateAnEarlierFlatGuiSet)
    {
        // The property is the state of the rung that initialize ran at: an application that was flat and is initialized again at the
        // legacy rung (a test process does) does not keep it
        m_application->setProperty(StyleManager::FlatGuiProperty, true);
        const RungOfTheProcess legacyGui(GuiPerfFloor::Rung::Qss);

        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_FALSE(m_application->property(StyleManager::FlatGuiProperty).isValid());
    }

    TEST_F(StyleManagerTests, FlatGate_TheNoopRungIsFlatAsStylesheetsDisabledIs)
    {
        const RungOfTheProcess floor(GuiPerfFloor::Rung::Noop);
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_TRUE(m_application->property(StyleManager::FlatGuiProperty).toBool());
    }

    TEST_F(StyleManagerTests, FlatGate_ThePassthroughRungIsFlatAsStylesheetsDisabledIs)
    {
        const RungOfTheProcess floor(GuiPerfFloor::Rung::Passthrough);
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_TRUE(m_application->property(StyleManager::FlatGuiProperty).toBool());
    }

    // ----------------------------------------------------------------------------------
    // T5: the sample pages' sheets (setSampleStyleSheet), which the Gallery keeps live in both GUIs, are reloaded by refresh in both
    // as they are when a sheet file changes on disk (StyleSheetCache::styleSheetsChanged calls refresh)
    // ----------------------------------------------------------------------------------

    TEST_F(StyleManagerTests, SampleSheet_TheLegacyGuiSetsTheFileAsItIsAndRefreshReloadsIt)
    {
        const RungOfTheProcess legacyGui(GuiPerfFloor::Rung::Qss);
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());
        const QString file = loadedSheet(styleManager, sampleSheet());
        ASSERT_TRUE(file.contains(QStringLiteral("AzQtComponents--FilteredSearchWidget")));

        QWidget page;
        EXPECT_TRUE(StyleManager::setSampleStyleSheet(&page, sampleSheet()));
        EXPECT_EQ(page.styleSheet(), file);

        page.setStyleSheet(QStringLiteral("QLabel { color: red; }"));
        styleManager.refresh();
        EXPECT_EQ(page.styleSheet(), file);
    }

    TEST_F(StyleManagerTests, SampleSheet_TheFlatGuiSetsTheFileBehindTheLabelMarginAndRefreshReloadsIt)
    {
        const RungOfTheProcess flatGui(GuiPerfFloor::Rung::Flat);
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());
        const QString file = loadedSheet(styleManager, sampleSheet());
        ASSERT_TRUE(file.contains(QStringLiteral("AzQtComponents--FilteredSearchWidget")));

        QWidget page;
        EXPECT_TRUE(StyleManager::setSampleStyleSheet(&page, sampleSheet()));
        // The margin every label has (Text.qss:93-94) comes first, so that a margin of the page's own outranks it; the file follows
        // as it is
        const QString margin = QStringLiteral("QLabel { margin-top: 1px; margin-bottom: 1px; }\n");
        const QString set = page.styleSheet();
        EXPECT_EQ(set, margin + file);

        // The page's sheet is something else when the file changed: refresh sets the file's text on the tracked page again
        page.setStyleSheet(QStringLiteral("QLabel { color: red; }"));
        styleManager.refresh();
        EXPECT_EQ(page.styleSheet(), set);
    }

    TEST_F(StyleManagerTests, SampleSheet_TheFlatGuiReloadsTheSamplesAndNoSheetItCarriesByElements)
    {
        const RungOfTheProcess flatGui(GuiPerfFloor::Rung::Flat);
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        // A sheet carried by elements is no sheet of the widget in the flat GUI and is not tracked, so refresh leaves the widget bare
        QWidget carried;
        EXPECT_FALSE(StyleManager::setStyleSheet(&carried, sampleSheet()));
        QWidget sample;
        EXPECT_TRUE(StyleManager::setSampleStyleSheet(&sample, sampleSheet()));
        sample.setStyleSheet(QString());

        styleManager.refresh();

        EXPECT_TRUE(carried.styleSheet().isEmpty());
        EXPECT_FALSE(sample.styleSheet().isEmpty());
    }

    TEST_F(StyleManagerTests, SampleSheet_ADestroyedPageIsNotTrackedAnyMore)
    {
        const RungOfTheProcess flatGui(GuiPerfFloor::Rung::Flat);
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        {
            QWidget page;
            ASSERT_TRUE(StyleManager::setSampleStyleSheet(&page, sampleSheet()));
        }

        styleManager.refresh();     // a widget that is gone is not touched: no crash
        SUCCEED();
    }
} // namespace AzQtComponents
