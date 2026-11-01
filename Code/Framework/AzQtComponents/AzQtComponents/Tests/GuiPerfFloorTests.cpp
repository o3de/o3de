/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/GuiPerfCounters.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ColorLabel.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <memory>

namespace AzQtComponents
{
    //! Named, not anonymous: a unity build joins this file with the other tests, where anonymous namespaces merge
    namespace GuiPerfFloorTestsInternal
    {
        using Rung = GuiPerfFloor::Rung;

        constexpr const char* SnapshotProperty = "_o3de_perf_snapshot";
        constexpr const char* RungProperty = "_o3de_floor_rung";
        constexpr const char* CountersVariable = "O3DE_UI_PERF_COUNTERS";
        constexpr const char* FlatVariable = "O3DE_DISABLE_STYLESHEETS";
        constexpr const char* NoopVariable = "O3DE_STYLESHEETS_NOOP";
        constexpr const char* PassthroughVariable = "O3DE_STYLE_PASSTHROUGH";

        //! A rule of any kind: all that counts is that the widget has a style sheet of its own
        const QString OwnSheet = QStringLiteral("QWidget { margin: 0px; }");

        //! The rungs, in the order of the ladder
        const Rung AllRungs[] = { Rung::Qss, Rung::Flat, Rung::Noop, Rung::Passthrough };

        //! For the failure messages: the name of a rung
        const char* named(Rung rung)
        {
            return GuiPerfFloor::rungName(rung);
        }

        //! Whether the style of the widget is a style sheet style (QStyleSheetStyle has no public header, so it is told by its class name)
        bool hasSheetStyle(const QWidget& widget)
        {
            return widget.style()->inherits("QStyleSheetStyle");
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

        //! The key of each "key=value" piece of a line, after the three leading pieces ("GuiPerfFloor:", "v=1", "tag=...")
        QStringList keysAfterTag(const QString& line)
        {
            QStringList keys;
            const QStringList pieces = line.split(QLatin1Char(' '));
            for (int i = 3; i < pieces.size(); ++i)
            {
                keys.append(pieces[i].section(QLatin1Char('='), 0, 0));
            }
            return keys;
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

        //! The three variables of the ladder, set or unset (null) for the life of the object. The test environment sets the kill switch
        //! for the whole module, so a test that asks what a process reads must say what it has set.
        struct ScopedVariables
        {
            ScopedVariables(const char* flat, const char* noop, const char* passthrough)
                : m_flat(FlatVariable, flat)
                , m_noop(NoopVariable, noop)
                , m_passthrough(PassthroughVariable, passthrough)
            {
            }

            ScopedEnvironment m_flat;
            ScopedEnvironment m_noop;
            ScopedEnvironment m_passthrough;
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
    } // namespace GuiPerfFloorTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: an application at a rung of the ladder, the lines the floor and the counters write
    // ----------------------------------------------------------------------------------

    class GuiPerfFloorTests
        : public ::UnitTest::LeakDetectionFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        using Rung = GuiPerfFloor::Rung;

        GuiPerfFloorTests()
        {
            GuiPerfFloor::resetForTesting();        // the rung is read anew, the counts are zero
        }

        ~GuiPerfFloorTests() override
        {
            BusDisconnect();
            GuiPerfFloor::captureLinesInto(nullptr);
            GuiPerfCounters::captureLinesInto(nullptr);
            if (m_application)
            {
                m_application->setStyleSheet(QString());
            }
            m_application.reset();
            GuiPerfFloor::resetForTesting();        // whatever the test made the rung, the next one reads the process again
        }

        //! The application, started with these arguments after the program name. The style sheet of the application is a static of
        //! Qt and outlives the application: start with none, whatever an earlier test left.
        void startApplication(std::initializer_list<const char*> arguments = {})
        {
            m_application.reset();
            m_arguments.clear();
            m_arguments.append(QByteArrayLiteral("test"));
            for (const char* argument : arguments)
            {
                m_arguments.append(argument);
            }
            m_argv.clear();
            for (QByteArray& argument : m_arguments)
            {
                m_argv.append(argument.data());
            }
            m_argv.append(nullptr);
            m_argc = static_cast<int>(m_arguments.size());

            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv.data());
            m_application->setStyleSheet(QString());
        }

        //! The application at this rung (the rung is set, not read), with the style of the flattened GUI on it as StyleManager::initialize
        //! makes it (StyleManager.cpp:330-332); the application owns the style
        void startAt(Rung rung)
        {
            GuiPerfFloor::setRungForTesting(rung);
            startApplication();
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        //! The rung a process started with these arguments reads (its flags, and the variables that are set now)
        Rung rungOfArguments(std::initializer_list<const char*> arguments)
        {
            startApplication(arguments);
            return GuiPerfFloor::readRung();
        }

        //! The elements the flattened GUI registers once for the process (Style::polish(QApplication*)): the tests that polish widgets with
        //! a Style of their own need them as the application has them
        static void registerGlobalElements()
        {
            PropertyEditorElements::registerGlobal();
            ControlElements::registerGlobal();
            ComboBox::registerFlatElements();
        }

        //! The lines of the floor and the counters go to m_lines, in the order they are written
        void captureLines()
        {
            GuiPerfFloor::captureLinesInto(&m_lines);
            GuiPerfCounters::captureLinesInto(&m_lines);
        }

        //! The snapshot request, as the PerfBench kit makes it
        void snapshot(const char* tag)
        {
            m_application->setProperty(GuiPerfFloorTestsInternal::SnapshotProperty, QString::fromLatin1(tag));
        }

        // ---- AZ::Debug::TraceMessageBus::Handler
        bool OnPrintf(const char* window, const char* message) override
        {
            if (AZStd::string_view(window) == "GuiPerfFloor")
            {
                m_logged.append(QString::fromUtf8(message));
            }
            return false;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        QList<QByteArray> m_arguments;
        QVector<char*> m_argv;
        AZStd::unique_ptr<QApplication> m_application;
        QStringList m_lines;                                // the lines captured instead of being written to the log
        QStringList m_logged;                               // the lines that reached the log
    };

    // ----------------------------------------------------------------------------------
    // T1: the rung (flags, variables, implication, read once)
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Rung_IsQssWithoutAFlagOrAVariable)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "qss");
    }

