/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QElapsedTimer>
#include <QList>
#include <QString>
#include <QStringList>

class QApplication;
class QWidget;

namespace AzQtComponents
{
    //! GUI performance counters (GuiPerfCounters v=1). Not product code: nothing happens unless O3DE_UI_PERF_COUNTERS is set (and
    //! is not "0") when StyleManager::initialize runs.
    //!
    //! They count what Qt asks of Style: the calls to Style::polish(QWidget*) and Style::unpolish(QWidget*) and the time inside
    //! them, the widget events that lead to those calls (Polish, ParentChange, StyleChange, PolishRequest) and the executions of
    //! StyleHelpers::repolishWhenPropertyChanges. They exist to check, with style sheets on and with O3DE_DISABLE_STYLESHEETS, what
    //! the property editor pooling work assumes: one unpolish and polish per widget into the pool and one out, and that
    //! re-evaluating style sheets on reparent is what a pooled widget costs.
    //!
    //! The values are unsigned 64-bit, cumulative from install and never reset (a reader subtracts two snapshots). Only the GUI
    //! thread counts: a call from another thread is ignored.
    //!
    //! Reading them: set the dynamic property "_o3de_perf_snapshot" on the QApplication to a tag (non-empty, ASCII, no spaces).
    //! The setProperty call itself writes ONE line to the log (window "GuiPerfCounters", so it lands in Editor.log):
    //!
    //!     GuiPerfCounters: v=1 tag=<tag> polish=<n> polish_sheet=<n> ... widgets_alive=<n>
    //!
    //! A tag that ends with "!classes" adds a second line, the 20 widget classes with the most polish calls:
    //!
    //!     GuiPerfCountersClasses: v=1 tag=<tag> <Class>=<polish>/<polish_sheet>/<ev_parent>/<ev_style> ...
    //!
    //! Both lines carry the tag as it was set. Setting the same tag twice in a row writes once: Qt sends no property change for
    //! an equal value (qobject.cpp:4440-4446).
    class AZ_QT_COMPONENTS_API GuiPerfCounters
    {
    public:
        GuiPerfCounters() = delete;

        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        //! What the snapshot line carries, in the order of the line. "Sheet" in a polish or unpolish count means that the style of the
        //! widget is a style sheet style when the call arrives, in an event count that the widget has Qt::WA_StyleSheet when the
        //! event arrives (hasSheetStyle and hasSheetAttribute in GuiPerfCounters.cpp say why the two differ).
        struct Counters
        {
            quint64 m_polish = 0;                   //!< Style::polish(QWidget*) calls
            quint64 m_polishSheet = 0;              //!<   ... of which the style of the widget was a style sheet style
            quint64 m_unpolish = 0;                 //!< Style::unpolish(QWidget*) calls
            quint64 m_unpolishSheet = 0;            //!<   ... of which the style of the widget was a style sheet style
            quint64 m_polishNanoseconds = 0;        //!< time inside Style::polish, the outermost call only
            quint64 m_unpolishNanoseconds = 0;      //!< time inside Style::unpolish, the outermost call only
            quint64 m_evPolish = 0;                 //!< QEvent::Polish delivered to a widget (once in its life)
            quint64 m_evPolishOwnSheet = 0;         //!<   ... of which the widget had a style sheet of its own (read by the filter)
            quint64 m_evParent = 0;                 //!< QEvent::ParentChange delivered to a widget
            quint64 m_evParentSheet = 0;            //!<   ... of which Qt::WA_StyleSheet
            quint64 m_evStyle = 0;                  //!< QEvent::StyleChange delivered to a widget
            quint64 m_evStyleSheet = 0;             //!<   ... of which Qt::WA_StyleSheet
            quint64 m_evPolishRequest = 0;          //!< QEvent::PolishRequest delivered to a widget
            quint64 m_repolishHelper = 0;           //!< executions of StyleHelpers::repolishWhenPropertyChanges
            quint64 m_repolishHelperWidgets = 0;    //!<   ... widgets they unpolished and polished (the widget and its children)
            quint64 m_widgetsAlive = 0;             //!< QApplication::allWidgets().size() when read: a level, not cumulative

