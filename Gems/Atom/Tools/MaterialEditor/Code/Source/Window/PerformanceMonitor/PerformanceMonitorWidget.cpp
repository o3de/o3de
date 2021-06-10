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

#include <Atom/Viewport/PerformanceMonitorRequestBus.h>

#include <Source/Window/PerformanceMonitor/PerformanceMonitorWidget.h>
#include <Source/Window/PerformanceMonitor/ui_PerformanceMonitorWidget.h>

#include <QTimer>

namespace MaterialEditor
{
    PerformanceMonitorWidget::PerformanceMonitorWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::PerformanceMonitorWidget)
    {
        m_ui->setupUi(this);


        m_updateTimer.setInterval(UpdateIntervalMs);
        connect(&m_updateTimer, &QTimer::timeout, this, &PerformanceMonitorWidget::UpdateMetrics);
    }

    PerformanceMonitorWidget::~PerformanceMonitorWidget() = default;

    void PerformanceMonitorWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);

        m_updateTimer.start();
        PerformanceMonitorRequestBus::Broadcast(&PerformanceMonitorRequestBus::Handler::SetProfilerEnabled, true);
    }

    void PerformanceMonitorWidget::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);

        m_updateTimer.stop();
        PerformanceMonitorRequestBus::Broadcast(&PerformanceMonitorRequestBus::Handler::SetProfilerEnabled, false);
    }

    void PerformanceMonitorWidget::UpdateMetrics()
    {
        PerformanceMetrics metrics;
        PerformanceMonitorRequestBus::BroadcastResult(metrics, &PerformanceMonitorRequestBus::Handler::GetMetrics);
        m_ui->m_cpuFrameTimeValue->setText(QString("%1 ms").arg(QString::number(metrics.m_cpuFrameTimeMs, 'f', 2)));
        m_ui->m_gpuFrameTimeValue->setText(QString("%1 ms").arg(QString::number(metrics.m_gpuFrameTimeMs, 'f', 2)));
        int frameRate = metrics.m_cpuFrameTimeMs > 0 ? aznumeric_cast<int>(1000 / metrics.m_cpuFrameTimeMs) : 0;
        m_ui->m_frameRateValue->setText(QString::number(frameRate));
    }
} // namespace MaterialEditor

#include <Source/Window/PerformanceMonitor/moc_PerformanceMonitorWidget.cpp>
