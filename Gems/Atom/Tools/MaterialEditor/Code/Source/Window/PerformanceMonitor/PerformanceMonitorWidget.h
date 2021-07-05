/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <QTimer>

namespace Ui
{
    class PerformanceMonitorWidget;
}

namespace MaterialEditor
{
    //! Displays performance metrics for Material Editor
    class PerformanceMonitorWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        PerformanceMonitorWidget(QWidget* parent = nullptr);
        ~PerformanceMonitorWidget();

    private slots:
        void UpdateMetrics();

    private:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

        QScopedPointer<Ui::PerformanceMonitorWidget> m_ui;
        QTimer m_updateTimer;


        //! interval to request performance metrics
        static constexpr int UpdateIntervalMs = 1000;
    };
} // namespace MaterialEditor