    TEST_F(GuiPerfFloorTests, Rung_KillSwitchFlagIsFlat)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--disable-stylesheets" })), "flat");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--Disable-StyleSheets" })), "flat");
    }

    TEST_F(GuiPerfFloorTests, Rung_KillSwitchVariableKeepsItsMeaningItIsOnWhenSet)
    {
        // O3DE_DISABLE_STYLESHEETS was always on when it was set at all, "0" included: the ladder does not change that
        GuiPerfFloorTestsInternal::ScopedVariables variables("1", nullptr, nullptr);
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "flat");

        GuiPerfFloorTestsInternal::ScopedEnvironment zero(GuiPerfFloorTestsInternal::FlatVariable, "0");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "flat");
    }

    TEST_F(GuiPerfFloorTests, Rung_NoopFlagIsNoop)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--stylesheets-noop" })), "noop");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--StyleSheets-NOOP" })), "noop");
    }

    TEST_F(GuiPerfFloorTests, Rung_NoopVariableIsOnForAnyValueButZero)
    {
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, "1", nullptr);
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "noop");
        }
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, "yes", nullptr);
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "noop");
        }
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, "0", nullptr);
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "qss");
        }
    }

    TEST_F(GuiPerfFloorTests, Rung_PassthroughFlagIsPassthrough)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--style-passthrough" })), "passthrough");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--STYLE-Passthrough" })), "passthrough");
    }

    TEST_F(GuiPerfFloorTests, Rung_PassthroughVariableIsOnForAnyValueButZero)
    {
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, "1");
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "passthrough");
        }
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, "0");
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "qss");
        }
    }

    TEST_F(GuiPerfFloorTests, Rung_AFlagIsMatchedWholeAndNotInPieces)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--stylesheets-noop-not" })), "qss");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "stylesheets-noop" })), "qss");
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--style-passthrough=1" })), "qss");
    }

    TEST_F(GuiPerfFloorTests, Rung_TheHigherRungWinsWhenSeveralAreAsked)
    {
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--stylesheets-noop", "--style-passthrough" })), "passthrough");
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--disable-stylesheets", "--stylesheets-noop" })), "noop");
        }
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables("1", "1", nullptr);
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "noop");
        }
        {
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, "1", "1");
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({})), "passthrough");
        }
        {
            // a flag and a variable for different rungs: the higher one
            GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, "1");
            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(rungOfArguments({ "--stylesheets-noop" })), "passthrough");
        }
    }

    TEST_F(GuiPerfFloorTests, Rung_AlonePassthroughMeansNoopFlatAndNoStyleSheets)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        // Through the real route: the flag is read by rung(), which StyleManager::stylesheetsDisabled() asks
        startApplication({ "--style-passthrough" });
        EXPECT_TRUE(GuiPerfFloor::isPassthrough());
        EXPECT_TRUE(GuiPerfFloor::inlineSheetsDisabled());
        EXPECT_TRUE(StyleManager::stylesheetsDisabled());
        EXPECT_FALSE(StyleManager::flatStylingActive());       // the styling of the flattened GUI is what the rung takes away
    }

    TEST_F(GuiPerfFloorTests, Rung_AloneNoopMeansFlatAndNoInlineSheets)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        startApplication({ "--stylesheets-noop" });
        EXPECT_FALSE(GuiPerfFloor::isPassthrough());
        EXPECT_TRUE(GuiPerfFloor::inlineSheetsDisabled());
        EXPECT_TRUE(StyleManager::stylesheetsDisabled());
        EXPECT_TRUE(StyleManager::flatStylingActive());        // the flattened GUI styles all the same at noop
    }

    TEST_F(GuiPerfFloorTests, Rung_FlatHasNoStyleSheetsButKeepsTheInlineOnes)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        startApplication({ "--disable-stylesheets" });
        EXPECT_FALSE(GuiPerfFloor::isPassthrough());
        EXPECT_FALSE(GuiPerfFloor::inlineSheetsDisabled());
        EXPECT_TRUE(StyleManager::stylesheetsDisabled());
        EXPECT_TRUE(StyleManager::flatStylingActive());
    }

    TEST_F(GuiPerfFloorTests, Rung_QssHasStyleSheets)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, nullptr, nullptr);

        startApplication();
        EXPECT_FALSE(GuiPerfFloor::isPassthrough());
        EXPECT_FALSE(GuiPerfFloor::inlineSheetsDisabled());
        EXPECT_FALSE(StyleManager::stylesheetsDisabled());
        EXPECT_FALSE(StyleManager::flatStylingActive());
    }

    TEST_F(GuiPerfFloorTests, Rung_IsReadOnceAndKept)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables(nullptr, "1", nullptr);
        startApplication();
        ASSERT_STREQ(GuiPerfFloorTestsInternal::named(GuiPerfFloor::rung()), "noop");

        qunsetenv(GuiPerfFloorTestsInternal::NoopVariable);

        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(GuiPerfFloor::readRung()), "qss");        // the process says qss now
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(GuiPerfFloor::rung()), "noop");           // and the first answer is kept
        EXPECT_TRUE(StyleManager::stylesheetsDisabled());

        GuiPerfFloor::resetForTesting();
        EXPECT_STREQ(GuiPerfFloorTestsInternal::named(GuiPerfFloor::rung()), "qss");            // read again after a reset
    }

    TEST_F(GuiPerfFloorTests, Rung_TheTestSeamAnswersWhateverTheProcessSays)
    {
        GuiPerfFloorTestsInternal::ScopedVariables variables("1", nullptr, nullptr);
        startApplication();

        for (const Rung rung : GuiPerfFloorTestsInternal::AllRungs)
        {
            GuiPerfFloor::setRungForTesting(rung);

            EXPECT_STREQ(GuiPerfFloorTestsInternal::named(GuiPerfFloor::rung()), GuiPerfFloorTestsInternal::named(rung));
            EXPECT_EQ(StyleManager::stylesheetsDisabled(), rung != Rung::Qss) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::inlineSheetsDisabled(), rung == Rung::Noop || rung == Rung::Passthrough)
                << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::isPassthrough(), rung == Rung::Passthrough) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(StyleManager::flatStylingActive(), rung == Rung::Flat || rung == Rung::Noop)
                << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Rung_NamesAreThoseOfTheLines)
    {
        EXPECT_STREQ(GuiPerfFloor::rungName(Rung::Qss), "qss");
        EXPECT_STREQ(GuiPerfFloor::rungName(Rung::Flat), "flat");
        EXPECT_STREQ(GuiPerfFloor::rungName(Rung::Noop), "noop");
        EXPECT_STREQ(GuiPerfFloor::rungName(Rung::Passthrough), "passthrough");
    }

    // ----------------------------------------------------------------------------------
    // T2: the inline sheet helper
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Helper_SetsTheSheetBelowNoopExactlyAsSetStyleSheetDoes)
    {
        for (const Rung rung : { Rung::Qss, Rung::Flat })
        {
            startAt(rung);
            QWidget widget;
            QWidget plain;

            GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);
            plain.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

            EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloorTestsInternal::hasSheetStyle(widget), GuiPerfFloorTestsInternal::hasSheetStyle(plain));
            EXPECT_EQ(GuiPerfFloor::counts().m_helperSkipped, 0u) << GuiPerfFloorTestsInternal::named(rung);

            GuiPerfFloor::setInlineStyleSheet(&widget, QString());      // an empty sheet takes it away, as setStyleSheet does
            EXPECT_TRUE(widget.styleSheet().isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Helper_DoesNothingAtNoopAndPassthroughAndCountsTheCall)
    {
        for (const Rung rung : { Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            QWidget widget;

            GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);
            GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);
            GuiPerfFloor::setInlineStyleSheet(&widget, QStringLiteral("QWidget { color: red; }"));

            EXPECT_TRUE(widget.styleSheet().isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(widget.style(), QApplication::style()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts().m_helperSkipped, 3u) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    // ----------------------------------------------------------------------------------
    // T3: the strip filter (first polish, late, order against the counters, never consumes)
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Strip_ClearsTheSheetBeforeTheFirstPolishAndTheWidgetHasNoSheetStyle)
    {
        for (const Rung rung : { Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            GuiPerfFloor::install(m_application.get());
            QWidget widget;
            widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
            ASSERT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet);
            ASSERT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << "the sheet made a QStyleSheetStyle for the widget";

            widget.ensurePolished();

            EXPECT_TRUE(widget.styleSheet().isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(widget.style(), QApplication::style()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(widget.testAttribute(Qt::WA_StyleSheet)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_TRUE(widget.testAttribute(Qt::WA_WState_Polished)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 1u) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Strip_LeavesAWidgetWithoutASheetAlone)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget widget;

        widget.ensurePolished();

        EXPECT_EQ(widget.style(), QApplication::style());
        EXPECT_EQ(GuiPerfFloor::counts(), GuiPerfFloor::Counts());
    }

    TEST_F(GuiPerfFloorTests, Strip_ClearsEverySheetOfATreeAndEveryWidgetEndsWithTheStyleOfTheApplication)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget root;
        root.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        auto* child = new QWidget(&root);
        auto* grandchild = new QWidget(child);
        grandchild->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        auto* plain = new QWidget(&root);
        auto* stripped = new QWidget(plain);
        stripped->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        root.ensurePolished();      // the widget, then its children (qwidget.cpp:10239)

        for (QWidget* widget : root.findChildren<QWidget*>())
        {
            EXPECT_TRUE(widget->styleSheet().isEmpty()) << widget->metaObject()->className();
            EXPECT_EQ(widget->style(), QApplication::style());
            EXPECT_TRUE(widget->testAttribute(Qt::WA_WState_Polished));
        }
        EXPECT_TRUE(root.styleSheet().isEmpty());
        EXPECT_EQ(root.style(), QApplication::style());
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 3u);
    }

    TEST_F(GuiPerfFloorTests, Strip_AChildPolishedBeforeItsParentStillEndsWithoutASheetStyle)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget parent;
        parent.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        auto* child = new QWidget(&parent);
        ASSERT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(*child)) << "the child follows the sheet style of its parent";

        child->ensurePolished();        // polished first, as ElidingLabel does with itself
        parent.ensurePolished();

        EXPECT_TRUE(parent.styleSheet().isEmpty());
        EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(parent));
        EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(*child));
        EXPECT_EQ(child->style(), QApplication::style());
    }

    TEST_F(GuiPerfFloorTests, Strip_NoPolishReachesTheStyleUnderASheetStyleAndAStrippedWidgetIsPolishedTwiceAndUnpolishedOnce)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget widget;
        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        captureLines();
        GuiPerfCounters::install(m_application.get());

        widget.ensurePolished();

        // The polish inside the strip (setStyle_helper, qwidget.cpp:2693-2696) and the polish of the Polish event that follows it
        // (qwidget.cpp:9108), both through the plain style; the unpolish of the sheet style hands over to the base
        // (qstylesheetstyle.cpp:3006)
        const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
        EXPECT_EQ(counters.m_polish, 2u);
        EXPECT_EQ(counters.m_unpolish, 1u);
        EXPECT_EQ(counters.m_polishSheet, 0u);      // the proof that no polish reached the style under a sheet style
        EXPECT_EQ(counters.m_unpolishSheet, 0u);
    }

    TEST_F(GuiPerfFloorTests, Strip_InstalledBeforeTheCountersTheCountersStillSeeTheInlineSheet)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        GuiPerfCounters::install(m_application.get());      // installed last: asked first (qobject.cpp:2339-2353)
        QWidget widget;
        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        widget.ensurePolished();

        EXPECT_EQ(GuiPerfCounters::counters().m_evPolish, 1u);
        EXPECT_EQ(GuiPerfCounters::counters().m_evPolishOwnSheet, 1u);     // the counts of the counters stay honest
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 1u);                   // and the floor says how many it cleared after
    }

    TEST_F(GuiPerfFloorTests, Strip_InstalledAfterTheCountersHidesTheSheetFromThem)
    {
        // The reason for the order: this is what StyleManager::initialize would give with the two lines swapped
        startAt(Rung::Noop);
        GuiPerfCounters::install(m_application.get());
        GuiPerfFloor::install(m_application.get());         // installed last: asked first, clears the sheet before the counters look
        QWidget widget;
        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        widget.ensurePolished();

        EXPECT_EQ(GuiPerfCounters::counters().m_evPolishOwnSheet, 0u);
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 1u);
    }

    TEST_F(GuiPerfFloorTests, Strip_StyleManagerInstallsTheFloorBeforeTheCounters)
    {
        GuiPerfFloorTestsInternal::ScopedEnvironment counters(GuiPerfFloorTestsInternal::CountersVariable, "1");
        GuiPerfFloor::setRungForTesting(Rung::Noop);
        startApplication();
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());
        ASSERT_TRUE(GuiPerfFloor::isInstalled());
        ASSERT_TRUE(GuiPerfCounters::isInstalled());
        QWidget widget;
        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        widget.ensurePolished();

        EXPECT_EQ(GuiPerfCounters::counters().m_evPolishOwnSheet, 1u);
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 1u);
    }

    TEST_F(GuiPerfFloorTests, Strip_NothingIsInstalledBelowNoopAndTheSheetsStay)
    {
        for (const Rung rung : { Rung::Qss, Rung::Flat })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            GuiPerfFloor::initialize(m_application.get());
            QWidget widget;
            widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

            widget.ensurePolished();

            EXPECT_FALSE(GuiPerfFloor::isInstalled()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(widget)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_TRUE(widget.testAttribute(Qt::WA_StyleSheet)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts(), GuiPerfFloor::Counts()) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Strip_InstallTwiceIsOnceAndTheFilterGoesWithTheApplication)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        const qsizetype children = m_application->children().size();

        GuiPerfFloor::install(m_application.get());

        EXPECT_EQ(m_application->children().size(), children);
        EXPECT_TRUE(GuiPerfFloor::isInstalled());

        m_application.reset();

        EXPECT_FALSE(GuiPerfFloor::isInstalled());
    }

    TEST_F(GuiPerfFloorTests, Strip_TheFilterNeverConsumesAnEvent)
    {
        startAt(Rung::Noop);
        // The probe is installed first, so it is the last filter the application asks: it sees an event only if the filters before it,
        // the floor among them, let it go on
        GuiPerfFloorTestsInternal::EventProbe probe(m_application.get());
        m_application->installEventFilter(&probe);
        GuiPerfFloor::install(m_application.get());
        captureLines();

        {
            QWidget widget;
            widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
            widget.ensurePolished();                                                    // Polish
            widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);                  // StyleChange, of a polished widget with a sheet
            snapshot("probe");                                                          // DynamicPropertyChange, sent to the application
        }

        for (const QEvent::Type type : { QEvent::Polish, QEvent::StyleChange, QEvent::DynamicPropertyChange })
        {
            EXPECT_TRUE(probe.m_seen.contains(static_cast<int>(type))) << "event type " << static_cast<int>(type);
        }
        EXPECT_EQ(m_lines.size(), 1);       // and the floor did take the snapshot
    }

    TEST_F(GuiPerfFloorTests, LateStrip_ClearsASheetSetAfterThePolishOnTheNextTurn)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget widget;
        widget.ensurePolished();

        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        // At once the sheet is in effect: setStyleSheet repolished the polished widget (qwidget.cpp:2603-2622)
        EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet);
        EXPECT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(widget));
        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u);

        QCoreApplication::sendPostedEvents(&widget, 0);

        EXPECT_TRUE(widget.styleSheet().isEmpty());
        EXPECT_FALSE(GuiPerfFloorTestsInternal::hasSheetStyle(widget));
        EXPECT_EQ(widget.style(), QApplication::style());
        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 1u);
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 0u);       // counted separately from the sheets cleared at the first polish
    }

    TEST_F(GuiPerfFloorTests, LateStrip_ASheetSetBeforeThePolishIsLeftToTheFirstPolish)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget widget;

        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        QCoreApplication::sendPostedEvents(&widget, QEvent::MetaCall);

        EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet) << "nothing was queued for a widget that is not polished";
        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u);

        widget.ensurePolished();

        EXPECT_TRUE(widget.styleSheet().isEmpty());
        EXPECT_EQ(GuiPerfFloor::counts().m_stripped, 1u);
        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u);
    }

    TEST_F(GuiPerfFloorTests, LateStrip_AWidgetThatIsGoneBeforeTheTurnIsLeftAlone)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        auto* widget = new QWidget;
        widget->ensurePolished();
        widget->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);        // queues the clearing

        delete widget;
        QCoreApplication::sendPostedEvents();

        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u);
    }

    TEST_F(GuiPerfFloorTests, LateStrip_ASheetThatWasTakenAwayBeforeTheTurnIsNotCounted)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget widget;
        widget.ensurePolished();
        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        widget.setStyleSheet(QString());

        QCoreApplication::sendPostedEvents(&widget, 0);

        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 0u);
    }

    TEST_F(GuiPerfFloorTests, LateStrip_NotBelowNoop)
    {
        startAt(Rung::Flat);
        GuiPerfFloor::initialize(m_application.get());
        QWidget widget;
        widget.ensurePolished();

        widget.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        QCoreApplication::sendPostedEvents(&widget, 0);

        EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet);
        EXPECT_TRUE(GuiPerfFloorTestsInternal::hasSheetStyle(widget));
        EXPECT_EQ(GuiPerfFloor::counts(), GuiPerfFloor::Counts());
    }

    // ----------------------------------------------------------------------------------
    // T4: the sites that make a sheet for every widget they build (Card, VectorInput) and the work that passthrough leaves out
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Card_TheHeaderSheetIsMadeBelowNoopAndLeftOutAtNoopAndPassthrough)
    {
        for (const Rung rung : GuiPerfFloorTestsInternal::AllRungs)
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            Card card;

            const bool sheet = rung == Rung::Qss || rung == Rung::Flat;
            EXPECT_EQ(card.header()->styleSheet().isEmpty(), !sheet) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloorTestsInternal::hasSheetStyle(*card.header()), sheet) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts().m_helperSkipped, sheet ? 0u : 1u) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Card_TheHeaderSheetIsTheTooltipPaddingRule)
    {
        startAt(Rung::Flat);
        Card card;

        EXPECT_TRUE(card.header()->styleSheet().startsWith(QStringLiteral("QToolTip { padding: ")));
    }

    TEST_F(GuiPerfFloorTests, VectorInput_TheLabelStyleIsSetBelowNoopAndLeftOutAtNoopAndPassthrough)
    {
        for (const Rung rung : GuiPerfFloorTestsInternal::AllRungs)
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            QWidget host;
            auto* vector = new VectorInput(&host, 3);
            const QString qss = QStringLiteral("QLabel { color: red; }");

            vector->setLabelStyle(1, qss);

            QLabel* label = vector->findChildren<QLabel*>().at(1);
            const bool sheet = rung == Rung::Qss || rung == Rung::Flat;
            EXPECT_EQ(label->styleSheet(), sheet ? qss : QString()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts().m_helperSkipped, sheet ? 0u : 1u) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Card_PaintsItsBodyFlatAtFlatAndNoopAndNotAtPassthrough)
    {
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            Card card;
            card.setTitle(QStringLiteral("Card"));
            card.setContentWidget(new QLabel(QStringLiteral("content"), &card));
            card.resize(300, 200);
            card.setAttribute(Qt::WA_DontShowOnScreen);
            card.show();
            QApplication::processEvents();
            QImage image(card.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0xFF, 0x00, 0xFF));

            QPainter painter(&image);
            card.render(&painter, QPoint(), QRegion(), QWidget::RenderFlags());     // the card only, not its children
            painter.end();

            const QPoint bodyCenter = card.findChild<QFrame*>(QStringLiteral("contentContainer"))->geometry().center();
            if (rung == Rung::Passthrough)
            {
                EXPECT_EQ(image.pixelColor(bodyCenter), QColor(0xFF, 0x00, 0xFF)) << "the frame as the style draws it paints no body";
            }
            else
            {
                EXPECT_EQ(image.pixelColor(bodyCenter), QColor(0x55, 0x55, 0x55)) << GuiPerfFloorTestsInternal::named(rung);
            }
        }
    }

    TEST_F(GuiPerfFloorTests, CardHeader_PaintsItsFillFlatAtFlatAndNoopAndNotAtPassthrough)
    {
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            Card card;
            card.setTitle(QStringLiteral("Card"));
            card.resize(300, 200);
            card.setAttribute(Qt::WA_DontShowOnScreen);
            card.show();
            QApplication::processEvents();
            CardHeader* header = card.header();
            QImage image(header->size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0xFF, 0x00, 0xFF));

            QPainter painter(&image);
            header->render(&painter, QPoint(), QRegion(), QWidget::RenderFlags());  // the header only, not its children
            painter.end();

            const QPoint inside(header->width() / 2, header->height() / 2);         // inside the fill, whatever the children draw there
            if (rung == Rung::Passthrough)
            {
                EXPECT_EQ(image.pixelColor(inside), QColor(0xFF, 0x00, 0xFF));
            }
            else
            {
                EXPECT_EQ(image.pixelColor(inside), QColor(0x33, 0x33, 0x33)) << GuiPerfFloorTestsInternal::named(rung);
            }
        }
    }

    TEST_F(GuiPerfFloorTests, BrowseEdit_KeepsTheSizesAndFrameOfTheSheetAtFlatAndNoopAndNotAtPassthrough)
    {
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            QWidget host;
            auto* browseEdit = new BrowseEdit(&host);
            QPushButton* button = browseEdit->findChild<QPushButton*>(QStringLiteral("attached-button"));
            QLineEdit* lineEdit = browseEdit->findChild<QLineEdit*>(QStringLiteral("line-edit"));
            ASSERT_TRUE(button && lineEdit);

            const bool flat = rung != Rung::Passthrough;
            EXPECT_EQ(button->isFlat(), flat) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(button->maximumSize() == QSize(16, 16), flat) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(lineEdit->hasFrame(), !flat) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, RepolishHelper_ARepolishIsDoneAtFlatAndNoopAndLeftOutAtPassthrough)
    {
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            GuiPerfFloor::setRungForTesting(rung);
            startApplication();
            StyleManager styleManager(nullptr);
            styleManager.initialize(m_application.get(), AZ::IO::PathView());
            GuiPerfCounters::captureLinesInto(&m_lines);
            GuiPerfCounters::install(m_application.get());
            Card card;
            card.ensurePolished();
            const GuiPerfCounters::Counters before = GuiPerfCounters::counters();

            card.setSelected(true);

            const GuiPerfCounters::Counters after = GuiPerfCounters::counters();
            if (rung == Rung::Passthrough)
            {
                EXPECT_EQ(after.m_repolishHelper, before.m_repolishHelper);
                EXPECT_EQ(after.m_polish, before.m_polish);
                EXPECT_EQ(after.m_unpolish, before.m_unpolish);
            }
            else
            {
                EXPECT_EQ(after.m_repolishHelper - before.m_repolishHelper, 1u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_GT(after.m_polish, before.m_polish) << GuiPerfFloorTestsInternal::named(rung);
            }
        }
    }

    TEST_F(GuiPerfFloorTests, ClassTags_ArePolishedAgainAtFlatAndNoopAndNotAtPassthrough)
    {
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            QWidget widget;
            widget.ensurePolished();
            GuiPerfCounters::install(m_application.get());

            Style::addClass(&widget, QStringLiteral("alpha"));
            Style::addClass(&widget, QStringLiteral("beta"));
            Style::removeClass(&widget, QStringLiteral("alpha"));

            // The tags are set whatever the rung, the polish that follows each change is not
            EXPECT_FALSE(Style::hasClass(&widget, QStringLiteral("alpha"))) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_TRUE(Style::hasClass(&widget, QStringLiteral("beta"))) << GuiPerfFloorTestsInternal::named(rung);
            const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
            EXPECT_EQ(counters.m_polish, rung == Rung::Passthrough ? 0u : 3u) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(counters.m_unpolish, rung == Rung::Passthrough ? 0u : 3u) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    // ----------------------------------------------------------------------------------
    // T5: passthrough: every override of Style hands the call to its base, and Style stays the style of the application
    // ----------------------------------------------------------------------------------

    namespace GuiPerfFloorTestsInternal
    {
        //! The widgets the answers of the two styles are compared on
        struct StyleSample
        {
            QPushButton m_button{ QStringLiteral("Text") };
            QCheckBox m_check{ QStringLiteral("Check") };
            QRadioButton m_radio{ QStringLiteral("Radio") };
            QScrollBar m_scrollBar;
            QFrame m_frame;
            QSpinBox m_spin;
            QComboBox m_combo;
            QTabBar m_tabBar;
            QToolBar m_toolBar;

            QList<QWidget*> widgets()
            {
                return { &m_button, &m_check, &m_radio, &m_scrollBar, &m_frame, &m_spin, &m_combo, &m_tabBar, &m_toolBar };
            }
        };

        const QStyle::PixelMetric SampleMetrics[] = {
            QStyle::PM_IndicatorWidth,      QStyle::PM_IndicatorHeight, QStyle::PM_ExclusiveIndicatorWidth, QStyle::PM_ScrollBarExtent,
            QStyle::PM_ScrollBarSliderMin,  QStyle::PM_SplitterWidth,   QStyle::PM_MenuHMargin,             QStyle::PM_SliderThickness,
            QStyle::PM_ButtonMargin,        QStyle::PM_DefaultFrameWidth, QStyle::PM_TabBarTabHSpace,       QStyle::PM_ToolBarIconSize,
        };

        const QStyle::StyleHint SampleHints[] = {
            QStyle::SH_SpinBox_StepModifier,
            QStyle::SH_Menu_SubMenuPopupDelay,
            QStyle::SH_ComboBox_Popup,
            QStyle::SH_ComboBox_UseNativePopup,
            QStyle::SH_Slider_AbsoluteSetButtons,
            QStyle::SH_ComboBox_PopupFrameStyle,
        };

        int differingMetrics(const QStyle& first, const QStyle& second, StyleSample& sample)
        {
            int differing = 0;
            for (QWidget* widget : sample.widgets())
            {
                for (const QStyle::PixelMetric metric : SampleMetrics)
                {
                    differing += first.pixelMetric(metric, nullptr, widget) != second.pixelMetric(metric, nullptr, widget) ? 1 : 0;
                }
            }
            return differing;
        }

        int differingHints(const QStyle& first, const QStyle& second, StyleSample& sample)
        {
            int differing = 0;
            for (QWidget* widget : sample.widgets())
            {
                for (const QStyle::StyleHint hint : SampleHints)
                {
                    const int firstAnswer = first.styleHint(hint, nullptr, widget, nullptr);
                    differing += firstAnswer != second.styleHint(hint, nullptr, widget, nullptr) ? 1 : 0;
                }
            }
            return differing;
        }

        QStyleOptionButton buttonOption(const QAbstractButton& button, const QRect& rect)
        {
            QStyleOptionButton option;
            option.initFrom(&button);
            option.text = button.text();
            option.rect = rect;
            return option;
        }

        QStyleOptionSpinBox spinOption(const QSpinBox& spin)
        {
            QStyleOptionSpinBox option;
            option.initFrom(&spin);
            option.buttonSymbols = QAbstractSpinBox::UpDownArrows;
            option.subControls = QStyle::SC_All;
            option.frame = true;
            return option;
        }

        QImage greyImage(const QSize& size)
        {
            QImage image(size, QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor(0x80, 0x80, 0x80));
            return image;
        }

        QImage drawnControl(const QStyle& style, QStyle::ControlElement element, const QStyleOption& option, const QWidget& widget)
        {
            QImage image = greyImage(option.rect.size());
            QPainter painter(&image);
            style.drawControl(element, &option, &painter, &widget);
            painter.end();
            return image;
        }

        QImage drawnPrimitive(const QStyle& style, QStyle::PrimitiveElement element, const QStyleOption& option, const QWidget& widget)
        {
            QImage image = greyImage(option.rect.size());
            QPainter painter(&image);
            style.drawPrimitive(element, &option, &painter, &widget);
            painter.end();
            return image;
        }

        QImage drawnSpinBox(const QStyle& style, const QSpinBox& spin)
        {
            QImage image = greyImage(spin.size());
            QPainter painter(&image);
            const QStyleOptionSpinBox option = spinOption(spin);
            style.drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
            painter.end();
            return image;
        }
    } // namespace GuiPerfFloorTestsInternal

    TEST_F(GuiPerfFloorTests, Passthrough_PixelMetricsAreThoseOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        GuiPerfFloorTestsInternal::StyleSample sample;

        // the control: the flattened Style answers otherwise, or the equality below proves nothing
        EXPECT_GE(GuiPerfFloorTestsInternal::differingMetrics(style, *fusion, sample), 20);

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(GuiPerfFloorTestsInternal::differingMetrics(style, *fusion, sample), 0);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_StyleHintsAreThoseOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        GuiPerfFloorTestsInternal::StyleSample sample;

        EXPECT_GE(GuiPerfFloorTestsInternal::differingHints(style, *fusion, sample), 9);

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(GuiPerfFloorTestsInternal::differingHints(style, *fusion, sample), 0);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_SizeFromContentsIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QPushButton button(QStringLiteral("Text"));
        const QStyleOptionButton option = GuiPerfFloorTestsInternal::buttonOption(button, QRect(0, 0, 80, 22));

        EXPECT_NE(style.sizeFromContents(QStyle::CT_PushButton, &option, QSize(40, 14), &button),
                  fusion->sizeFromContents(QStyle::CT_PushButton, &option, QSize(40, 14), &button));       // the control

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(style.sizeFromContents(QStyle::CT_PushButton, &option, QSize(40, 14), &button),
                  fusion->sizeFromContents(QStyle::CT_PushButton, &option, QSize(40, 14), &button));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_SubControlRectIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QSpinBox spin;
        const QStyleOptionSpinBox option = GuiPerfFloorTestsInternal::spinOption(spin);

        EXPECT_NE(style.subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, &spin),
                  fusion->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, &spin));       // the control

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        for (const QStyle::SubControl subControl :
             { QStyle::SC_SpinBoxUp, QStyle::SC_SpinBoxDown, QStyle::SC_SpinBoxEditField, QStyle::SC_SpinBoxFrame })
        {
            EXPECT_EQ(
                style.subControlRect(QStyle::CC_SpinBox, &option, subControl, &spin),
                fusion->subControlRect(QStyle::CC_SpinBox, &option, subControl, &spin))
                << "sub control " << static_cast<int>(subControl);
        }
    }

    TEST_F(GuiPerfFloorTests, Passthrough_SubElementRectIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QLineEdit lineEdit;
        lineEdit.resize(100, 20);
        QStyleOptionFrame option;
        option.initFrom(&lineEdit);
        option.rect = lineEdit.rect();
        option.lineWidth = 1;

        // the control: the flattened Style lays the contents of a line edit out by the box of the sheet. (The rect of a check box would
        // not do: Fusion lays it out through the pixel metrics of the style, which the passthrough hands on by itself.)
        EXPECT_NE(
            style.subElementRect(QStyle::SE_LineEditContents, &option, &lineEdit),
            fusion->subElementRect(QStyle::SE_LineEditContents, &option, &lineEdit));

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        for (const QStyle::SubElement element : { QStyle::SE_LineEditContents, QStyle::SE_FrameContents, QStyle::SE_ShapedFrameContents })
        {
            EXPECT_EQ(style.subElementRect(element, &option, &lineEdit), fusion->subElementRect(element, &option, &lineEdit))
                << "sub element " << static_cast<int>(element);
        }
    }

    TEST_F(GuiPerfFloorTests, Passthrough_DrawControlIsThatOfTheBaseStyle)
    {
        registerGlobalElements();
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QCheckBox check(QStringLiteral("Check"));
        check.resize(100, 20);
        style.polish(&check);
        QStyleOptionButton option = GuiPerfFloorTestsInternal::buttonOption(check, check.rect());
        option.state |= QStyle::State_On;

        EXPECT_NE(GuiPerfFloorTestsInternal::drawnControl(style, QStyle::CE_CheckBox, option, check),
                  GuiPerfFloorTestsInternal::drawnControl(*fusion, QStyle::CE_CheckBox, option, check));      // the control

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(GuiPerfFloorTestsInternal::drawnControl(style, QStyle::CE_CheckBox, option, check),
                  GuiPerfFloorTestsInternal::drawnControl(*fusion, QStyle::CE_CheckBox, option, check));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_DrawPrimitiveIsThatOfTheBaseStyle)
    {
        registerGlobalElements();
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QWidget widget;
        QStyleOption handle;
        handle.initFrom(&widget);
        handle.rect = QRect(0, 0, 40, 6);

        // the control: the flattened Style fills the resize handle of a dock widget
        EXPECT_NE(
            GuiPerfFloorTestsInternal::drawnPrimitive(style, QStyle::PE_IndicatorDockWidgetResizeHandle, handle, widget),
            GuiPerfFloorTestsInternal::drawnPrimitive(*fusion, QStyle::PE_IndicatorDockWidgetResizeHandle, handle, widget));

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(GuiPerfFloorTestsInternal::drawnPrimitive(style, QStyle::PE_IndicatorDockWidgetResizeHandle, handle, widget),
                  GuiPerfFloorTestsInternal::drawnPrimitive(*fusion, QStyle::PE_IndicatorDockWidgetResizeHandle, handle, widget));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_DrawComplexControlIsThatOfTheBaseStyle)
    {
        registerGlobalElements();
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QSpinBox spin;
        spin.resize(80, 22);
        style.polish(&spin);

        // the control: the flattened Style draws a spin box of its own
        EXPECT_NE(GuiPerfFloorTestsInternal::drawnSpinBox(style, spin), GuiPerfFloorTestsInternal::drawnSpinBox(*fusion, spin));

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(GuiPerfFloorTestsInternal::drawnSpinBox(style, spin), GuiPerfFloorTestsInternal::drawnSpinBox(*fusion, spin));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_StandardPaletteIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        style.polish(m_application.get());     // the flattened Style keeps the palette it builds there as its standard palette

        EXPECT_NE(style.standardPalette().color(QPalette::Window), fusion->standardPalette().color(QPalette::Window));       // the control

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(style.standardPalette(), fusion->standardPalette());
    }

    TEST_F(GuiPerfFloorTests, Passthrough_StandardIconIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QWidget widget;
        const QImage baseGlyph = fusion->standardIcon(QStyle::SP_TitleBarCloseButton, nullptr, &widget).pixmap(16, 16).toImage();

        // the control: the flattened Style has a glyph of its own for the close button of a title bar
        EXPECT_NE(style.standardIcon(QStyle::SP_TitleBarCloseButton, nullptr, &widget).pixmap(16, 16).toImage(), baseGlyph);

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(style.standardIcon(QStyle::SP_TitleBarCloseButton, nullptr, &widget).pixmap(16, 16).toImage(), baseGlyph);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_GeneratedIconPixmapIsThatOfTheBaseStyle)
    {
        startAt(Rung::Flat);
        std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        QPixmap pixmap(8, 8);
        pixmap.fill(QColor(0x20, 0x40, 0x60));
        const QStyleOption option;

        // the control: the flattened Style tints a disabled icon with the colour of its push button configuration
        EXPECT_NE(
            style.generatedIconPixmap(QIcon::Disabled, pixmap, &option).toImage(),
            fusion->generatedIconPixmap(QIcon::Disabled, pixmap, &option).toImage());

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        EXPECT_EQ(
            style.generatedIconPixmap(QIcon::Disabled, pixmap, &option).toImage(),
            fusion->generatedIconPixmap(QIcon::Disabled, pixmap, &option).toImage());
    }

    TEST_F(GuiPerfFloorTests, Passthrough_PolishOfAWidgetSetsNoneOfOurElementsPalettesOrLimits)
    {
        registerGlobalElements();

        struct Polished
        {
            int m_progressMaximumHeight;
            bool m_headerPaletteChanged;
            int m_tableFrameShape;
            bool m_tableShowGrid;
            QStringList m_elementProperties;
        };

        const auto polishAt = [this](Rung rung)
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            Style style(QStyleFactory::create(QStringLiteral("Fusion")));
            QProgressBar progress;
            QHeaderView header(Qt::Horizontal);
            QTableView table;
            QComboBox combo;
            QFrame frame;
            const QPalette headerBefore = header.palette();

            style.polish(&progress);
            style.polish(&header);
            style.polish(&table);
            style.polish(&combo);
            style.polish(&frame);

            Polished result{ progress.maximumHeight(), header.palette() != headerBefore, static_cast<int>(table.frameShape()),
                             table.showGrid(), {} };
            const QList<const QWidget*> polished = { &progress, &header, &table, &combo, &frame };
            for (const QWidget* widget : polished)
            {
                for (const QByteArray& name : widget->dynamicPropertyNames())
                {
                    if (name.startsWith("_o3de_"))
                    {
                        result.m_elementProperties.append(QString::fromLatin1(name));
                    }
                }
            }
            return result;
        };

        // the control: what the flattened Style sets
        const Polished flat = polishAt(Rung::Flat);
        EXPECT_EQ(flat.m_progressMaximumHeight, 5);
        EXPECT_TRUE(flat.m_headerPaletteChanged);
        EXPECT_EQ(flat.m_tableFrameShape, static_cast<int>(QFrame::NoFrame));
        EXPECT_FALSE(flat.m_tableShowGrid);
        EXPECT_GE(flat.m_elementProperties.size(), 4);

        const Polished passthrough = polishAt(Rung::Passthrough);

        EXPECT_EQ(passthrough.m_progressMaximumHeight, QWIDGETSIZE_MAX);
        EXPECT_FALSE(passthrough.m_headerPaletteChanged);
        EXPECT_EQ(passthrough.m_tableFrameShape, static_cast<int>(QFrame::StyledPanel));
        EXPECT_TRUE(passthrough.m_tableShowGrid);
        EXPECT_TRUE(passthrough.m_elementProperties.isEmpty())
            << passthrough.m_elementProperties.join(QLatin1Char(' ')).toUtf8().constData();
    }

    TEST_F(GuiPerfFloorTests, Passthrough_UnpolishOfAWidgetUndoesNothingOfOurs)
    {
        startAt(Rung::Flat);
        Style style(QStyleFactory::create(QStringLiteral("Fusion")));
        Card card;
        card.layout()->setSpacing(7);

        style.unpolish(&card);

        EXPECT_EQ(card.layout()->spacing(), 3);         // the control: the flattened Style puts the spacing of a card back (Card::unpolish)

        card.layout()->setSpacing(7);
        GuiPerfFloor::setRungForTesting(Rung::Passthrough);

        style.unpolish(&card);

        EXPECT_EQ(card.layout()->spacing(), 7);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_PolishAndUnpolishAreStillCountedByTheCounters)
    {
        startAt(Rung::Passthrough);
        GuiPerfCounters::install(m_application.get());
        QWidget widget;
        widget.ensurePolished();
        const GuiPerfCounters::Counters before = GuiPerfCounters::counters();

        widget.style()->unpolish(&widget);
        widget.style()->polish(&widget);

        const GuiPerfCounters::Counters after = GuiPerfCounters::counters();
        EXPECT_EQ(after.m_unpolish - before.m_unpolish, 1u);        // the counters see the calls of a style that hands them on
        EXPECT_EQ(after.m_polish - before.m_polish, 1u);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_PolishOfAPaletteIsLeftAsTheBaseStyleLeavesIt)
    {
        startAt(Rung::Flat);
        QPalette flatPalette;
        flatPalette.setColor(QPalette::Window, QColor(0x12, 0x34, 0x56));
        QApplication::style()->polish(flatPalette);
        // the control: the palette of O3DE replaces any palette
        EXPECT_EQ(flatPalette.color(QPalette::Window), QColor(0x44, 0x44, 0x44));

        GuiPerfFloor::setRungForTesting(Rung::Passthrough);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(0x12, 0x34, 0x56));
        QApplication::style()->polish(palette);

        EXPECT_EQ(palette.color(QPalette::Window), QColor(0x12, 0x34, 0x56));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_ApplicationPolishLeavesOutThePaletteAndTheClassFontsAndKeepsTheApplicationFont)
    {
        GuiPerfFloor::setRungForTesting(Rung::Passthrough);
        startApplication();
        QFont applicationFont(QStringLiteral("Open Sans"));
        applicationFont.setPixelSize(17);
        QApplication::setFont(applicationFont);
        QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        const QPalette reference = QApplication::palette();
        const QFont referenceMenuBar = QApplication::font("QMenuBar");

        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

        EXPECT_EQ(QApplication::palette(), reference);                  // the palette the base style gave the application
        EXPECT_EQ(QApplication::font("QMenuBar"), referenceMenuBar);   // no class font of ours
        EXPECT_EQ(QApplication::font().pixelSize(), 17);                // and the application font StyleManager set before the style
        EXPECT_NE(QApplication::palette().color(QPalette::Window), QColor(0x44, 0x44, 0x44));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_ApplicationPolishAtFlatSetsThePaletteAndTheClassFontsTheControl)
    {
        GuiPerfFloor::setRungForTesting(Rung::Flat);
        startApplication();
        QFont applicationFont(QStringLiteral("Open Sans"));
        applicationFont.setPixelSize(17);
        QApplication::setFont(applicationFont);
        QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        const QFont referenceMenuBar = QApplication::font("QMenuBar");

        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

        EXPECT_EQ(QApplication::palette().color(QPalette::Window), QColor(0x44, 0x44, 0x44));
        EXPECT_NE(QApplication::font("QMenuBar"), referenceMenuBar);
    }

    TEST_F(GuiPerfFloorTests, Passthrough_ApplicationPolishInstallsNoTooltipFilter)
    {
        GuiPerfFloor::setRungForTesting(Rung::Passthrough);
        startApplication();
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        GuiPerfFloor::setRungForTesting(Rung::Flat);      // had the style installed itself, this is where its filter would act again
        QWidget widget;

        widget.setToolTip(QStringLiteral("plain text"));

        EXPECT_EQ(widget.toolTip(), QStringLiteral("plain text"));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_TheStyleFilterHandsOnWhenItIsAskedAtPassthrough)
    {
        GuiPerfFloor::setRungForTesting(Rung::Flat);
        startApplication();
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        QWidget wrapped;
        wrapped.setToolTip(QStringLiteral("plain text"));
        ASSERT_EQ(wrapped.toolTip(), QStringLiteral("<b></b>plain text"));          // the control: the filter wraps a tooltip at flat
        GuiPerfFloor::setRungForTesting(Rung::Passthrough);
        QWidget widget;

        widget.setToolTip(QStringLiteral("plain text"));

        EXPECT_EQ(widget.toolTip(), QStringLiteral("plain text"));
    }

    TEST_F(GuiPerfFloorTests, Passthrough_StyleStaysTheApplicationStyleAndItsCastsKeepWorking)
    {
        GuiPerfFloor::setRungForTesting(Rung::Passthrough);
        startApplication();
        StyleManager styleManager(nullptr);
        styleManager.initialize(m_application.get(), AZ::IO::PathView());
        QWidget widget;
        widget.ensurePolished();

        // EntityPropertyEditor.cpp:371 and PropertiesContainer.cpp:94 cast the style of the widget unchecked
        auto* style = qobject_cast<Style*>(QApplication::style());
        ASSERT_NE(style, nullptr);
        EXPECT_EQ(qobject_cast<Style*>(widget.style()), style);
        QImage image(20, 10, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        QStyleOption option;
        option.rect = QRect(0, 0, 20, 10);
        static_cast<Style*>(widget.style())->drawDragIndicator(&option, &painter, &widget);
        painter.end();
        EXPECT_NE(image.pixelColor(10, 5).alpha(), 0);
    }

    // ----------------------------------------------------------------------------------
    // T6: what the floor reports
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Lines_StartupLineCarriesTheRungAndWhetherTheStyleSheetsAreDisabled)
    {
        EXPECT_EQ(GuiPerfFloor::formatStartupLine(Rung::Qss, false), QStringLiteral("GuiPerfFloor: v=1 rung=qss stylesheets_disabled=0"));
        EXPECT_EQ(GuiPerfFloor::formatStartupLine(Rung::Flat, true), QStringLiteral("GuiPerfFloor: v=1 rung=flat stylesheets_disabled=1"));
        EXPECT_EQ(GuiPerfFloor::formatStartupLine(Rung::Noop, true), QStringLiteral("GuiPerfFloor: v=1 rung=noop stylesheets_disabled=1"));
        EXPECT_EQ(
            GuiPerfFloor::formatStartupLine(Rung::Passthrough, true),
            QStringLiteral("GuiPerfFloor: v=1 rung=passthrough stylesheets_disabled=1"));
    }

    TEST_F(GuiPerfFloorTests, Lines_SnapshotLineCarriesTheKeysInTheOrderOfTheSpecification)
    {
        GuiPerfFloor::Counts values;
        values.m_stripped = 11;
        values.m_strippedLate = 12;
        values.m_helperSkipped = 13;

        const QString line = GuiPerfFloor::formatSnapshotLine(QStringLiteral("S-OC:noop:3:post"), Rung::Noop, values);

        EXPECT_EQ(line, QStringLiteral("GuiPerfFloor: v=1 tag=S-OC:noop:3:post rung=noop stripped=11 stripped_late=12 helper_skipped=13"));
        const QStringList keys = { QStringLiteral("rung"), QStringLiteral("stripped"), QStringLiteral("stripped_late"),
                                   QStringLiteral("helper_skipped") };
        EXPECT_EQ(GuiPerfFloorTestsInternal::keysAfterTag(line), keys);
    }

    TEST_F(GuiPerfFloorTests, Startup_AllRungsWriteOneLineAndSetTheRungProperty)
    {
        for (const Rung rung : GuiPerfFloorTestsInternal::AllRungs)
        {
            GuiPerfFloor::resetForTesting();
            m_lines.clear();
            startAt(rung);
            captureLines();

            GuiPerfFloor::initialize(m_application.get());

            ASSERT_EQ(m_lines.size(), 1) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(m_lines[0], GuiPerfFloor::formatStartupLine(rung, rung != Rung::Qss)) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(
                m_application->property(GuiPerfFloorTestsInternal::RungProperty).toString(),
                QString::fromLatin1(GuiPerfFloorTestsInternal::named(rung)));
            EXPECT_EQ(GuiPerfFloor::isInstalled(), rung == Rung::Noop || rung == Rung::Passthrough)
                << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Startup_TheLineIsWrittenOnceMoreOnTheFirstTurnOfTheEventLoop)
    {
        // The Editor opens Editor.log after it makes its QApplication: the second line is the one that reaches it
        startAt(Rung::Flat);
        captureLines();

        GuiPerfFloor::initialize(m_application.get());
        ASSERT_EQ(m_lines.size(), 1);

        QCoreApplication::sendPostedEvents(m_application.get(), 0);

        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_EQ(m_lines[1], m_lines[0]);
    }

    TEST_F(GuiPerfFloorTests, Startup_StyleManagerInitializeRunsTheFloor)
    {
        GuiPerfFloor::setRungForTesting(Rung::Noop);
        startApplication();
        captureLines();
        StyleManager styleManager(nullptr);

        styleManager.initialize(m_application.get(), AZ::IO::PathView());

        ASSERT_GE(m_lines.size(), 1);
        // the rung, and what StyleManager::stylesheetsDisabled() answers for it
        EXPECT_EQ(m_lines[0], GuiPerfFloor::formatStartupLine(Rung::Noop, true));
        EXPECT_TRUE(GuiPerfFloor::isInstalled());
        EXPECT_TRUE(qobject_cast<Style*>(QApplication::style()));
    }

    TEST_F(GuiPerfFloorTests, Snapshot_NoopWritesOneLineInsideTheSetPropertyCallWithTheCountsSoFar)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        captureLines();
        QWidget widget;
        GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);        // skipped
        GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);        // skipped
        QWidget stripped;
        stripped.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        stripped.ensurePolished();                                                              // stripped
        QWidget late;
        late.ensurePolished();
        late.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
        QCoreApplication::sendPostedEvents(&late, 0);                                           // stripped late

        snapshot("S-OC:noop:3:post");      // the line is there when setProperty returns

        ASSERT_EQ(m_lines.size(), 1);
        EXPECT_EQ(
            m_lines[0], QStringLiteral("GuiPerfFloor: v=1 tag=S-OC:noop:3:post rung=noop stripped=1 stripped_late=1 helper_skipped=2"));
    }

    TEST_F(GuiPerfFloorTests, Snapshot_PassthroughNamesItsRung)
    {
        startAt(Rung::Passthrough);
        GuiPerfFloor::install(m_application.get());
        captureLines();

        snapshot("p");

        ASSERT_EQ(m_lines.size(), 1);
        EXPECT_EQ(GuiPerfFloorTestsInternal::valueOf(m_lines[0], "rung"), QStringLiteral("passthrough"));
    }

    TEST_F(GuiPerfFloorTests, Snapshot_TheLinesAreCumulativeAndNeverReset)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        captureLines();
        QWidget widget;

        GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);
        snapshot("first");
        GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);
        snapshot("second");

        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_EQ(GuiPerfFloorTestsInternal::valueOf(m_lines[0], "helper_skipped"), QStringLiteral("1"));
        EXPECT_EQ(GuiPerfFloorTestsInternal::valueOf(m_lines[1], "helper_skipped"), QStringLiteral("2"));
    }

    TEST_F(GuiPerfFloorTests, Snapshot_TheCountersLineComesFirstThenTheFloorLineAndBothCarryTheTag)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        GuiPerfCounters::install(m_application.get());      // after the floor, as StyleManager::initialize does: asked first
        captureLines();

        snapshot("pair");

        ASSERT_EQ(m_lines.size(), 2);
        EXPECT_TRUE(m_lines[0].startsWith(QStringLiteral("GuiPerfCounters: v=1 tag=pair ")));
        EXPECT_TRUE(m_lines[1].startsWith(QStringLiteral("GuiPerfFloor: v=1 tag=pair ")));
    }

    TEST_F(GuiPerfFloorTests, Snapshot_NothingForTheFlatAndQssRungsThatHaveNoFilter)
    {
        for (const Rung rung : { Rung::Qss, Rung::Flat })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            GuiPerfFloor::initialize(m_application.get());
            m_lines.clear();
            captureLines();

            snapshot("none");

            EXPECT_TRUE(m_lines.isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    TEST_F(GuiPerfFloorTests, Snapshot_NothingForAnEmptyTagOrAWidgetThatCarriesTheProperty)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        captureLines();
        QWidget widget;

        snapshot("");
        // the application is asked, nobody else
        widget.setProperty(GuiPerfFloorTestsInternal::SnapshotProperty, QStringLiteral("tag"));

        EXPECT_TRUE(m_lines.isEmpty());
    }

    TEST_F(GuiPerfFloorTests, Snapshot_AnotherPropertyOfTheApplicationWritesNothing)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        captureLines();
        snapshot("first");
        ASSERT_EQ(m_lines.size(), 1);

        // The tag is still "first": only the snapshot property asks
        m_application->setProperty("_o3de_something_else", QStringLiteral("x"));

        EXPECT_EQ(m_lines.size(), 1);
    }

    TEST_F(GuiPerfFloorTests, Snapshot_TheSameTagTwiceInARowWritesOnce)
    {
        // Qt sends no property change for an equal value (qobject.cpp:4444-4446): the tags of the kit carry an index for that reason
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        captureLines();

        snapshot("same");
        snapshot("same");
        snapshot("other");
        snapshot("same");

        EXPECT_EQ(m_lines.size(), 3);
    }

    TEST_F(GuiPerfFloorTests, Snapshot_ReachesTheLogWhenNothingCapturesIt)
    {
#if defined(AZ_ENABLE_TRACING)
        BusConnect();
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());

        snapshot("log");

        ASSERT_EQ(m_logged.size(), 1);
        EXPECT_TRUE(m_logged[0].startsWith(QStringLiteral("GuiPerfFloor: v=1 tag=log rung=noop ")));
        EXPECT_TRUE(m_logged[0].endsWith(QLatin1Char('\n')));
