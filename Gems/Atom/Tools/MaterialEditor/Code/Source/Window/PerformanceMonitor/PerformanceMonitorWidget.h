/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
