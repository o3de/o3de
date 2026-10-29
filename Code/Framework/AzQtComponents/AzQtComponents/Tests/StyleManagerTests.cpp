/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include <QApplication>
#include <QEvent>
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
        // An AP details panel: a sheet of its .ui (SourceAssetDetailsPanel.ui:19-21) that legacy replaced
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
        // assetNameLabel has a sheet of its own (SourceAssetDetailsPanel.ui:40-42), which legacy kept as well
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
} // namespace AzQtComponents