#endif // defined(AZ_ENABLE_TRACING)
    }

    // ----------------------------------------------------------------------------------
    // T7: off changes nothing (qss and flat)
    // ----------------------------------------------------------------------------------

    TEST_F(GuiPerfFloorTests, Off_QssAndFlatStyleAnswerOtherwiseThanTheBaseStyleAndKeepTheirSheets)
    {
        for (const Rung rung : { Rung::Qss, Rung::Flat })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
            GuiPerfFloorTestsInternal::StyleSample sample;
            QWidget widget;
            GuiPerfFloor::setInlineStyleSheet(&widget, GuiPerfFloorTestsInternal::OwnSheet);

            // (the flattened metrics need the flattened GUI: at qss fewer answers differ)
            EXPECT_GE(GuiPerfFloorTestsInternal::differingMetrics(*QApplication::style(), *fusion, sample), rung == Rung::Flat ? 20 : 5)
                << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(widget.styleSheet(), GuiPerfFloorTestsInternal::OwnSheet) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(GuiPerfFloor::inlineSheetsDisabled()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_FALSE(GuiPerfFloor::isPassthrough()) << GuiPerfFloorTestsInternal::named(rung);
            EXPECT_EQ(GuiPerfFloor::counts(), GuiPerfFloor::Counts()) << GuiPerfFloorTestsInternal::named(rung);
        }
    }

    // ----------------------------------------------------------------------------------
    // T3b: the strip on the classes the Editor builds (real widgets, the real Style: no stand-in for a Card or a vector control)
    // ----------------------------------------------------------------------------------

    namespace GuiPerfFloorTestsInternal
    {
        using WidgetFactory = QWidget* (*)(QWidget* parent);

        struct NamedFactory
        {
            const char* m_name;
            WidgetFactory m_make;
        };

        template<typename T>
        QWidget* makeWidget(QWidget* parent)
        {
            return new T(parent);
        }

        //! A widget of each class the inspector and the outliner build in numbers, the classes of Qt and those of AzQtComponents
        const NamedFactory WidgetClasses[] = {
            { "QWidget", &makeWidget<QWidget> },
            { "QFrame", &makeWidget<QFrame> },
            { "QLabel", &makeWidget<QLabel> },
            { "QPushButton", &makeWidget<QPushButton> },
            { "QToolButton", &makeWidget<QToolButton> },
            { "QCheckBox", &makeWidget<QCheckBox> },
            { "QRadioButton", &makeWidget<QRadioButton> },
            { "QLineEdit", &makeWidget<QLineEdit> },
            { "QTextEdit", &makeWidget<QTextEdit> },
            { "QSpinBox", &makeWidget<QSpinBox> },
            { "QDoubleSpinBox", &makeWidget<QDoubleSpinBox> },
            { "QComboBox", &makeWidget<QComboBox> },
            { "QSlider", &makeWidget<QSlider> },
            { "QProgressBar", &makeWidget<QProgressBar> },
            { "QScrollBar", &makeWidget<QScrollBar> },
            { "QScrollArea", &makeWidget<QScrollArea> },
            { "QTreeView", &makeWidget<QTreeView> },
            { "QTableView", &makeWidget<QTableView> },
            { "QListView", &makeWidget<QListView> },
            { "QHeaderView", [](QWidget* parent) -> QWidget* { return new QHeaderView(Qt::Horizontal, parent); } },
            { "QTabWidget", &makeWidget<QTabWidget> },
            { "QTabBar", &makeWidget<QTabBar> },
            { "QToolBar", &makeWidget<QToolBar> },
            { "QMenuBar", &makeWidget<QMenuBar> },
            { "QMenu", &makeWidget<QMenu> },
            { "QGroupBox", &makeWidget<QGroupBox> },
            { "QSplitter", &makeWidget<QSplitter> },
            { "QStackedWidget", &makeWidget<QStackedWidget> },
            { "QDockWidget", &makeWidget<QDockWidget> },
            { "QMainWindow", &makeWidget<QMainWindow> },
            { "QDialog", &makeWidget<QDialog> },
            { "QDialogButtonBox", &makeWidget<QDialogButtonBox> },
            { "Card", &makeWidget<Card> },
            { "CardHeader", &makeWidget<CardHeader> },
            { "BrowseEdit", &makeWidget<BrowseEdit> },
            { "VectorInput", [](QWidget* parent) -> QWidget* { return new VectorInput(parent, 3); } },
            { "ColorLabel", [](QWidget* parent) -> QWidget* { return new ColorLabel(parent); } },
            { "ElidingLabel", [](QWidget* parent) -> QWidget* { return new ElidingLabel(parent); } },
            { "SegmentControl", [](QWidget* parent) -> QWidget* { return new SegmentControl(parent); } },
            { "SliderInt", &makeWidget<SliderInt> },
            { "SliderDouble", &makeWidget<SliderDouble> },
            { "SliderCombo", [](QWidget* parent) -> QWidget* { return new SliderCombo(parent); } },
            { "SpinBox", &makeWidget<SpinBox> },
            { "DoubleSpinBox", &makeWidget<DoubleSpinBox> },
            { "TabWidget", &makeWidget<TabWidget> },
            { "TableView", &makeWidget<TableView> },
            { "StyledTreeView", &makeWidget<StyledTreeView> },
            { "StyledTreeWidget", &makeWidget<StyledTreeWidget> },
        };

        //! The widget and everything below it, the widget first
        QList<QWidget*> treeOf(QWidget* root)
        {
            QList<QWidget*> all = root->findChildren<QWidget*>();
            all.prepend(root);
            return all;
        }

        //! The widgets of a tree that still carry a sheet or are still styled by a sheet style (their class names, for the message)
        QStringList withSheetLeft(QWidget* root)
        {
            QStringList left;
            for (QWidget* widget : treeOf(root))
            {
                if (!widget->styleSheet().isEmpty() || hasSheetStyle(*widget))
                {
                    left.append(QString::fromLatin1(widget->metaObject()->className()));
                }
            }
            return left;
        }

        //! The widgets of a tree that carry Qt::WA_StyleSheet without a sheet style: the sheet style marks every widget it is asked about,
        //! and takes the mark off only from a widget it unpolishes
        QStringList withStaleAttributeLeft(QWidget* root)
        {
            QStringList left;
            for (QWidget* widget : treeOf(root))
            {
                if (widget->testAttribute(Qt::WA_StyleSheet) && !hasSheetStyle(*widget))
                {
                    left.append(QString::fromLatin1(widget->metaObject()->className()));
                }
            }
            return left;
        }

        //! Shows the widget the way a window is shown, without a window on a screen, and lets the event loop run once
        void showOffscreen(QWidget& widget)
        {
            widget.setAttribute(Qt::WA_DontShowOnScreen);
            widget.resize(320, 240);
            widget.show();
            QApplication::processEvents();
        }

        //! Gives the widget a sheet, and every second widget below it (a form of the Editor has sheets at several levels)
        void giveSheets(QWidget* root)
        {
            const QList<QWidget*> tree = treeOf(root);
            for (int index = 0; index < tree.size(); index += 2)
            {
                tree[index]->setStyleSheet(OwnSheet);
            }
        }

        //! What the widgets of a tree show, by class: the distinct values among the widgets of the class. (A tree that had a sheet can hold
        //! a widget more than one that never had: a class creates some of its children when it is told that its style changed.)
        using ValuesByClass = QMap<QString, QSet<quint32>>;

        //! The pixel sizes of the fonts of a tree after its root was given a font of 31 pixels
        ValuesByClass pixelSizesAfterSetFont(QWidget* root)
        {
            QFont font = root->font();
            font.setPixelSize(31);
            root->setFont(font);
            ValuesByClass sizes;
            for (const QWidget* widget : treeOf(root))
            {
                sizes[QString::fromLatin1(widget->metaObject()->className())].insert(static_cast<quint32>(widget->font().pixelSize()));
            }
            return sizes;
        }

        //! The text colours of a tree after its root was given a palette
        ValuesByClass textColorsAfterSetPalette(QWidget* root)
        {
            QPalette palette = root->palette();
            palette.setColor(QPalette::WindowText, QColor(0x65, 0x43, 0x21));
            root->setPalette(palette);
            ValuesByClass colors;
            for (const QWidget* widget : treeOf(root))
            {
                colors[QString::fromLatin1(widget->metaObject()->className())].insert(widget->palette().color(QPalette::WindowText).rgb());
            }
            return colors;
        }

        //! The classes that both trees hold and whose widgets show other values in the two
        QStringList classesThatDiffer(const ValuesByClass& first, const ValuesByClass& second)
        {
            QStringList differing;
            for (auto it = first.constBegin(); it != first.constEnd(); ++it)
            {
                if (second.contains(it.key()) && second[it.key()] != it.value())
                {
                    differing.append(it.key());
                }
            }
            return differing;
        }
    } // namespace GuiPerfFloorTestsInternal

    TEST_F(GuiPerfFloorTests, Strip_EveryClassTheEditorBuildsEndsWithoutASheetOrASheetStyleAnywhereInItsTree)
    {
        for (const Rung rung : { Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            registerGlobalElements();
            GuiPerfFloor::install(m_application.get());
            GuiPerfCounters::install(m_application.get());      // after the floor, as StyleManager::initialize does

            for (const GuiPerfFloorTestsInternal::NamedFactory& entry : GuiPerfFloorTestsInternal::WidgetClasses)
            {
                QWidget host;
                QWidget* widget = entry.m_make(&host);
                widget->setGeometry(0, 0, 160, 60);
                GuiPerfFloorTestsInternal::giveSheets(widget);
                const quint64 stripped = GuiPerfFloor::counts().m_stripped + GuiPerfFloor::counts().m_strippedLate;

                GuiPerfFloorTestsInternal::showOffscreen(host);
                for (QWidget* shown : GuiPerfFloorTestsInternal::treeOf(widget))
                {
                    shown->ensurePolished();        // a window (a menu, a main window) is not polished by the show of its host
                }
                QApplication::processEvents();

                const QStringList sheets = GuiPerfFloorTestsInternal::withSheetLeft(&host);
                EXPECT_TRUE(sheets.isEmpty()) << entry.m_name << " at " << GuiPerfFloorTestsInternal::named(rung)
                                              << " still has a sheet or a sheet style on "
                                              << sheets.join(QLatin1Char(' ')).toUtf8().constData();
                const QStringList attributes = GuiPerfFloorTestsInternal::withStaleAttributeLeft(&host);
                EXPECT_TRUE(attributes.isEmpty()) << entry.m_name << " at " << GuiPerfFloorTestsInternal::named(rung)
                                                  << " still has Qt::WA_StyleSheet without a sheet style on "
                                                  << attributes.join(QLatin1Char(' ')).toUtf8().constData();
                EXPECT_GE(GuiPerfFloor::counts().m_stripped + GuiPerfFloor::counts().m_strippedLate, stripped + 1) << entry.m_name;
            }
        }
    }

    TEST_F(GuiPerfFloorTests, Strip_ATreeThatHadASheetTakesFontAndPaletteFromItsParentLikeATreeThatNeverDid)
    {
        // The sheet style marks every widget it is asked about with Qt::WA_StyleSheet, and a marked widget is left out of the font and
        // the palette that its parent hands down (qwidget.cpp:1884, 4603, 4750, 4853). The children of a widget that is polished for the
        // first time were not polished yet when the sheet went: unless the floor takes the mark off, they do not follow their parent
        const char* const classes[] = { "BrowseEdit", "SliderInt", "SliderCombo", "ElidingLabel", "TabWidget", "CardHeader",
                                        "ColorLabel", "SegmentControl", "QTabWidget", "QToolBar", "QComboBox", "QSpinBox" };
        for (const char* wanted : classes)
        {
            struct Observed
            {
                GuiPerfFloorTestsInternal::ValuesByClass m_fonts;
                GuiPerfFloorTestsInternal::ValuesByClass m_colors;
            };
            const auto observe = [this, wanted](bool withSheet, bool lateSheet)
            {
                GuiPerfFloor::resetForTesting();
                startAt(Rung::Noop);
                registerGlobalElements();
                GuiPerfFloor::install(m_application.get());
                Observed observed;
                for (const GuiPerfFloorTestsInternal::NamedFactory& entry : GuiPerfFloorTestsInternal::WidgetClasses)
                {
                    if (QByteArray(entry.m_name) != QByteArray(wanted))
                    {
                        continue;
                    }
                    QWidget host;
                    QWidget* widget = entry.m_make(&host);
                    widget->setGeometry(0, 0, 160, 60);
                    if (withSheet && !lateSheet)
                    {
                        widget->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);     // the first polish clears it
                    }
                    GuiPerfFloorTestsInternal::showOffscreen(host);
                    if (withSheet && lateSheet)
                    {
                        widget->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);     // the next turn clears it
                        QApplication::processEvents();
                    }
                    for (QWidget* shown : GuiPerfFloorTestsInternal::treeOf(widget))
                    {
                        shown->ensurePolished();
                    }
                    observed.m_fonts = GuiPerfFloorTestsInternal::pixelSizesAfterSetFont(widget);
                    observed.m_colors = GuiPerfFloorTestsInternal::textColorsAfterSetPalette(widget);
                }
                return observed;
            };

            const Observed never = observe(false, false);
            const Observed atFirstPolish = observe(true, false);
            const Observed late = observe(true, true);

            ASSERT_FALSE(never.m_fonts.isEmpty()) << wanted;
            const auto list = [](const QStringList& classes)
            {
                return classes.join(QLatin1Char(' ')).toUtf8();
            };
            EXPECT_TRUE(GuiPerfFloorTestsInternal::classesThatDiffer(atFirstPolish.m_fonts, never.m_fonts).isEmpty())
                << wanted << ": a sheet cleared at the first polish, the font of "
                << list(GuiPerfFloorTestsInternal::classesThatDiffer(atFirstPolish.m_fonts, never.m_fonts)).constData();
            EXPECT_TRUE(GuiPerfFloorTestsInternal::classesThatDiffer(atFirstPolish.m_colors, never.m_colors).isEmpty())
                << wanted << ": a sheet cleared at the first polish, the palette of "
                << list(GuiPerfFloorTestsInternal::classesThatDiffer(atFirstPolish.m_colors, never.m_colors)).constData();
            EXPECT_TRUE(GuiPerfFloorTestsInternal::classesThatDiffer(late.m_fonts, never.m_fonts).isEmpty())
                << wanted << ": a sheet cleared on the next turn, the font of "
                << list(GuiPerfFloorTestsInternal::classesThatDiffer(late.m_fonts, never.m_fonts)).constData();
            EXPECT_TRUE(GuiPerfFloorTestsInternal::classesThatDiffer(late.m_colors, never.m_colors).isEmpty())
                << wanted << ": a sheet cleared on the next turn, the palette of "
                << list(GuiPerfFloorTestsInternal::classesThatDiffer(late.m_colors, never.m_colors)).constData();
        }
    }

    TEST_F(GuiPerfFloorTests, Strip_AChildThatStillHasASheetStyleKeepsItsMarkSoThatItsSheetStyleUndoesItsPaletteWhenTheTurnComes)
    {
        // A polished child is given a sheet that sets its palette (a late sheet: it waits for its turn) while its parent, not polished
        // yet, is stripped at its first polish. The mark of the child is the sheet style's own, and the sheet style undoes the palette only
        // for a marked widget (qstylesheetstyle.cpp:3006): a strip that took the mark off the child would leave the palette of the sheet
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        QWidget parent;
        auto* child = new QLabel(QStringLiteral("child"), &parent);
        child->ensurePolished();
        const QColor original = child->palette().color(QPalette::WindowText);
        child->setStyleSheet(QStringLiteral("QLabel { color: #654321; }"));
        ASSERT_EQ(child->palette().color(QPalette::WindowText), QColor(0x65, 0x43, 0x21)) << "the sheet gave the child its own palette";
        parent.setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        parent.ensurePolished();                            // the strip of the parent, and nothing asks the style of the child after it
        QCoreApplication::sendPostedEvents(child, 0);       // the turn of the child

        EXPECT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&parent).isEmpty());
        EXPECT_EQ(child->palette().color(QPalette::WindowText), original);
    }

    TEST_F(GuiPerfFloorTests, Strip_AReparentTripOfAStrippedTreeIsNotCountedAsUnderASheet)
    {
        // What the pooling of the property editor does to a widget: out of its parent and back. The counters count the ParentChange and the
        // StyleChange of a widget that has Qt::WA_StyleSheet as "under a sheet" (the share the kit reports): once the sheets are stripped,
        // no widget of the tree may have the mark, or the trip of a pooled widget would read as one under a sheet
        for (const char* wanted : { "BrowseEdit", "SliderCombo", "CardHeader", "TabWidget" })
        {
            GuiPerfFloor::resetForTesting();
            startAt(Rung::Noop);
            registerGlobalElements();
            GuiPerfFloor::install(m_application.get());
            GuiPerfCounters::install(m_application.get());
            QWidget host;
            QWidget* widget = nullptr;
            for (const GuiPerfFloorTestsInternal::NamedFactory& entry : GuiPerfFloorTestsInternal::WidgetClasses)
            {
                if (QByteArray(entry.m_name) == QByteArray(wanted))
                {
                    widget = entry.m_make(&host);
                }
            }
            ASSERT_NE(widget, nullptr) << wanted;
            widget->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);
            GuiPerfFloorTestsInternal::showOffscreen(host);
            const GuiPerfCounters::Counters before = GuiPerfCounters::counters();

            widget->setParent(nullptr);
            widget->setParent(&host);
            QApplication::processEvents();

            const GuiPerfCounters::Counters after = GuiPerfCounters::counters();
            EXPECT_GT(after.m_evParent, before.m_evParent) << wanted << ": the trip happened";
            EXPECT_EQ(after.m_evParentSheet, before.m_evParentSheet) << wanted;
            EXPECT_EQ(after.m_evStyleSheet, before.m_evStyleSheet) << wanted;
            EXPECT_EQ(after.m_polishSheet, before.m_polishSheet) << wanted;
            EXPECT_EQ(after.m_unpolishSheet, before.m_unpolishSheet) << wanted;
        }
    }

    TEST_F(GuiPerfFloorTests, Strip_ARealCardWithItsHeaderSheetAndAVectorInputWithLabelStylesEndWithoutASheetStyle)
    {
        // The floor is at noop from the start, with its filter in, and these sites are not routed here: the sheets are set with
        // setStyleSheet, the way a site nobody routed sets them. Some widgets are polished already when their sheet arrives (the labels of
        // a vector control), some are not: the strip clears the first at the next turn and the others at their first polish
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        GuiPerfCounters::install(m_application.get());
        QWidget host;
        auto* hostLayout = new QVBoxLayout(&host);
        auto* card = new Card(&host);
        card->setTitle(QStringLiteral("Transform"));
        auto* content = new QWidget;
        auto* contentLayout = new QVBoxLayout(content);
        auto* vector = new VectorInput(content, 3);
        contentLayout->addWidget(vector);
        contentLayout->addWidget(new QLabel(QStringLiteral("a label"), content));
        card->setContentWidget(content);
        hostLayout->addWidget(card);
        card->header()->setStyleSheet(QStringLiteral("QToolTip { padding: 4px; }"));
        for (QLabel* label : vector->findChildren<QLabel*>())
        {
            label->setStyleSheet(QStringLiteral("QLabel { color: #FF0000; border-left: 2px solid #FF0000; }"));
        }
        quint64 withSheet = 0;
        for (const QWidget* widget : GuiPerfFloorTestsInternal::treeOf(&host))
        {
            withSheet += widget->styleSheet().isEmpty() ? 0 : 1;
        }
        ASSERT_GE(withSheet, 4u) << "the header, and the labels of the vector";

        GuiPerfFloorTestsInternal::showOffscreen(host);

        EXPECT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&host).isEmpty());
        EXPECT_TRUE(GuiPerfFloorTestsInternal::withStaleAttributeLeft(&host).isEmpty());
        const GuiPerfFloor::Counts floorCounts = GuiPerfFloor::counts();
        EXPECT_EQ(floorCounts.m_stripped + floorCounts.m_strippedLate, withSheet) << "every sheet was cleared, one way or the other";
        EXPECT_EQ(GuiPerfCounters::counters().m_evPolishOwnSheet, floorCounts.m_stripped)
            << "and the counters saw each one that was cleared at the first polish";
    }

    TEST_F(GuiPerfFloorTests, Strip_ACardWhoseSheetsWereStrippedHasTheGeometryOfACardThatNeverHadThem)
    {
        // The two ways noop has of taking an inline sheet away must leave the same GUI: the helper (the sheet is never made) and the strip
        // (the sheet is made, and cleared at the first polish or on the next turn). The sheets here change the geometry, so that a sheet
        // left in effect would show
        const auto geometryOfACard = [this](bool viaHelper)
        {
            GuiPerfFloor::resetForTesting();
            startAt(Rung::Noop);
            GuiPerfFloor::install(m_application.get());
            QWidget host;
            auto* hostLayout = new QVBoxLayout(&host);
            auto* card = new Card(&host);
            card->setTitle(QStringLiteral("Transform"));
            auto* content = new QWidget;
            auto* contentLayout = new QVBoxLayout(content);
            auto* vector = new VectorInput(content, 3);
            contentLayout->addWidget(vector);
            card->setContentWidget(content);
            hostLayout->addWidget(card);
            const QString labelSheet = QStringLiteral("QLabel { margin: 9px; min-width: 40px; }");
            for (int index = 0; index < 3; ++index)
            {
                if (viaHelper)
                {
                    vector->setLabelStyle(index, labelSheet);                                   // the site that the helper routes
                }
                else
                {
                    // the same sheet, set as a site nobody routed sets it
                    vector->findChildren<QLabel*>().at(index)->setStyleSheet(labelSheet);
                }
            }
            if (!viaHelper)
            {
                card->header()->setStyleSheet(QStringLiteral("QWidget { padding: 6px; }"));
            }
            QApplication::processEvents();      // the next turn: what was set after a polish is cleared before the layout and the show

            GuiPerfFloorTestsInternal::showOffscreen(host);

            EXPECT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&host).isEmpty()) << (viaHelper ? "via the helper" : "via the strip");
            QList<QRect> geometry;
            for (const QWidget* widget : GuiPerfFloorTestsInternal::treeOf(&host))
            {
                geometry.append(widget->geometry());
            }
            return geometry;
        };

        const QList<QRect> viaTheHelper = geometryOfACard(true);
        const QList<QRect> viaTheStrip = geometryOfACard(false);

        ASSERT_GT(viaTheHelper.size(), 10);
        EXPECT_EQ(viaTheStrip, viaTheHelper);
    }

    TEST_F(GuiPerfFloorTests, LateStrip_ASheetGivenToTheHeaderOfAShownCardIsClearedOnTheNextTurnAndItsSubtreeLeavesTheSheetStyle)
    {
        startAt(Rung::Noop);
        GuiPerfFloor::install(m_application.get());
        Card card;
        card.setTitle(QStringLiteral("Transform"));
        card.setContentWidget(new QLabel(QStringLiteral("content")));
        GuiPerfFloorTestsInternal::showOffscreen(card);
        ASSERT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&card).isEmpty());

        card.header()->setStyleSheet(GuiPerfFloorTestsInternal::OwnSheet);

        EXPECT_FALSE(GuiPerfFloorTestsInternal::withSheetLeft(card.header()).isEmpty()) << "in effect for the turn that makes it";
        QApplication::processEvents();
        EXPECT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&card).isEmpty());
        EXPECT_TRUE(GuiPerfFloorTestsInternal::withStaleAttributeLeft(&card).isEmpty());
        EXPECT_EQ(GuiPerfFloor::counts().m_strippedLate, 1u);
    }

    TEST_F(GuiPerfFloorTests, Scenario_ASelectionOfCardsWithVectorLabelsNeverMakesASheetStyleAtNoopAndPassthrough)
    {
        // The inspector's selection in small: 25 cards, each with a vector control whose three labels get a sheet (StyleForX/Y/Z), built
        // through the sites that the helper routes. The counters are the reading the kit takes: below noop the headers (and, when they are
        // not polished yet, the labels) carry sheets that the counters see; at noop and passthrough there is none, nothing is stripped, and
        // no polish reached the style under a sheet style.
        for (const Rung rung : { Rung::Flat, Rung::Noop, Rung::Passthrough })
        {
            GuiPerfFloor::resetForTesting();
            startAt(rung);
            registerGlobalElements();
            if (rung != Rung::Flat)
            {
                GuiPerfFloor::install(m_application.get());
            }
            GuiPerfCounters::install(m_application.get());

            QWidget host;
            auto* hostLayout = new QVBoxLayout(&host);
            constexpr int CardCount = 25;
            for (int index = 0; index < CardCount; ++index)
            {
                auto* card = new Card(&host);
                card->setTitle(QStringLiteral("Component %1").arg(index));
                auto* content = new QWidget;
                auto* contentLayout = new QVBoxLayout(content);
                auto* vector = new VectorInput(content, 3);
                for (int element = 0; element < 3; ++element)
                {
                    vector->setLabelStyle(element, QStringLiteral("QLabel { color: #FF0000; }"));
                }
                contentLayout->addWidget(vector);
                card->setContentWidget(content);
                hostLayout->addWidget(card);
            }
            GuiPerfFloorTestsInternal::showOffscreen(host);

            const GuiPerfCounters::Counters counters = GuiPerfCounters::counters();
            const GuiPerfFloor::Counts floorCounts = GuiPerfFloor::counts();
            constexpr quint64 Sheets = CardCount * (1 + 3);
            if (rung == Rung::Flat)
            {
                EXPECT_GE(counters.m_evPolishOwnSheet, static_cast<quint64>(CardCount));
                EXPECT_LE(counters.m_evPolishOwnSheet, Sheets);
                EXPECT_GT(counters.m_polishSheet, 0u);
                EXPECT_FALSE(GuiPerfFloorTestsInternal::withSheetLeft(&host).isEmpty());
            }
            else
            {
                EXPECT_EQ(counters.m_evPolishOwnSheet, 0u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_EQ(counters.m_polishSheet, 0u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_EQ(counters.m_unpolishSheet, 0u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_EQ(floorCounts.m_stripped, 0u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_EQ(floorCounts.m_strippedLate, 0u) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_EQ(floorCounts.m_helperSkipped, Sheets) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_TRUE(GuiPerfFloorTestsInternal::withSheetLeft(&host).isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
                EXPECT_TRUE(GuiPerfFloorTestsInternal::withStaleAttributeLeft(&host).isEmpty()) << GuiPerfFloorTestsInternal::named(rung);
            }
        }
    }
} // namespace AzQtComponents
