/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/GuiPerfCounters.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/Card.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QHash>
#include <QLabel>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QStyleFactory>
#include <QWidget>

namespace AzQtComponents
{
    //! Named, not anonymous: a unity build joins this file with the other tests, where anonymous namespaces merge
    namespace GuiPerfCountersTestsInternal
    {
        constexpr const char* EnableVariable = "O3DE_UI_PERF_COUNTERS";
        constexpr const char* SnapshotProperty = "_o3de_perf_snapshot";

        //! A rule of any kind: all that counts is that the widget has a style sheet of its own
        const QString OwnSheet = QStringLiteral("QWidget { margin: 0px; }");

        //! The keys of the snapshot line, in the order the contract (GuiPerfCounters v=1) gives them
        QStringList contractKeys()
        {
            return QStringLiteral(
                       "polish polish_sheet unpolish unpolish_sheet polish_us unpolish_us ev_polish ev_polish_own_sheet ev_parent "
                       "ev_parent_sheet ev_style ev_style_sheet ev_polish_request repolish_helper repolish_helper_widgets widgets_alive")
                .split(QLatin1Char(' '));
        }

        //! The key of each "key=value" piece of a line, after the three leading pieces ("GuiPerfCounters:", "v=1", "tag=...")
        QStringList keysOf(const QString& line)
        {
            QStringList keys;
            const QStringList pieces = line.split(QLatin1Char(' '));
            for (int i = 3; i < pieces.size(); ++i)
            {
                keys.append(pieces[i].section(QLatin1Char('='), 0, 0));
            }
            return keys;
        }

        //! The value of one key of a line (empty if the line has no such key)
        QString valueOf(const QString& line, const char* key)
        {
            const QString prefix = QString::fromLatin1(key) + QLatin1Char('=');
            for (const QString& piece : line.split(QLatin1Char(' ')))
            {
                if (piece.startsWith(prefix))
                {
                    return piece.mid(prefix.size());
                }
            }
            return QString();
        }

        //! Busy for at least this long. Not Sleep: that rounds to the timer tick of the system (15.6 ms by default), which would make the
        //! time of a scope as uncertain as the thresholds the tests compare it with.
        void spinFor(int milliseconds)
        {
            QElapsedTimer timer;
            timer.start();
            while (timer.nsecsElapsed() < static_cast<qint64>(milliseconds) * 1000 * 1000)
            {
            }
        }

        //! The counts per class, by the name of the class
        QHash<QString, GuiPerfCounters::ClassCounts> classCountsByName()
        {
            QHash<QString, GuiPerfCounters::ClassCounts> byName;
            for (const GuiPerfCounters::ClassCounts& row : GuiPerfCounters::classCounts())
            {
                byName.insert(row.m_name, row);
            }
            return byName;
        }

        //! Sets (or, with a null value, unsets) an environment variable for the life of the object
        class ScopedEnvironment
        {
        public:
            ScopedEnvironment(const char* name, const char* value)
                : m_name(name)
                , m_wasSet(qEnvironmentVariableIsSet(name))
                , m_previous(qgetenv(name))
            {
                set(value);
            }

            ~ScopedEnvironment()
            {
                set(m_wasSet ? m_previous.constData() : nullptr);
            }

        private:
            void set(const char* value)
            {
                if (value)
                {
                    qputenv(m_name, value);
                }
                else
                {
                    qunsetenv(m_name);
                }
            }

            const char* m_name;
            bool m_wasSet;
            QByteArray m_previous;
        };

        //! Remembers the types of the events that reach it as an application wide filter, and lets every event go on
        class EventProbe
            : public QObject
        {
        public:
            explicit EventProbe(QObject* parent)
                : QObject(parent)
            {
            }

            bool eventFilter([[maybe_unused]] QObject* watched, QEvent* event) override
            {
                m_seen.insert(static_cast<int>(event->type()));
                return false;
            }

            QSet<int> m_seen;
        };
    } // namespace GuiPerfCountersTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: an application with the style of the flattened GUI, and the lines the counters write
    // ----------------------------------------------------------------------------------

    class GuiPerfCountersTests
        : public ::UnitTest::LeakDetectionFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        GuiPerfCountersTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // The style sheet of the application is a static of Qt and outlives the application (QApplicationPrivate::styleSheet,
            // qapplication.cpp:322): start with none, whatever an earlier test left, and leave none
            m_application->setStyleSheet(QString());

            // As StyleManager::initialize builds it (StyleManager.cpp:329-331); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        ~GuiPerfCountersTests() override
        {
            BusDisconnect();
            GuiPerfCounters::captureLinesInto(nullptr);
            if (m_application)
            {
                m_application->setStyleSheet(QString());
            }
            m_application.reset();
        }

