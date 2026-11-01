/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QObject>
#include <QString>
#include <QStringList>
#endif

class QApplication;
class QWidget;

namespace AzQtComponents
{
    //! Capture harness for the GUI flatten (A/B screens + style resolution). Not product code:
    //! nothing happens unless O3DE_UI_CAPTURE_DIR is set.
    //!
    //! Triggers
    //!  - auto (default): O3DE_UI_CAPTURE_DELAY_MS after startup (default 20000) every visible
    //!    top-level window is settled, grabbed and recorded into O3DE_UI_CAPTURE_DIR; the app quits
    //!    afterwards unless O3DE_UI_CAPTURE_QUIT=0. O3DE_UI_CAPTURE_AUTO=0 turns this off.
    //!  - request: setting the dynamic property "o3deUiCapture" on a widget to a base path records
    //!    that widget; "o3deUiWalk" walks it (scripted circuits, e.g. the Editor Python circuit).
    //!
    //! walk() captures every menu bar menu (+ one level of submenus), every other tab (+ one level
    //! of nested tab widgets), every other stacked page and every hidden dock of a window, each
    //! settled, grabbed and recorded as <base>__menu-* / __tab-* / __page-* / __dock-*. No action
    //! is triggered and every state is restored. Auto mode walks each window unless
    //! O3DE_UI_CAPTURE_WALK=0.
    //!
    //! record() writes <base>.widgets.json (every visible widget: class chain, object name, the
    //! properties stylesheet selectors test, state flags, palette, font, own sheet) and appends a
    //! census line (widgets with a sheet / widgets drawn through QStyleSheetStyle) to __sheets.txt.
    //! O3DE_UI_CAPTURE_ATTRIBUTES names a file of property names to record (one per line) in
    //! addition to every dynamic property.
    class AZ_QT_COMPONENTS_API UiCaptureHarness
        : public QObject
    {
        Q_OBJECT

    public:
        static void installIfRequested(QApplication* application);

        static void settle(QWidget* widget);
        static void record(QWidget* root, const QString& basePath);
        static QStringList walk(QWidget* window, const QString& basePath);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        explicit UiCaptureHarness(QObject* parent, const QString& outputDir);

        void captureTopLevelWindows();

        QString m_outputDir;
    };
} // namespace AzQtComponents
