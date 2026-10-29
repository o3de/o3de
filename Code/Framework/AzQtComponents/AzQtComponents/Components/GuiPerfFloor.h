/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QString>
#include <QStringList>

class QApplication;
class QWidget;

namespace AzQtComponents
{
    //! GUI performance floor (GuiPerfFloor v=1): how close to "no styling" the GUI can be run. Not product code: nothing changes
    //! unless the application is started at the noop or passthrough rung of the ladder below.
    //!
    //!   rung         flag                    environment variable      what runs
    //!   qss          (none)                  (none)                    legacy: the application sheet and the sheets of the widgets
    //!   flat         --disable-stylesheets   O3DE_DISABLE_STYLESHEETS  no sheet of StyleManager; our Style and its elements
    //!   noop         --stylesheets-noop      O3DE_STYLESHEETS_NOOP     flat, and the inline sheets of widgets do not take effect
    //!   passthrough  --style-passthrough     O3DE_STYLE_PASSTHROUGH    noop, and our Style hands every call to its base style
    //!
    //! A rung implies the ones above it: --style-passthrough alone is passthrough, and StyleManager::stylesheetsDisabled() is true at
    //! flat, noop and passthrough. A flag is matched like --disable-stylesheets (QCoreApplication::arguments(), without regard to
    //! case); the two new variables are on when they are set to anything but "0", and O3DE_DISABLE_STYLESHEETS keeps the meaning it
    //! always had (on when set). The rung is read once and kept.
    //!
    //! The distance from one rung to the next is what the rung takes away: flat from qss is what the flattening gained; noop from
    //! flat is what the inline sheets cost; passthrough from noop is what our Style costs, so passthrough is the nearest the GUI gets
    //! to no styling. Passthrough LOOKS like plain Fusion: it is a measuring instrument, not a theme.
    //!
    //! Qt cannot switch the inline sheets off (QWidget::setStyleSheet is not virtual), so noop removes them in two ways:
    //!  - setInlineStyleSheet, for the sites that make a sheet in every selection (a card header, a vector label, a prefab label):
    //!    the call does nothing at noop and passthrough, and the widget never gets a QStyleSheetStyle;
    //!  - an application wide event filter for every other site (.ui files, gems, tools): it clears the sheet a widget carries when
    //!    the widget is polished for the first time, and, a moment after it happened, the sheet a polished widget was given later.
    //!    What stays is the QStyleSheetStyle made when the sheet was set, until the filter clears it. The filter also takes the mark of
    //!    the sheet style (Qt::WA_StyleSheet) off the widgets around the one it cleared, so that they follow the font and the palette of
    //!    their parent as widgets that never had a sheet do.
    //!
    //! What it reports (AZ_TracePrintf, window "GuiPerfFloor"; a reader finds the marker "GuiPerfFloor: v=1" anywhere in a log line
    //! and ignores keys it does not know):
    //!  - once at startup, at every rung:
    //!        GuiPerfFloor: v=1 rung=<qss|flat|noop|passthrough> stylesheets_disabled=<0|1>
    //!    The application also carries the rung as the dynamic property "_o3de_floor_rung", for a reader that cannot find the line (the
    //!    Editor makes its QApplication before ToolsApplication::Start opens Editor.log).
    //!  - at noop and passthrough, when the dynamic property "_o3de_perf_snapshot" of the application is set to a tag (the property
    //!    GuiPerfCounters reads), one line, cumulative:
    //!        GuiPerfFloor: v=1 tag=<tag> rung=<noop|passthrough> stripped=<n> stripped_late=<n> helper_skipped=<n>
    class AZ_QT_COMPONENTS_API GuiPerfFloor
    {
    public:
        GuiPerfFloor() = delete;

        // ------------------------------------------------------------------------------
        // The ladder
        // ------------------------------------------------------------------------------

        //! In the order of the ladder: a rung implies the ones before it
        enum class Rung
        {
            Qss,
            Flat,
            Noop,
            Passthrough,
        };

        //! The rung of this process. The flags and variables are read by the first call and the answer is kept (a test may
        //! set it with setRungForTesting).
        static Rung rung();

        //! The rung the flags and variables of this process ask for, read now: rung() keeps the first answer of this
        static Rung readRung();

        //! "qss", "flat", "noop" or "passthrough"
        static const char* rungName(Rung rung);

        //! Noop or passthrough: the inline style sheets of widgets do not take effect
        static bool inlineSheetsDisabled();

        //! Passthrough: our Style hands every call to its base style, and the widgets leave their own styling code out
        static bool isPassthrough();

        // ------------------------------------------------------------------------------
        // Inline style sheets
        // ------------------------------------------------------------------------------

        //! widget->setStyleSheet(sheet), exactly as before, below noop; at noop and passthrough it does nothing (and the widget
        //! keeps the style of the application). For a site that sets a sheet in every selection or for every widget it builds.
        static void setInlineStyleSheet(QWidget* widget, const QString& sheet);

        // ------------------------------------------------------------------------------
        // Install
        // ------------------------------------------------------------------------------

        //! Called by StyleManager::initialize, before GuiPerfCounters::installIfRequested: writes the startup line, sets the rung
        //! property of the application and, at noop and passthrough, installs the filter that clears the inline sheets. The filter
        //! goes in BEFORE the counters on purpose: Qt asks the filter installed last first, so the counters still see an inline
        //! sheet when the widget is polished (ev_polish_own_sheet) and this filter then clears it (stripped).
        static void initialize(QApplication* application);

        //! Installs the filter whatever the rung says (the test seam of initialize). The application owns it, and it goes with it.
        //! Nothing happens if it is installed already.
        static void install(QApplication* application);

        static bool isInstalled();

        // ------------------------------------------------------------------------------
        // Reading
        // ------------------------------------------------------------------------------

        //! What the snapshot line carries. Cumulative from the start of the process, never reset (a reader subtracts two snapshots).
        struct Counts
        {
            quint64 m_stripped = 0;         //!< inline sheets cleared when their widget was polished for the first time
            quint64 m_strippedLate = 0;     //!< inline sheets cleared after their widget was polished (they were set late)
            quint64 m_helperSkipped = 0;    //!< setInlineStyleSheet calls that did nothing

            bool operator==(const Counts&) const = default;
        };

        static Counts counts();

        // ------------------------------------------------------------------------------
        // The lines (pure functions of their arguments)
        // ------------------------------------------------------------------------------

        //! "GuiPerfFloor: v=1 rung=<rung> stylesheets_disabled=<0|1>"
        static QString formatStartupLine(Rung rung, bool stylesheetsDisabled);

        //! "GuiPerfFloor: v=1 tag=<tag> rung=<rung> stripped=<n> stripped_late=<n> helper_skipped=<n>"
        static QString formatSnapshotLine(const QString& tag, Rung rung, const Counts& values);

        // ------------------------------------------------------------------------------
        // Test seams
        // ------------------------------------------------------------------------------

        //! Makes rung() answer with this rung, whatever the flags and variables say
        static void setRungForTesting(Rung rung);

        //! Forgets the rung and the counts: the next rung() reads the flags and variables again
        static void resetForTesting();

        //! While set, the lines are appended to the list instead of being written to the log
        static void captureLinesInto(QStringList* lines);
    };
} // namespace AzQtComponents