            bool operator==(const Counters&) const = default;
        };

        //! The counts of one widget class, as the "!classes" line lists them
        struct ClassCounts
        {
            QString m_name;                         //!< QMetaObject::className()
            quint64 m_polish = 0;                   //!< Style::polish calls for widgets of the class
            quint64 m_polishSheet = 0;              //!<   ... of which the style of the widget was a style sheet style
            quint64 m_evParent = 0;                 //!< ParentChange events (one is sent while the widget is constructed, when it
                                                    //!< has a parent: then the class is still QWidget)
            quint64 m_evStyle = 0;                  //!< StyleChange events (same)
        };

        // ------------------------------------------------------------------------------
        // Install
        // ------------------------------------------------------------------------------

        //! Installs the counters on the application if O3DE_UI_PERF_COUNTERS is set and is not "0". The variable is read here,
        //! once. Off (the default), no hook costs more than one read of a pointer, no event filter exists and nothing is written.
        static void installIfRequested(QApplication* application);

        //! Installs the counters whatever the environment says (the test seam of installIfRequested). The application owns them;
        //! they go with it. Nothing happens if they are installed already.
        static void install(QApplication* application);

        static bool isInstalled();

        // ------------------------------------------------------------------------------
        // Reading (GUI thread)
        // ------------------------------------------------------------------------------

        //! The counters now, all zero while they are not installed
        static Counters counters();

        //! The per class counts (in no particular order), empty while the counters are not installed
        static QList<ClassCounts> classCounts();

        //! Test seam: while set, the snapshot lines are appended to the list instead of being written to the log
        static void captureLinesInto(QStringList* lines);

        // ------------------------------------------------------------------------------
        // The lines (pure functions of their arguments)
        // ------------------------------------------------------------------------------

        //! "GuiPerfCounters: v=1 tag=<tag> polish=<n> ..." with the keys in the order of Counters. Times are microseconds, one decimal.
        static QString formatLine(const QString& tag, const Counters& values);

        //! "GuiPerfCountersClasses: v=1 tag=<tag> <Class>=<polish>/<polish_sheet>/<ev_parent>/<ev_style> ..." for the 20 classes
        //! with the most polish calls (ties by name)
        static QString formatClassesLine(const QString& tag, QList<ClassCounts> classes);

        // ------------------------------------------------------------------------------
        // Hooks
        // ------------------------------------------------------------------------------

        //! The first line of Style::polish(QWidget*): counts the call, and times it if it is the outermost polish (a polish nested
        //! in another one is counted but its time is inside the outer one). Safe on every way out of the function. A call that the
        //! recursion guard of Style::polish turns away (the same widget, inside its own polish) is a call and is counted.
        class AZ_QT_COMPONENTS_API PolishScope
        {
        public:
            explicit PolishScope(const QWidget* widget);
            ~PolishScope();
            Q_DISABLE_COPY(PolishScope)

        private:
            bool m_counted = false;     //!< the counters were on, and this is the GUI thread
            bool m_outermost = false;   //!< no other polish was running: this one is timed
            QElapsedTimer m_timer;
        };

        //! The first line of Style::unpolish(QWidget*), as PolishScope is for polish
        class AZ_QT_COMPONENTS_API UnpolishScope
        {
        public:
            explicit UnpolishScope(const QWidget* widget);
            ~UnpolishScope();
            Q_DISABLE_COPY(UnpolishScope)

        private:
            bool m_counted = false;
            bool m_outermost = false;
            QElapsedTimer m_timer;
        };

        //! One execution of StyleHelpers::repolishWhenPropertyChanges that unpolished and polished widgetCount widgets (the
        //! widget and its children)
        static void onRepolishHelper(qsizetype widgetCount);
    };
} // namespace AzQtComponents
