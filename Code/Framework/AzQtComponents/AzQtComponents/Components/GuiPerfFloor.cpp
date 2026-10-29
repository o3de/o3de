/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Debug/Trace.h>

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QMetaObject>
#include <QObject>
#include <QStyle>
#include <QWidget>

#include <atomic>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others into one
    //! translation unit, where anonymous namespaces merge (StyleManager.cpp does the same).
    namespace GuiPerfFloorInternal
    {
        // ------------------------------------------------------------------------------
        // Names
        // ------------------------------------------------------------------------------

        constexpr const char* FlatFlag = "--disable-stylesheets";
        constexpr const char* NoopFlag = "--stylesheets-noop";
        constexpr const char* PassthroughFlag = "--style-passthrough";
        constexpr const char* FlatVariable = "O3DE_DISABLE_STYLESHEETS";
        constexpr const char* NoopVariable = "O3DE_STYLESHEETS_NOOP";
        constexpr const char* PassthroughVariable = "O3DE_STYLE_PASSTHROUGH";

        constexpr const char* SnapshotProperty = "_o3de_perf_snapshot";     // the property GuiPerfCounters reads
        constexpr const char* RungProperty = "_o3de_floor_rung";
        // Only AZ_TracePrintf names it, and that is compiled out of a build without tracing
        [[maybe_unused]] constexpr const char* LogWindow = "GuiPerfFloor";

        // ------------------------------------------------------------------------------
        // State: the rung, the counts, the lines
        // ------------------------------------------------------------------------------

        //! The rung, or -1 while the flags and variables have not been read. One read of this is what every hook costs.
        std::atomic<int> g_rung{ -1 };

        std::atomic<quint64> g_stripped{ 0 };
        std::atomic<quint64> g_strippedLate{ 0 };
        std::atomic<quint64> g_helperSkipped{ 0 };

        //! Test seam: the lines go here instead of the log while it is set
        QStringList* g_capturedLines = nullptr;

        //! The line goes to the log (window "GuiPerfFloor"), or to the captured list in a test
        void writeLine(const QString& line)
        {
            if (g_capturedLines)
            {
                g_capturedLines->append(line);
                return;
            }
            AZ_TracePrintf(LogWindow, "%s\n", line.toUtf8().constData());
        }

        //! A flag is matched like --disable-stylesheets always was: whole, without regard to case
        bool hasFlag(const char* flag)
        {
            return QCoreApplication::instance() && QCoreApplication::arguments().contains(QLatin1String(flag), Qt::CaseInsensitive);
        }

        //! A variable of the floor is on when it is set to anything but "0"
        bool hasVariable(const char* name)
        {
            return qEnvironmentVariableIsSet(name) && qgetenv(name) != "0";
        }

        // ------------------------------------------------------------------------------
        // Clearing the inline sheets
        // ------------------------------------------------------------------------------

        //! Whether the style of the widget is a style sheet style (QStyleSheetStyle has no public header, so it is told by its class name)
        bool hasSheetStyle(const QWidget* widget)
        {
            return widget->style()->inherits("QStyleSheetStyle");
        }

        //! The mark of the sheet style, Qt::WA_StyleSheet, off a widget that no sheet style styles
        void takeSheetMarkOff(QWidget* widget)
        {
            if (widget->testAttribute(Qt::WA_StyleSheet) && !hasSheetStyle(widget))
            {
                widget->setAttribute(Qt::WA_StyleSheet, false);
            }
        }

        //! The same for the widget and everything below it
        void takeSheetMarksOffBelow(QWidget* widget)
        {
            takeSheetMarkOff(widget);
            for (QObject* child : widget->children())
            {
                if (child->isWidgetType())
                {
                    takeSheetMarksOffBelow(static_cast<QWidget*>(child));
                }
            }
        }

        //! The sheet style puts Qt::WA_StyleSheet on every widget it is asked about, polished or not (initObject, qstylesheetstyle.cpp:
        //! 1628, 1831, 2147), and takes it off only from a widget it unpolishes (:3018) - which setStyle_helper does only for a widget
        //! that was polished (qwidget.cpp:2693). The children of the widget that is polished now were not, so they leave the sheet style
        //! with the mark still on, and with it Qt leaves them out of the font and the palette their parent hands down (qwidget.cpp:1884,
        //! 4603, 4750, 4853): a widget that never had a sheet follows its parent. A child that still has its sheet style can put the mark
        //! on the widgets above it too, when it is asked with one of them as the widget. Taking the marks off is what the sheet style
        //! would have done.
        void takeSheetMarksOff(QWidget* widget)
        {
            takeSheetMarksOffBelow(widget);
            for (QWidget* parent = widget->parentWidget(); parent; parent = parent->parentWidget())
            {
                takeSheetMarkOff(parent);
            }
        }

        //! Takes the sheet of the widget away, and the marks it left on the widgets around it
        void clearSheet(QWidget* widget)
        {
            widget->setStyleSheet(QString());
            takeSheetMarksOff(widget);
        }

        //! A widget gets its first polish from QWidget::ensurePolished (qwidget.cpp:10239) as a QEvent::Polish (:10248-10249), and an
        //! application wide filter sees that event BEFORE the widget does: QApplicationPrivate::notify_helper asks the application
        //! filters first (qapplication.cpp:3276), then the filters of the widget, then calls QWidget::event, which polishes the widget
        //! with its style (qwidget.cpp:9107-9109). A sheet cleared here is gone when that polish comes, and the widget is polished
        //! with the style of the application. Clearing it at this point is safe:
        //!  - setStyleSheet() of an empty sheet only calls inheritStyle() (qwidget.cpp:2595-2600), which gives the widget back the style
        //!    it had before the sheet (:2748-2780) through setStyle_helper (:2680-2724);
        //!  - ensurePolished has set the polished mark already (:10246), so setStyle_helper unpolishes the widget with the old style
        //!    and polishes it with the new one (:2693-2696). QStyleSheetStyle::unpolish of a widget it never polished hands the call
        //!    to the base style (qstylesheetstyle.cpp:3006-3008), so nothing of a sheet has to be undone. The QWidget::event that
        //!    follows polishes the widget once more with the plain style: that is what a stripped widget costs;
        //!  - the children take their style from the same call (:2698-2705), and none of them is polished by Qt yet, as ensurePolished
        //!    polishes the children after the widget (:10251-10259). A child with a sheet of its own keeps its sheet style and is
        //!    repolished under it (:2736-2740, updateObjects, qstylesheetstyle.cpp:2782-2802): its own Polish event clears it in turn;
        //!  - the filter consumes nothing and changes no list that Qt walks at this moment (the application filters are walked by
        //!    index, and ensurePolished walks a copy of the children, :10252): it is a call that user code makes from an event()
        //!    override of Polish.
        void stripAtFirstPolish(QObject* watched)
        {
            if (!watched->isWidgetType())
            {
                return;
            }

            auto* widget = static_cast<QWidget*>(watched);
            if (!widget->styleSheet().isEmpty())
            {
                clearSheet(widget);
                ++g_stripped;
            }
        }

        //! The widget is still alive and still has the sheet: clear it (the call is queued, it runs outside the setStyleSheet that
        //! made it)
        void stripLate(QWidget* widget)
        {
            if (!widget->styleSheet().isEmpty())
            {
                clearSheet(widget);
                ++g_strippedLate;
            }
        }

        //! A sheet given to a widget that was polished before is in effect at once (setStyleSheet -> setStyle_helper repolishes a
        //! polished widget, qwidget.cpp:2680-2690), and a StyleChange tells the filter. Clearing it inside that call would change the
        //! style under setStyle_helper and updateObjects (qstylesheetstyle.cpp:2782-2802), so the clearing is queued to the widget:
        //! Qt drops it if the widget is gone. A widget that was not polished yet is left to stripAtFirstPolish, which is cheaper.
        void queueLateStrip(QObject* watched)
        {
            if (!watched->isWidgetType())
            {
                return;
            }

            auto* widget = static_cast<QWidget*>(watched);
            if (widget->testAttribute(Qt::WA_WState_Polished) && !widget->styleSheet().isEmpty())
            {
                QMetaObject::invokeMethod(
                    widget,
                    [widget]()
                    {
                        stripLate(widget);
                    },
                    Qt::QueuedConnection);
            }
        }

        // ------------------------------------------------------------------------------
        // The filter
        // ------------------------------------------------------------------------------

        //! The filter of the application, null while there is none. A child of the QApplication, as the counters are, and it goes with
        //! it. No Q_OBJECT: it has no signals, slots or properties.
        std::atomic<QObject*> g_filter{ nullptr };

        class StripFilter
            : public QObject
        {
        public:
            explicit StripFilter(QApplication* application)
                : QObject(application)
            {
                g_filter = this;
                application->installEventFilter(this);
            }

            ~StripFilter() override
            {
                g_filter = nullptr;
            }

        protected:
            //! Never consumes an event
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                switch (event->type())
                {
                case QEvent::Polish:
                    stripAtFirstPolish(watched);
                    break;

                case QEvent::StyleChange:
                    queueLateStrip(watched);
                    break;

                case QEvent::DynamicPropertyChange:
                    onDynamicPropertyChange(watched, static_cast<const QDynamicPropertyChangeEvent*>(event));
                    break;

                default:
                    break;
                }
                return false;
            }

        private:
            //! Setting the snapshot property on the application writes the line, inside that setProperty call (GuiPerfCounters does
            //! the same: QObject::setProperty sends the event at once, and an application wide filter sees an event sent to the
            //! application itself)
            void onDynamicPropertyChange(QObject* watched, const QDynamicPropertyChangeEvent* event)
            {
                if (watched != parent() || event->propertyName() != SnapshotProperty)
                {
                    return;
                }

                const QString tag = watched->property(SnapshotProperty).toString();
                if (!tag.isEmpty())
                {
                    writeLine(GuiPerfFloor::formatSnapshotLine(tag, GuiPerfFloor::rung(), GuiPerfFloor::counts()));
                }
            }
        };
    } // namespace GuiPerfFloorInternal

    // ----------------------------------------------------------------------------------
    // The ladder
    // ----------------------------------------------------------------------------------

    GuiPerfFloor::Rung GuiPerfFloor::readRung()
    {
        using namespace GuiPerfFloorInternal;

        if (hasFlag(PassthroughFlag) || hasVariable(PassthroughVariable))
        {
            return Rung::Passthrough;
        }
        if (hasFlag(NoopFlag) || hasVariable(NoopVariable))
        {
            return Rung::Noop;
        }
        // The kill switch as it always was: its variable counts when it is set at all
        if (hasFlag(FlatFlag) || qEnvironmentVariableIsSet(FlatVariable))
        {
            return Rung::Flat;
        }
        return Rung::Qss;
    }

    GuiPerfFloor::Rung GuiPerfFloor::rung()
    {
        using namespace GuiPerfFloorInternal;

        int value = g_rung.load(std::memory_order_relaxed);
        if (value < 0)
        {
            value = static_cast<int>(readRung());
            g_rung.store(value, std::memory_order_relaxed);
        }
        return static_cast<Rung>(value);
    }

    const char* GuiPerfFloor::rungName(Rung rung)
    {
        switch (rung)
        {
        case Rung::Qss:
            return "qss";
        case Rung::Flat:
            return "flat";
        case Rung::Noop:
            return "noop";
        case Rung::Passthrough:
            return "passthrough";
        }
        return "qss";
    }

    bool GuiPerfFloor::inlineSheetsDisabled()
    {
        return rung() >= Rung::Noop;
    }

    bool GuiPerfFloor::isPassthrough()
    {
        return rung() == Rung::Passthrough;
    }

    // ----------------------------------------------------------------------------------
    // Inline style sheets
    // ----------------------------------------------------------------------------------

    void GuiPerfFloor::setInlineStyleSheet(QWidget* widget, const QString& sheet)
    {
        if (inlineSheetsDisabled())
        {
            ++GuiPerfFloorInternal::g_helperSkipped;
            return;
        }
        widget->setStyleSheet(sheet);
    }

    // ----------------------------------------------------------------------------------
    // Install
    // ----------------------------------------------------------------------------------

    void GuiPerfFloor::initialize(QApplication* application)
    {
        using namespace GuiPerfFloorInternal;

        const Rung current = rung();
        const bool disabled = StyleManager::stylesheetsDisabled();

        writeLine(formatStartupLine(current, disabled));
        application->setProperty(RungProperty, QString::fromLatin1(rungName(current)));

        // The Editor makes its QApplication, and so runs this, before ToolsApplication::Start opens Editor.log (CryEdit.cpp, the
        // EditorQtApplication::newInstance call and the ToolsApplication start after it): the line above reaches the console only.
        // The same line is written again on the first turn of the event loop, which comes after the log is open.
        QMetaObject::invokeMethod(
            application,
            [current, disabled]()
            {
                writeLine(formatStartupLine(current, disabled));
            },
            Qt::QueuedConnection);

        if (current >= Rung::Noop)
        {
            install(application);
        }
    }

    void GuiPerfFloor::install(QApplication* application)
    {
        using namespace GuiPerfFloorInternal;

        if (application && !g_filter)
        {
            new StripFilter(application);       // owned by the application
        }
    }

    bool GuiPerfFloor::isInstalled()
    {
        return GuiPerfFloorInternal::g_filter != nullptr;
    }

    // ----------------------------------------------------------------------------------
    // Reading
    // ----------------------------------------------------------------------------------

    GuiPerfFloor::Counts GuiPerfFloor::counts()
    {
        using namespace GuiPerfFloorInternal;

        Counts now;
        now.m_stripped = g_stripped;
        now.m_strippedLate = g_strippedLate;
        now.m_helperSkipped = g_helperSkipped;
        return now;
    }

    // ----------------------------------------------------------------------------------
    // The lines
    // ----------------------------------------------------------------------------------

    QString GuiPerfFloor::formatStartupLine(Rung rung, bool stylesheetsDisabled)
    {
        return QStringLiteral("GuiPerfFloor: v=1 rung=%1 stylesheets_disabled=%2")
            .arg(QLatin1String(rungName(rung)))
            .arg(stylesheetsDisabled ? 1 : 0);
    }

    QString GuiPerfFloor::formatSnapshotLine(const QString& tag, Rung rung, const Counts& values)
    {
        return QStringLiteral("GuiPerfFloor: v=1 tag=%1 rung=%2 stripped=%3 stripped_late=%4 helper_skipped=%5")
            .arg(tag)
            .arg(QLatin1String(rungName(rung)))
            .arg(values.m_stripped)
            .arg(values.m_strippedLate)
            .arg(values.m_helperSkipped);
    }

    // ----------------------------------------------------------------------------------
    // Test seams
    // ----------------------------------------------------------------------------------

    void GuiPerfFloor::setRungForTesting(Rung rung)
    {
        GuiPerfFloorInternal::g_rung = static_cast<int>(rung);
    }

    void GuiPerfFloor::resetForTesting()
    {
        using namespace GuiPerfFloorInternal;

        g_rung = -1;
        g_stripped = 0;
        g_strippedLate = 0;
        g_helperSkipped = 0;
    }

    void GuiPerfFloor::captureLinesInto(QStringList* lines)
    {
        GuiPerfFloorInternal::g_capturedLines = lines;
    }
} // namespace AzQtComponents