        //! Turns the counters on, with their lines going to m_lines. A test builds and polishes the widgets it measures first, so
        //! that every counter then starts at the action under test.
        void countFromHere()
        {
            GuiPerfCounters::captureLinesInto(&m_lines);
            GuiPerfCounters::install(m_application.get());
        }

        //! The snapshot request, as the PerfBench kit makes it
        void snapshot(const char* tag)
        {
            m_application->setProperty(GuiPerfCountersTestsInternal::SnapshotProperty, QString::fromLatin1(tag));
        }

        // ---- AZ::Debug::TraceMessageBus::Handler
        bool OnPrintf(const char* window, const char* message) override
        {
            if (AZStd::string_view(window) == "GuiPerfCounters")
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
        QStringList m_lines;                                // the lines captured instead of being written to the log
        QStringList m_logged;                               // the lines that reached the log
    };

    // ----------------------------------------------------------------------------------
    // T1: install (O3DE_UI_PERF_COUNTERS, read once at install)
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, InstallIfRequested_StaysOffWithoutTheVariable)
    {
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, nullptr);

        GuiPerfCounters::installIfRequested(m_application.get());

        EXPECT_FALSE(GuiPerfCounters::isInstalled());
    }

    TEST_F(GuiPerfCountersTests, InstallIfRequested_StaysOffForZero)
    {
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, "0");

        GuiPerfCounters::installIfRequested(m_application.get());

        EXPECT_FALSE(GuiPerfCounters::isInstalled());
    }

    TEST_F(GuiPerfCountersTests, InstallIfRequested_TurnsOnForAnyOtherValue)
    {
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, "1");

        GuiPerfCounters::installIfRequested(m_application.get());

        EXPECT_TRUE(GuiPerfCounters::isInstalled());
    }

    TEST_F(GuiPerfCountersTests, Install_GoesWithTheApplicationAndTwiceIsOnce)
    {
        countFromHere();
        GuiPerfCounters::install(m_application.get());     // already installed: nothing new, nothing reset
        {
            QWidget widget;
            widget.ensurePolished();
            GuiPerfCounters::install(m_application.get());
            EXPECT_EQ(GuiPerfCounters::counters().m_polish, 1u);
        }

        m_application.reset();

        EXPECT_FALSE(GuiPerfCounters::isInstalled());
        GuiPerfCounters::PolishScope afterwards(nullptr);   // a hook with the counters gone does nothing
    }

    TEST_F(GuiPerfCountersTests, Off_CountsNothingAndWritesNothing)
    {
        GuiPerfCounters::captureLinesInto(&m_lines);
        QWidget widget;

        widget.ensurePolished();
        widget.style()->unpolish(&widget);
        widget.style()->polish(&widget);
        GuiPerfCounters::onRepolishHelper(3);
        snapshot("off");

        EXPECT_FALSE(GuiPerfCounters::isInstalled());
        EXPECT_TRUE(GuiPerfCounters::counters() == GuiPerfCounters::Counters());
        EXPECT_TRUE(m_lines.isEmpty());
    }

    // ----------------------------------------------------------------------------------
    // T2: the hooks of Style::polish and Style::unpolish
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, Style_CountsEveryPolishAndUnpolishCall)
    {
        QWidget widget;
        countFromHere();

        widget.ensurePolished();                    // QEvent::Polish: style()->polish(this), qwidget.cpp:9108
        EXPECT_EQ(GuiPerfCounters::counters().m_polish, 1u);
        EXPECT_EQ(GuiPerfCounters::counters().m_unpolish, 0u);

        widget.style()->unpolish(&widget);
        widget.style()->polish(&widget);

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_EQ(counters.m_unpolish, 1u);
        EXPECT_EQ(counters.m_polishSheet, 0u);      // no style sheet anywhere
        EXPECT_EQ(counters.m_unpolishSheet, 0u);
    }

    TEST_F(GuiPerfCountersTests, Style_TimesOnlyTheOutermostPolish)
    {
        countFromHere();

        QElapsedTimer wall;
        wall.start();
        {
            GuiPerfCounters::PolishScope outer(nullptr);
            GuiPerfCountersTestsInternal::spinFor(20);
            {
                GuiPerfCounters::PolishScope nested(nullptr);       // a polish inside a polish: counted, but its time is in the outer one
                GuiPerfCountersTestsInternal::spinFor(20);
            }
        }
        const quint64 wallNanoseconds = static_cast<quint64>(wall.nsecsElapsed());

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_GE(counters.m_polishNanoseconds, 40u * 1000u * 1000u);       // the outer call took 40 ms
        EXPECT_LE(counters.m_polishNanoseconds, wallNanoseconds);           // and the nested 20 ms are not added a second time
        EXPECT_EQ(counters.m_unpolishNanoseconds, 0u);
    }

    TEST_F(GuiPerfCountersTests, Style_TimesOnlyTheOutermostUnpolish)
    {
        countFromHere();

        QElapsedTimer wall;
        wall.start();
        {
            GuiPerfCounters::UnpolishScope outer(nullptr);
            GuiPerfCountersTestsInternal::spinFor(20);
            {
                GuiPerfCounters::UnpolishScope nested(nullptr);
                GuiPerfCountersTestsInternal::spinFor(20);
            }
        }
        const quint64 wallNanoseconds = static_cast<quint64>(wall.nsecsElapsed());

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_unpolish, 2u);
        EXPECT_GE(counters.m_unpolishNanoseconds, 40u * 1000u * 1000u);
        EXPECT_LE(counters.m_unpolishNanoseconds, wallNanoseconds);
        EXPECT_EQ(counters.m_polishNanoseconds, 0u);
    }

    TEST_F(GuiPerfCountersTests, Style_TimesACallWhateverTheOtherKindDoes)
    {
        countFromHere();

        {
            GuiPerfCounters::PolishScope polish(nullptr);
            GuiPerfCountersTestsInternal::spinFor(5);
            {
                // An unpolish inside a polish (Style::addClass does that): the outermost unpolish, whatever polish it sits in
                GuiPerfCounters::UnpolishScope unpolish(nullptr);
                GuiPerfCountersTestsInternal::spinFor(20);
            }
        }

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_GE(counters.m_unpolishNanoseconds, 20u * 1000u * 1000u);
        EXPECT_GE(counters.m_polishNanoseconds, counters.m_unpolishNanoseconds);    // the polish took the unpolish and more
    }

    TEST_F(GuiPerfCountersTests, Style_CallsOneAfterTheOtherAreAllTimed)
    {
        countFromHere();

        for (int i = 0; i < 2; ++i)
        {
            GuiPerfCounters::PolishScope polish(nullptr);
            GuiPerfCountersTestsInternal::spinFor(10);
        }
        for (int i = 0; i < 2; ++i)
        {
            GuiPerfCounters::UnpolishScope unpolish(nullptr);
            GuiPerfCountersTestsInternal::spinFor(10);
        }

        // A call that ended is not running any more: the next one is an outermost call again
        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_GE(counters.m_polishNanoseconds, 20u * 1000u * 1000u);
        EXPECT_EQ(counters.m_unpolish, 2u);
        EXPECT_GE(counters.m_unpolishNanoseconds, 20u * 1000u * 1000u);
    }

    TEST_F(GuiPerfCountersTests, Style_ACallFromAnotherThreadIsNotCounted)
    {
        QWidget widget;
        countFromHere();

        AZStd::thread worker(
            [&widget]()
            {
                GuiPerfCounters::PolishScope polish(&widget);
                GuiPerfCounters::UnpolishScope unpolish(&widget);
                GuiPerfCounters::onRepolishHelper(5);
            });
        worker.join();

        GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        counters.m_widgetsAlive = 0;                // a level, not a count
        EXPECT_TRUE(counters == GuiPerfCounters::Counters());
    }

    // ----------------------------------------------------------------------------------
    // T3: Qt::WA_StyleSheet splits of polish and unpolish
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, Sheet_CountsTheCallsForAWidgetThatHasOne)
    {
        QWidget plain;
        QWidget withSheet;
        withSheet.setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        plain.ensurePolished();
        withSheet.ensurePolished();                 // the sheet style set Qt::WA_StyleSheet on it (qstylesheetstyle.cpp:2864)
        ASSERT_TRUE(withSheet.testAttribute(Qt::WA_StyleSheet));
        ASSERT_FALSE(plain.testAttribute(Qt::WA_StyleSheet));
        countFromHere();

        plain.style()->polish(&plain);
        withSheet.style()->polish(&withSheet);      // through its sheet style, which calls Style::polish first (:2875)
        QApplication::style()->unpolish(&plain);    // Style::unpolish itself, as StyleHelpers::repolishWhenPropertyChanges calls it
        QApplication::style()->unpolish(&withSheet);// when the sheets are off (StyleManager::styleSheetStyle is then Style)

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_EQ(counters.m_polishSheet, 1u);
        EXPECT_EQ(counters.m_unpolish, 2u);
        EXPECT_EQ(counters.m_unpolishSheet, 1u);
    }

    TEST_F(GuiPerfCountersTests, Sheet_AnUnpolishThroughTheSheetStyleIsCounted)
    {
        // The attribute would miss it: QStyleSheetStyle::unpolish clears Qt::WA_StyleSheet (qstylesheetstyle.cpp:3018) before it calls
        // the base style (:3028). The style of the widget is the sheet style all the same.
        QWidget withSheet;
        withSheet.setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        withSheet.ensurePolished();
        ASSERT_TRUE(withSheet.testAttribute(Qt::WA_StyleSheet));
        countFromHere();

        withSheet.style()->unpolish(&withSheet);

        EXPECT_EQ(GuiPerfCounters::counters().m_unpolish, 1u);
        EXPECT_EQ(GuiPerfCounters::counters().m_unpolishSheet, 1u);
        EXPECT_FALSE(withSheet.testAttribute(Qt::WA_StyleSheet));
    }

    TEST_F(GuiPerfCountersTests, Sheet_TheFirstPolishThroughTheSheetStyleIsCounted)
    {
        // The attribute would miss it: QStyleSheetStyle::polish calls the base style first (qstylesheetstyle.cpp:2875) and sets
        // Qt::WA_StyleSheet after (:2864).
        QWidget withSheet;
        withSheet.setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        ASSERT_FALSE(withSheet.testAttribute(Qt::WA_StyleSheet));
        countFromHere();

        withSheet.ensurePolished();

        EXPECT_EQ(GuiPerfCounters::counters().m_polish, 1u);
        EXPECT_EQ(GuiPerfCounters::counters().m_polishSheet, 1u);
        EXPECT_TRUE(withSheet.testAttribute(Qt::WA_StyleSheet));
    }

    // ----------------------------------------------------------------------------------
    // T4: QEvent::Polish and PolishRequest
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, Events_PolishComesOncePerWidgetAndSplitsOnTheSheetOfItsOwn)
    {
        QWidget parent;
        new QWidget(&parent);
        QWidget* withSheet = new QWidget(&parent);
        withSheet->setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        countFromHere();

        parent.ensurePolished();                    // the widget, then its children (qwidget.cpp:10239)
        parent.ensurePolished();                    // a widget is polished once (:10244)

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_evPolish, 3u);
        EXPECT_EQ(counters.m_evPolishOwnSheet, 1u);
        EXPECT_EQ(counters.m_polish, 3u);           // one Style::polish call for each of them
    }

    TEST_F(GuiPerfCountersTests, Events_PolishRequestIsCountedWhenItIsDelivered)
    {
        QWidget widget;                             // posts a PolishRequest as it is constructed (qwidget.cpp:1011)
        countFromHere();
        EXPECT_EQ(GuiPerfCounters::counters().m_evPolishRequest, 0u);

        QCoreApplication::sendPostedEvents(&widget, QEvent::PolishRequest);     // what the event loop does; it polishes the widget

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_evPolishRequest, 1u);
        EXPECT_EQ(counters.m_evPolish, 1u);
    }

    TEST_F(GuiPerfCountersTests, Events_TheFilterNeverConsumesAnEvent)
    {
        // The probe is installed first, so it is the last filter the application asks: it sees an event only if the filters before it,
        // the counters among them, let it go on. (Qt asks the filter installed last first, QObject::installEventFilter.)
        GuiPerfCountersTestsInternal::EventProbe probe(m_application.get());
        m_application->installEventFilter(&probe);
        m_application->setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);   // so that a widget out of its parent has a style change
        countFromHere();

        {
            QWidget parent;
            QWidget* child = new QWidget(&parent);                                  // ParentChange, while the widget is constructed
            parent.ensurePolished();                                                // Polish
            QCoreApplication::sendPostedEvents(child, QEvent::PolishRequest);       // PolishRequest
            child->setParent(nullptr);                                              // ParentChange and StyleChange
            snapshot("probe");                                                      // DynamicPropertyChange, sent to the application
            delete child;
        }

        for (const QEvent::Type type :
             { QEvent::Polish, QEvent::PolishRequest, QEvent::ParentChange, QEvent::StyleChange, QEvent::DynamicPropertyChange })
        {
            EXPECT_TRUE(probe.m_seen.contains(static_cast<int>(type))) << "event type " << static_cast<int>(type);
        }
        EXPECT_EQ(m_lines.size(), 1);   // and the counters themselves did count them
    }

    // ----------------------------------------------------------------------------------
    // T5: QEvent::ParentChange and StyleChange, and what a reparent costs
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, Reparent_WithoutAnySheetIsOneParentChangeAndNothingElse)
    {
        QWidget from;
        QWidget to;
        QWidget* child = new QWidget(&from);
        from.ensurePolished();
        to.ensurePolished();
        countFromHere();

        child->setParent(&to);

        // The flattened GUI without sheets: Qt keeps the style of the widget (qwidget.cpp:2768), so nothing is polished
        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_evParent, 1u);
        EXPECT_EQ(counters.m_evParentSheet, 0u);
        EXPECT_EQ(counters.m_evStyle, 0u);
        EXPECT_EQ(counters.m_polish, 0u);
        EXPECT_EQ(counters.m_unpolish, 0u);
    }

    TEST_F(GuiPerfCountersTests, Reparent_OfAWidgetWithASheetOfItsOwnPolishesItsSubtreeAgain)
    {
        // The card header of the property editor: an inline sheet, which stays in the flattened GUI. Qt repolishes the widget and its
        // children (QStyleSheetStyle::repolish, qwidget.cpp:2738) without an unpolish, and walks the children twice
        // (qstylesheetstyle.cpp:2982-2990, 2801)
        QWidget from;
        QWidget to;
        QWidget* header = new QWidget(&from);
        header->setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        new QWidget(header);
        from.ensurePolished();
        to.ensurePolished();
        countFromHere();

        header->setParent(&to);

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_evParent, 1u);
        EXPECT_EQ(counters.m_evParentSheet, 1u);
        EXPECT_EQ(counters.m_polish, 3u);           // the child twice, the header once
        EXPECT_EQ(counters.m_polishSheet, 3u);
        EXPECT_EQ(counters.m_unpolish, 0u);
        EXPECT_EQ(counters.m_evStyle, 3u);
        EXPECT_EQ(counters.m_evStyleSheet, 3u);

        // The same, by class: the header and its child are both QWidget
        const QHash<QString, GuiPerfCounters::ClassCounts> byName = GuiPerfCountersTestsInternal::classCountsByName();
        ASSERT_TRUE(byName.contains(QStringLiteral("QWidget")));
        EXPECT_EQ(byName.value(QStringLiteral("QWidget")).m_polish, 3u);
        EXPECT_EQ(byName.value(QStringLiteral("QWidget")).m_polishSheet, 3u);
        EXPECT_EQ(byName.value(QStringLiteral("QWidget")).m_evParent, 1u);
        EXPECT_EQ(byName.value(QStringLiteral("QWidget")).m_evStyle, 3u);
    }

    TEST_F(GuiPerfCountersTests, Reparent_IntoASheetSubtreeAndOutAgainUnpolishesAndPolishesEachWidgetOnce)
    {
        QWidget plain;
        QWidget sheetParent;
        sheetParent.setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        QWidget* child = new QWidget(&plain);
        new QWidget(child);
        plain.ensurePolished();
        sheetParent.ensurePolished();
        countFromHere();

        child->setParent(&sheetParent);         // the widget and its child take the style of the sheet parent (qwidget.cpp:2753-2762)

        GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_unpolish, 2u);
        EXPECT_EQ(counters.m_unpolishSheet, 2u);    // the widget has its new style, the sheet style, when the old one unpolishes it (:2694)
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_EQ(counters.m_polishSheet, 2u);
        EXPECT_EQ(counters.m_evStyle, 2u);
        EXPECT_EQ(counters.m_evStyleSheet, 2u); // both have the attribute by the time the StyleChange arrives
        EXPECT_EQ(counters.m_evParent, 1u);
        EXPECT_EQ(counters.m_evParentSheet, 1u);

        child->setParent(&plain);               // and back to the style of the application

        counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_unpolish, 4u);
        EXPECT_EQ(counters.m_unpolishSheet, 2u);    // the widgets leave the sheet style: their new style is the plain one
        EXPECT_EQ(counters.m_polish, 4u);
        EXPECT_EQ(counters.m_polishSheet, 2u);
        EXPECT_EQ(counters.m_evStyle, 4u);
        EXPECT_EQ(counters.m_evParent, 2u);
        EXPECT_EQ(counters.m_evParentSheet, 1u);    // the style of the application has no sheet: the attribute is gone
    }

    TEST_F(GuiPerfCountersTests, QtRule_ATripOutOfAndBackIntoAParentIsFreeWithoutSheets)
    {
        // Property editor pooling in the flattened GUI: setParent(nullptr) and back
        QWidget parent;
        QWidget* pooled = new QWidget(&parent);
        new QWidget(pooled);
        new QWidget(pooled);
        parent.ensurePolished();
        countFromHere();

        pooled->setParent(nullptr);
        pooled->setParent(&parent);

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_evParent, 2u);
        EXPECT_EQ(counters.m_polish, 0u);
        EXPECT_EQ(counters.m_unpolish, 0u);
        EXPECT_EQ(counters.m_evStyle, 0u);
    }

    TEST_F(GuiPerfCountersTests, QtRule_ATripOutOfAndBackIntoAParentUnderAnApplicationSheetCostsOneUnpolishAndPolishPerWidgetEachWay)
    {
        // The premise of #19932 and #19947: setParent re-derives the style of every widget of the subtree (QWidgetPrivate::inheritStyle,
        // qwidget.cpp:2753-2762, then setStyle_helper unpolishes and polishes each polished widget, :2694-2695 and propagates to the
        // children, :2699-2704). With the sheets of the application on, the style of a widget is the sheet style wrapped around Style.
        m_application->setStyleSheet(GuiPerfCountersTestsInternal::OwnSheet);
        QWidget parent;
        QWidget* pooled = new QWidget(&parent);
        new QWidget(pooled);
        new QWidget(pooled);
        parent.ensurePolished();
        countFromHere();

        pooled->setParent(nullptr);

        GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_unpolish, 3u);     // the widget and its two children
        EXPECT_EQ(counters.m_unpolishSheet, 3u);
        EXPECT_EQ(counters.m_polish, 3u);
        EXPECT_EQ(counters.m_polishSheet, 3u);
        EXPECT_EQ(counters.m_evStyle, 3u);
        EXPECT_EQ(counters.m_evStyleSheet, 3u);
        EXPECT_EQ(counters.m_evParent, 1u);
        EXPECT_EQ(counters.m_evParentSheet, 1u);

        pooled->setParent(&parent);

        counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_unpolish, 6u);
        EXPECT_EQ(counters.m_unpolishSheet, 6u);
        EXPECT_EQ(counters.m_polish, 6u);
        EXPECT_EQ(counters.m_polishSheet, 6u);
        EXPECT_EQ(counters.m_evStyle, 6u);
        EXPECT_EQ(counters.m_evStyleSheet, 6u);
        EXPECT_EQ(counters.m_evParent, 2u);
        EXPECT_EQ(counters.m_evParentSheet, 2u);
    }

    // ----------------------------------------------------------------------------------
    // T6: the repolish helper, and the install by the StyleManager
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, RepolishHelper_CountsExecutionsAndWidgets)
    {
        countFromHere();

        GuiPerfCounters::onRepolishHelper(4);
        GuiPerfCounters::onRepolishHelper(1);

        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_repolishHelper, 2u);
        EXPECT_EQ(counters.m_repolishHelperWidgets, 5u);
    }

    TEST_F(GuiPerfCountersTests, StyleManager_InstallsTheCountersWhenTheVariableIsSet)
    {
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, "1");
        StyleManager styleManager(nullptr);

        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_TRUE(GuiPerfCounters::isInstalled());
    }

    TEST_F(GuiPerfCountersTests, StyleManager_InstallsNothingWithoutTheVariable)
    {
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, nullptr);
        StyleManager styleManager(nullptr);

        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        EXPECT_FALSE(GuiPerfCounters::isInstalled());
    }

    TEST_F(GuiPerfCountersTests, StyleHelpers_ARepolishCountsTheWidgetAndItsChildren)
    {
        // A card repolishes itself and everything in it when it is selected (Card.cpp:148). The StyleManager is what the helper needs
        // to find the style to repolish with: with the sheets off that is Style itself (StyleManager.cpp:226-230).
        GuiPerfCountersTestsInternal::ScopedEnvironment variable(GuiPerfCountersTestsInternal::EnableVariable, "1");
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        Card card;
        card.ensurePolished();
        const GuiPerfCounters::Counters before = GuiPerfCounters::counters();

        card.setSelected(true);

        const GuiPerfCounters::Counters after = GuiPerfCounters::counters();
        const quint64 widgets = 1u + static_cast<quint64>(card.findChildren<QWidget*>().size());
        EXPECT_EQ(after.m_repolishHelper - before.m_repolishHelper, 1u);
        EXPECT_EQ(after.m_repolishHelperWidgets - before.m_repolishHelperWidgets, widgets);
        EXPECT_EQ(after.m_unpolish - before.m_unpolish, widgets);       // each of them was unpolished and polished through Style
        EXPECT_EQ(after.m_polish - before.m_polish, widgets);
    }

    // ----------------------------------------------------------------------------------
    // T7: the lines
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, FormatLine_CarriesTheKeysInTheOrderOfTheContract)
    {
        using namespace GuiPerfCountersTestsInternal;

        GuiPerfCounters::Counters values;
        values.m_polish = 11;
        values.m_polishSheet = 12;
        values.m_unpolish = 13;
        values.m_unpolishSheet = 14;
        values.m_polishNanoseconds = 15500;     // 15.5 us
        values.m_unpolishNanoseconds = 16260;   // 16.26 us: rounded to one decimal
        values.m_evPolish = 17;
        values.m_evPolishOwnSheet = 18;
        values.m_evParent = 19;
        values.m_evParentSheet = 20;
        values.m_evStyle = 21;
        values.m_evStyleSheet = 22;
        values.m_evPolishRequest = 23;
        values.m_repolishHelper = 24;
        values.m_repolishHelperWidgets = 25;
        values.m_widgetsAlive = 26;

        const QString line = GuiPerfCounters::formatLine(QStringLiteral("S-OC:flat:3:post"), values);

        EXPECT_TRUE(line.startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=S-OC:flat:3:post polish=11 ")));
        EXPECT_EQ(keysOf(line), contractKeys());
        EXPECT_EQ(valueOf(line, "polish_sheet"), QStringLiteral("12"));
        EXPECT_EQ(valueOf(line, "polish_us"), QStringLiteral("15.5"));
        EXPECT_EQ(valueOf(line, "unpolish_us"), QStringLiteral("16.3"));
        EXPECT_EQ(valueOf(line, "ev_polish_request"), QStringLiteral("23"));
        EXPECT_EQ(valueOf(line, "widgets_alive"), QStringLiteral("26"));
        EXPECT_TRUE(line.endsWith(QStringLiteral(" repolish_helper_widgets=25 widgets_alive=26")));
    }

    TEST_F(GuiPerfCountersTests, FormatLine_TimesAreMicrosecondsWithOneDecimal)
    {
        using namespace GuiPerfCountersTestsInternal;

        GuiPerfCounters::Counters values;
        EXPECT_EQ(valueOf(GuiPerfCounters::formatLine(QStringLiteral("t"), values), "polish_us"), QStringLiteral("0.0"));

        values.m_polishNanoseconds = 1234;
        values.m_unpolishNanoseconds = 123456789;
        const QString line = GuiPerfCounters::formatLine(QStringLiteral("t"), values);
        EXPECT_EQ(valueOf(line, "polish_us"), QStringLiteral("1.2"));
        EXPECT_EQ(valueOf(line, "unpolish_us"), QStringLiteral("123456.8"));
    }

    TEST_F(GuiPerfCountersTests, FormatClassesLine_ListsTheTwentyWithTheMostPolishCalls)
    {
        QList<GuiPerfCounters::ClassCounts> classes;
        for (int i = 0; i < 25; ++i)
        {
            GuiPerfCounters::ClassCounts row;
            row.m_name = QStringLiteral("Class%1").arg(i, 2, 10, QLatin1Char('0'));
            row.m_polish = static_cast<quint64>(i / 2);     // pairs share a count: ties go by name
            row.m_polishSheet = 1;
            row.m_evParent = 2;
            row.m_evStyle = 3;
            classes.append(row);
        }

        const QStringList pieces = GuiPerfCounters::formatClassesLine(QStringLiteral("t!classes"), classes).split(QLatin1Char(' '));

        ASSERT_EQ(pieces.size(), 3 + 20);
        EXPECT_EQ(pieces[0], QStringLiteral("GuiPerfCountersClasses:"));
        EXPECT_EQ(pieces[1], QStringLiteral("v=1"));
        EXPECT_EQ(pieces[2], QStringLiteral("tag=t!classes"));
        EXPECT_EQ(pieces[3], QStringLiteral("Class24=12/1/2/3"));       // the most calls, then the name
        EXPECT_EQ(pieces[4], QStringLiteral("Class22=11/1/2/3"));
        EXPECT_EQ(pieces[5], QStringLiteral("Class23=11/1/2/3"));
        EXPECT_EQ(pieces[22], QStringLiteral("Class04=2/1/2/3"));       // the 20th
        EXPECT_FALSE(pieces.contains(QStringLiteral("Class05=2/1/2/3")));   // the 21st is left out
    }

    // ----------------------------------------------------------------------------------
    // T8: the snapshot property
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfCountersTests, Snapshot_WritesOneLineInsideTheSetPropertyCall)
    {
        using namespace GuiPerfCountersTestsInternal;

        countFromHere();
        QWidget widget;
        widget.ensurePolished();

        snapshot("pre");     // the line is there when setProperty returns

        ASSERT_EQ(m_lines.size(), 1);
        const QString line = m_lines[0];
        EXPECT_TRUE(line.startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=pre polish=1 polish_sheet=0 unpolish=0 ")));
        EXPECT_EQ(keysOf(line), contractKeys());
        EXPECT_EQ(valueOf(line, "ev_polish"), QStringLiteral("1"));
        EXPECT_EQ(valueOf(line, "widgets_alive").toULongLong(), static_cast<quint64>(QApplication::allWidgets().size()));
        EXPECT_GE(valueOf(line, "widgets_alive").toULongLong(), 1u);       // at least the widget of the test

        snapshot("post");
        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_TRUE(m_lines[1].startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=post ")));
    }

    TEST_F(GuiPerfCountersTests, Snapshot_NothingForAnEmptyTag)
    {
        countFromHere();

        snapshot("");

        EXPECT_TRUE(m_lines.isEmpty());
    }

    TEST_F(GuiPerfCountersTests, Snapshot_AnotherPropertyOfTheApplicationWritesNothing)
    {
        countFromHere();
        snapshot("first");
        ASSERT_EQ(m_lines.size(), 1);

        // The tag is still "first": only the snapshot property asks
        m_application->setProperty("_o3de_something_else", QStringLiteral("x"));

        EXPECT_EQ(m_lines.size(), 1);
    }

    TEST_F(GuiPerfCountersTests, Snapshot_AWidgetThatCarriesThePropertyWritesNothing)
    {
        countFromHere();
        QWidget widget;

        // The application is asked, nobody else
        widget.setProperty(GuiPerfCountersTestsInternal::SnapshotProperty, QStringLiteral("tag"));

        EXPECT_TRUE(m_lines.isEmpty());
    }

    TEST_F(GuiPerfCountersTests, QtRule_TheSameTagTwiceInARowWritesOnce)
    {
        // Qt sends no property change for an equal value (qobject.cpp:4440-4446): the kit's tags carry an index for that reason
        countFromHere();

        snapshot("same");
        snapshot("same");
        snapshot("other");
        snapshot("same");

        EXPECT_EQ(m_lines.size(), 3);
    }

    TEST_F(GuiPerfCountersTests, Snapshot_WidgetsAliveIsALevel)
    {
        using namespace GuiPerfCountersTestsInternal;

        countFromHere();
        snapshot("zero");
        QWidget first;
        QWidget second;
        snapshot("two");

        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_EQ(valueOf(m_lines[0], "widgets_alive").toInt() + 2, valueOf(m_lines[1], "widgets_alive").toInt());
    }

    TEST_F(GuiPerfCountersTests, Snapshot_ClassesLineFollowsForATagThatEndsWithClasses)
    {
        QWidget first;
        QWidget second;
        QLabel label;
        countFromHere();
        first.ensurePolished();
        second.ensurePolished();
        label.ensurePolished();

        snapshot("c!classes");

        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_TRUE(m_lines[0].startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=c!classes ")));
        EXPECT_TRUE(m_lines[1] == QStringLiteral("GuiPerfCountersClasses: v=1 tag=c!classes QWidget=2/0/0/0 QLabel=1/0/0/0"))
            << m_lines[1].toUtf8().constData();

        snapshot("plain");
        EXPECT_EQ(m_lines.size(), 3);   // no classes line for a tag without the suffix

        snapshot("c!classes:post");
        EXPECT_EQ(m_lines.size(), 4);   // nor for a tag that has it in the middle
    }

    TEST_F(GuiPerfCountersTests, Classes_AWidgetConstructedWithAParentSendsItsParentChangeAsAQWidget)
    {
        // QWidget::QWidget sets the parent (qwidget.cpp:10879), when the object is a QWidget and not yet the class that is built
        QWidget parent;
        countFromHere();

        QLabel label(&parent);

        const QHash<QString, GuiPerfCounters::ClassCounts> byName = GuiPerfCountersTestsInternal::classCountsByName();
        ASSERT_TRUE(byName.contains(QStringLiteral("QWidget")));
        EXPECT_EQ(byName.value(QStringLiteral("QWidget")).m_evParent, 1u);
        EXPECT_FALSE(byName.contains(QStringLiteral("QLabel")));
        EXPECT_EQ(GuiPerfCounters::counters().m_evParent, 1u);
    }

    TEST_F(GuiPerfCountersTests, Snapshot_ReachesTheLogWhenNothingCapturesIt)
    {
#if defined(AZ_ENABLE_TRACING)
        BusConnect();
        GuiPerfCounters::install(m_application.get());

        snapshot("log");

        ASSERT_EQ(m_logged.size(), 1);
        EXPECT_TRUE(m_logged[0].startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=log ")));
        EXPECT_TRUE(m_logged[0].endsWith(QLatin1Char('\n')));
#endif // defined(AZ_ENABLE_TRACING)
    }
} // namespace AzQtComponents
