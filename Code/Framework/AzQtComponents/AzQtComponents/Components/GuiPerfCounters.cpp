/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/GuiPerfCounters.h>

#include <AzCore/Debug/Trace.h>

#include <QApplication>
#include <QByteArray>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QStyle>
#include <QThread>
#include <QWidget>

#include <algorithm>
#include <atomic>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others into one
    //! translation unit, where anonymous namespaces merge (StyleManager.cpp does the same).
    namespace GuiPerfCountersInternal
    {
        // ------------------------------------------------------------------------------
        // Names
        // ------------------------------------------------------------------------------

        constexpr const char* EnableVariable = "O3DE_UI_PERF_COUNTERS";
        constexpr const char* SnapshotProperty = "_o3de_perf_snapshot";
        // Only AZ_TracePrintf names it, and that is compiled out of a build without tracing
        [[maybe_unused]] constexpr const char* LogWindow = "GuiPerfCounters";
        constexpr int ClassesListed = 20;

        // ------------------------------------------------------------------------------
        // A style sheet in effect
        // ------------------------------------------------------------------------------

        //! Whether the style the widget has when a call of Style arrives is a style sheet style (QStyleSheetStyle has no public header,
        //! so it is told by its class name). This is the test of polish and unpolish, and not the attribute Qt::WA_StyleSheet, which is
        //! blind to the very case the counters are for: QStyleSheetStyle::polish calls this style FIRST (qstylesheetstyle.cpp:2875) and
        //! sets the attribute after (:2864, initObject), and QStyleSheetStyle::unpolish clears it (:3018) before it calls this style
        //! (:3028). Read on the attribute, a pool trip under an application style sheet (6 polishes and 6 unpolishes) counts 0 and 0.
        //! While a new style is set, the widget has the NEW style when the old one unpolishes it (qwidget.cpp:2694-2695): an unpolish
        //! counts for the style the widget goes to.
        bool hasSheetStyle(const QWidget* widget)
        {
            return widget && widget->style() && widget->style()->inherits("QStyleSheetStyle");
        }

        //! Whether the widget has Qt::WA_StyleSheet when the event arrives. This is the test of ParentChange and StyleChange: Qt sends
        //! them after it gave the widget its new style and polished it (qwidget.cpp:2694-2716 and 10866-10879), so the attribute is what
        //! the new style left. A widget that was never polished has none, and no sheet did any work for it.
        bool hasSheetAttribute(const QWidget* widget)
        {
            return widget && widget->testAttribute(Qt::WA_StyleSheet);
        }

        // ------------------------------------------------------------------------------
        // The recorder
        // ------------------------------------------------------------------------------

        class Recorder;

        //! The recorder of the application, null while the counters are off: the one thing every hook reads
        std::atomic<Recorder*> g_recorder{ nullptr };

        //! Test seam: the lines go here instead of the log while it is set
        QStringList* g_capturedLines = nullptr;

        //! The line goes to the log (window "GuiPerfCounters", so Editor.log), or to the captured list in a test. A line is far below
        //! the 4095 characters AZ_TracePrintf keeps (Trace.cpp:57): about 600 for the counters, about 1600 for the 20 classes.
        void writeLine(const QString& line)
        {
            if (g_capturedLines)
            {
                g_capturedLines->append(line);
                return;
            }
            AZ_TracePrintf(LogWindow, "%s\n", line.toUtf8().constData());
        }

        //! The counters of one application, and the application wide event filter that counts the widget events. It is a child of
        //! the QApplication, as UiCaptureHarness is, and goes with it. No Q_OBJECT: it has no signals, slots or properties.
        class Recorder
            : public QObject
        {
        public:
            explicit Recorder(QApplication* application)
                : QObject(application)
            {
                g_recorder = this;
                application->installEventFilter(this);
            }

            ~Recorder() override
            {
                g_recorder = nullptr;
            }

            //! The recorder if the counters are on and the caller is the GUI thread, else null: a call from another thread is not counted
            static Recorder* forThisThread()
            {
                Recorder* recorder = g_recorder;
                return (recorder && QThread::currentThread() == recorder->thread()) ? recorder : nullptr;
            }

            // ---- Style calls: begin* counts the call and says whether it is the outermost of its kind (the caller times that one)

            bool beginPolish(const QWidget* widget)
            {
                const bool sheet = hasSheetStyle(widget);
                ++m_counters.m_polish;
                if (sheet)
                {
                    ++m_counters.m_polishSheet;
                }
                if (widget)
                {
                    GuiPerfCounters::ClassCounts& row = classOf(widget);
                    ++row.m_polish;
                    if (sheet)
                    {
                        ++row.m_polishSheet;
                    }
                }
                return m_polishDepth++ == 0;
            }

            void endPolish(bool outermost, quint64 nanoseconds)
            {
                --m_polishDepth;
                if (outermost)
                {
                    m_counters.m_polishNanoseconds += nanoseconds;
                }
            }

            bool beginUnpolish(const QWidget* widget)
            {
                ++m_counters.m_unpolish;
                if (hasSheetStyle(widget))
                {
                    ++m_counters.m_unpolishSheet;
                }
                return m_unpolishDepth++ == 0;
            }

            void endUnpolish(bool outermost, quint64 nanoseconds)
            {
                --m_unpolishDepth;
                if (outermost)
                {
                    m_counters.m_unpolishNanoseconds += nanoseconds;
                }
            }

            void addRepolishHelper(qsizetype widgetCount)
            {
                ++m_counters.m_repolishHelper;
                m_counters.m_repolishHelperWidgets += static_cast<quint64>(widgetCount);
            }

            // ---- Reading

            GuiPerfCounters::Counters counters() const
            {
                GuiPerfCounters::Counters now = m_counters;
                now.m_widgetsAlive = static_cast<quint64>(QApplication::allWidgets().size());
                return now;
            }

            QList<GuiPerfCounters::ClassCounts> classCounts() const
            {
                return m_classes.values();
            }

        protected:
            // ---- Events: the filter counts and never consumes

            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::DynamicPropertyChange)
                {
                    onDynamicPropertyChange(watched, static_cast<const QDynamicPropertyChangeEvent*>(event));
                }
                else if (watched->isWidgetType())
                {
                    onWidgetEvent(static_cast<const QWidget*>(watched), event->type());
                }
                return false;
            }

        private:
            GuiPerfCounters::ClassCounts& classOf(const QWidget* widget)
            {
                const QMetaObject* metaObject = widget->metaObject();
                auto row = m_classes.find(metaObject);
                if (row == m_classes.end())
                {
                    row = m_classes.insert(metaObject, GuiPerfCounters::ClassCounts());
                    row->m_name = QString::fromLatin1(metaObject->className());
                }
                return *row;
            }

            void onWidgetEvent(const QWidget* widget, QEvent::Type type)
            {
                switch (type)
                {
                case QEvent::Polish:
                    ++m_counters.m_evPolish;
                    if (!widget->styleSheet().isEmpty())
                    {
                        ++m_counters.m_evPolishOwnSheet;
                    }
                    break;

                case QEvent::PolishRequest:
                    ++m_counters.m_evPolishRequest;
                    break;

                case QEvent::ParentChange:
                    ++m_counters.m_evParent;
                    ++classOf(widget).m_evParent;
                    if (hasSheetAttribute(widget))
                    {
                        ++m_counters.m_evParentSheet;
                    }
                    break;

                case QEvent::StyleChange:
                    ++m_counters.m_evStyle;
                    ++classOf(widget).m_evStyle;
                    if (hasSheetAttribute(widget))
                    {
                        ++m_counters.m_evStyleSheet;
                    }
                    break;

                default:
                    break;
                }
            }

            //! Setting the snapshot property on the application writes the lines, inside that setProperty call (QObject::setProperty
            //! sends the event to the object at once, qobject.cpp:4452, and an application wide filter sees an event sent to the
            //! application itself, qcoreapplication.cpp:1218)
            void onDynamicPropertyChange(QObject* watched, const QDynamicPropertyChangeEvent* event)
            {
                if (watched != parent() || event->propertyName() != SnapshotProperty)
                {
                    return;
                }

                const QString tag = watched->property(SnapshotProperty).toString();
                if (tag.isEmpty())
                {
                    return;
                }

                writeLine(GuiPerfCounters::formatLine(tag, counters()));
                if (tag.endsWith(QStringLiteral("!classes")))
                {
                    writeLine(GuiPerfCounters::formatClassesLine(tag, classCounts()));
                }
            }

            GuiPerfCounters::Counters m_counters;
            QHash<const QMetaObject*, GuiPerfCounters::ClassCounts> m_classes;
            int m_polishDepth = 0;      //!< polish calls running now (the calls nest)
            int m_unpolishDepth = 0;
        };

        // ------------------------------------------------------------------------------
        // Line pieces
        // ------------------------------------------------------------------------------

        void appendCount(QString& line, const char* key, quint64 value)
        {
            line += QLatin1Char(' ');
            line += QLatin1String(key);
            line += QLatin1Char('=');
            line += QString::number(value);
        }

        void appendMicroseconds(QString& line, const char* key, quint64 nanoseconds)
        {
            line += QLatin1Char(' ');
            line += QLatin1String(key);
            line += QLatin1Char('=');
            line += QString::number(static_cast<double>(nanoseconds) / 1000.0, 'f', 1);
        }
    } // namespace GuiPerfCountersInternal

    // ----------------------------------------------------------------------------------
    // Install
    // ----------------------------------------------------------------------------------

    void GuiPerfCounters::installIfRequested(QApplication* application)
    {
        using namespace GuiPerfCountersInternal;

        if (!qEnvironmentVariableIsSet(EnableVariable) || qgetenv(EnableVariable) == "0")
        {
            return;
        }
        install(application);
    }

    void GuiPerfCounters::install(QApplication* application)
    {
        using namespace GuiPerfCountersInternal;

        if (application && !g_recorder)
        {
            new Recorder(application);      // owned by the application
        }
    }

    bool GuiPerfCounters::isInstalled()
    {
        return GuiPerfCountersInternal::g_recorder != nullptr;
    }

    // ----------------------------------------------------------------------------------
    // Reading
    // ----------------------------------------------------------------------------------

    GuiPerfCounters::Counters GuiPerfCounters::counters()
    {
        const auto* recorder = GuiPerfCountersInternal::g_recorder.load();
        return recorder ? recorder->counters() : Counters();
    }

    QList<GuiPerfCounters::ClassCounts> GuiPerfCounters::classCounts()
    {
        const auto* recorder = GuiPerfCountersInternal::g_recorder.load();
        return recorder ? recorder->classCounts() : QList<ClassCounts>();
    }

    void GuiPerfCounters::captureLinesInto(QStringList* lines)
    {
        GuiPerfCountersInternal::g_capturedLines = lines;
    }

    // ----------------------------------------------------------------------------------
    // The lines
    // ----------------------------------------------------------------------------------

    QString GuiPerfCounters::formatLine(const QString& tag, const Counters& values)
    {
        using namespace GuiPerfCountersInternal;

        QString line = QStringLiteral("GuiPerfCounters: v=1 tag=") + tag;
        appendCount(line, "polish", values.m_polish);
        appendCount(line, "polish_sheet", values.m_polishSheet);
        appendCount(line, "unpolish", values.m_unpolish);
        appendCount(line, "unpolish_sheet", values.m_unpolishSheet);
        appendMicroseconds(line, "polish_us", values.m_polishNanoseconds);
        appendMicroseconds(line, "unpolish_us", values.m_unpolishNanoseconds);
        appendCount(line, "ev_polish", values.m_evPolish);
        appendCount(line, "ev_polish_own_sheet", values.m_evPolishOwnSheet);
        appendCount(line, "ev_parent", values.m_evParent);
        appendCount(line, "ev_parent_sheet", values.m_evParentSheet);
        appendCount(line, "ev_style", values.m_evStyle);
        appendCount(line, "ev_style_sheet", values.m_evStyleSheet);
        appendCount(line, "ev_polish_request", values.m_evPolishRequest);
        appendCount(line, "repolish_helper", values.m_repolishHelper);
        appendCount(line, "repolish_helper_widgets", values.m_repolishHelperWidgets);
        appendCount(line, "widgets_alive", values.m_widgetsAlive);
        return line;
    }

    QString GuiPerfCounters::formatClassesLine(const QString& tag, QList<ClassCounts> classes)
    {
        using namespace GuiPerfCountersInternal;

        std::sort(
            classes.begin(),
            classes.end(),
            [](const ClassCounts& first, const ClassCounts& second)
            {
                return (first.m_polish != second.m_polish) ? (first.m_polish > second.m_polish) : (first.m_name < second.m_name);
            });

        QString line = QStringLiteral("GuiPerfCountersClasses: v=1 tag=") + tag;
        const qsizetype listed = std::min<qsizetype>(ClassesListed, classes.size());
        for (qsizetype i = 0; i < listed; ++i)
        {
            const ClassCounts& row = classes[i];
            line += QLatin1Char(' ');
            line += row.m_name;
            line += QLatin1Char('=');
            line += QString::number(row.m_polish);
            line += QLatin1Char('/');
            line += QString::number(row.m_polishSheet);
            line += QLatin1Char('/');
            line += QString::number(row.m_evParent);
            line += QLatin1Char('/');
            line += QString::number(row.m_evStyle);
        }
        return line;
    }

    // ----------------------------------------------------------------------------------
    // Hooks
    // ----------------------------------------------------------------------------------

    GuiPerfCounters::PolishScope::PolishScope(const QWidget* widget)
    {
        using namespace GuiPerfCountersInternal;

        if (Recorder* recorder = Recorder::forThisThread())
        {
            m_counted = true;
            m_outermost = recorder->beginPolish(widget);
            if (m_outermost)
            {
                m_timer.start();
            }
        }
    }

    GuiPerfCounters::PolishScope::~PolishScope()
    {
        using namespace GuiPerfCountersInternal;

        if (m_counted)
        {
            if (Recorder* recorder = g_recorder)
            {
                recorder->endPolish(m_outermost, m_outermost ? static_cast<quint64>(m_timer.nsecsElapsed()) : 0);
            }
        }
    }

    GuiPerfCounters::UnpolishScope::UnpolishScope(const QWidget* widget)
    {
        using namespace GuiPerfCountersInternal;

        if (Recorder* recorder = Recorder::forThisThread())
        {
            m_counted = true;
            m_outermost = recorder->beginUnpolish(widget);
            if (m_outermost)
            {
                m_timer.start();
            }
        }
    }

    GuiPerfCounters::UnpolishScope::~UnpolishScope()
    {
        using namespace GuiPerfCountersInternal;

        if (m_counted)
        {
            if (Recorder* recorder = g_recorder)
            {
                recorder->endUnpolish(m_outermost, m_outermost ? static_cast<quint64>(m_timer.nsecsElapsed()) : 0);
            }
        }
    }

    void GuiPerfCounters::onRepolishHelper(qsizetype widgetCount)
    {
        using namespace GuiPerfCountersInternal;

        if (Recorder* recorder = Recorder::forThisThread())
        {
            recorder->addRepolishHelper(widgetCount);
        }
    }
} // namespace AzQtComponents
