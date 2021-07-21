/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include "Source/Driller/ChannelProfilerWidget.hxx"
#include <Source/Driller/moc_ChannelProfilerWidget.cpp>

#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/ChannelControl.hxx"
#include "Source/Driller/CollapsiblePanel.hxx"
#include "Source/Driller/CustomizeCSVExportWidget.hxx"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"

#include <QPainter>
#include <QColor>
#include <QFileDialog>

namespace Driller
{
    void ColorizeIcon(QIcon& icon, const char* iconPath, Aggregator* aggregator)
    {
        QImage alphaImage(iconPath);
        alphaImage = alphaImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QImage colorizedImage(alphaImage.width(), alphaImage.height(), QImage::Format_ARGB32_Premultiplied);

        QColor color = aggregator->GetColor();
        color.setAlphaF(1.0f);

        QPainter painter;
        painter.begin(&colorizedImage);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(colorizedImage.rect(), color);
        painter.end();

        colorizedImage.setAlphaChannel(alphaImage);

        icon.addPixmap(QPixmap::fromImage(colorizedImage));
    }

    ChannelProfilerWidget::ChannelProfilerWidget(ChannelControl* channelControl, Aggregator* aggregator)
        : QWidget(channelControl)
        , m_channelControl(channelControl)
        , m_drilledWidget(nullptr)
        , m_aggregator(aggregator)
        , m_captureMode(CaptureMode::Unknown)
        , m_isActive(true)
        , m_activeIcon()
        , m_inactiveIcon()
    {
        setupUi(this);

        connect(this, SIGNAL(DrillDownRequest(FrameNumberType)), m_aggregator, SLOT(DrillDownRequest(FrameNumberType)));
        connect(this, SIGNAL(ExportToCSVRequest(const char*, CSVExportSettings*)), m_aggregator, SLOT(ExportToCSVRequest(const char*, CSVExportSettings*)));

        connect(m_aggregator, SIGNAL(GetInspectionFileName()), m_channelControl, SIGNAL(GetInspectionFileName()));

        connect(profilerName, SIGNAL(clicked()), this, SLOT(OnActivationToggled()));
        connect(enableChannel, SIGNAL(clicked()), this, SLOT(OnActivationToggled()));
        //connect(channelOptions,SIGNAL(clicked()),this, SLOT(OnConfigureChannel()));
        connect(drillDown, SIGNAL(clicked()), this, SLOT(OnDrillDown()));
        connect(exportData, SIGNAL(clicked()), this, SLOT(OnExportToCSV()));

        ColorizeIcon(m_activeIcon, ":/driller/active_color_swatch", aggregator);
        ColorizeIcon(m_inactiveIcon, ":/driller/inactive_color_swatch", aggregator);

        profilerName->setToolTip(aggregator->GetToolTip());
        profilerName->setText(GetName());

        UpdateActivationIcon();
        ConfigureUI();
    }

    ChannelProfilerWidget::~ChannelProfilerWidget()
    {
    }

    Aggregator* ChannelProfilerWidget::GetAggregator() const
    {
        return m_aggregator;
    }

    bool ChannelProfilerWidget::IsActive() const
    {
        return m_isActive;
    }

    void ChannelProfilerWidget::SetIsActive(bool isActive)
    {
        if (m_isActive != isActive)
        {
            m_isActive = isActive;
            UpdateActivationIcon();

            if (m_aggregator && m_captureMode == Driller::CaptureMode::Configuration)
            {
                m_aggregator->EnableCapture(isActive);
            }

            emit OnActivationChanged(this, m_isActive);
        }
    }

    QString ChannelProfilerWidget::GetName() const
    {
        return (m_aggregator ? m_aggregator->GetName() : "Unknown Profiler");
    }

    AZ::Uuid ChannelProfilerWidget::GetID()
    {
        return (m_aggregator ? m_aggregator->GetID() : AZ::Uuid::CreateNull());
    }

    ChannelConfigurationWidget* ChannelProfilerWidget::CreateConfigurationWidget()
    {
        ChannelConfigurationWidget* configurationWidget = nullptr;

        if (m_aggregator)
        {
            configurationWidget = m_aggregator->CreateConfigurationWidget();
        }

        return configurationWidget;
    }
    
    void ChannelProfilerWidget::OnActivationToggled()
    {
        SetIsActive(!IsActive());
    }

    void ChannelProfilerWidget::SetCaptureMode(CaptureMode captureMode)
    {
        if (m_captureMode != captureMode)
        {
            m_captureMode = captureMode;

            ConfigureUI();
        }
    }

    void ChannelProfilerWidget::OnDrillDown()
    {
        if (m_channelControl && IsInCaptureMode(CaptureMode::Inspecting))
        {
            char traceStr[AZ::Uuid::MaxStringBuffer];
            m_aggregator->GetID().ToString(traceStr, AZ_ARRAY_SIZE(traceStr));
            AZ_TracePrintf("Driller", "Drill Down ID = %s\n", traceStr);

            if (m_drilledWidget == nullptr)
            {
                m_drilledWidget = emit DrillDownRequest(m_channelControl->m_State.m_ScrubberFrame);
                if (m_drilledWidget)
                {
                    connect(m_drilledWidget, SIGNAL(destroyed(QObject*)), this, SLOT(OnDrillDestroyed(QObject*)));
                    emit OnSuccessfulDrillDown(m_drilledWidget);
                }
            }
            else
            {
                if (m_drilledWidget->isMinimized())
                {
                    m_drilledWidget->showNormal();
                }

                m_drilledWidget->raise();
                m_drilledWidget->activateWindow();
            }
        }
    }

    void ChannelProfilerWidget::OnDrillDestroyed(QObject* widget)
    {
        if (widget == m_drilledWidget)
        {
            m_drilledWidget = nullptr;
        }
    }

    void ChannelProfilerWidget::OnExportToCSV()
    {
        DrillerOperationTelemetryEvent exportToCSVEvent;
        exportToCSVEvent.SetAttribute("ExportToCSV", m_aggregator->GetName().toStdString().c_str());
        exportToCSVEvent.Log();

        char traceStr[AZ::Uuid::MaxStringBuffer];
        m_aggregator->GetID().ToString(traceStr, AZ_ARRAY_SIZE(traceStr));
        AZ_TracePrintf("Driller", "Export Request for ID = %s\n", traceStr);

        QFileDialog fileDialog;

        CustomizeCSVExportWidget* customizeWidget = m_aggregator->CreateCSVExportCustomizationWidget();

        // Always use the Qt dialog for consistency
        fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        fileDialog.setWindowTitle(QString("Export %1 To CSV").arg(m_aggregator->GetName()));
        fileDialog.setNameFilter("CSV (*.csv)");
        fileDialog.setDefaultSuffix("csv");

        if (customizeWidget)
        {
            CollapsiblePanel* collapsiblePanel = aznew CollapsiblePanel(&fileDialog);

            collapsiblePanel->SetTitle("Customize");
            collapsiblePanel->SetContent(customizeWidget);

            // I don't know why the decided to use a GridLayout here instead of nested layouts. But whatever.
            QGridLayout* gridLayout = static_cast<QGridLayout*>(fileDialog.layout());
            int numRows = gridLayout->rowCount();

            gridLayout->addWidget(collapsiblePanel, numRows, 0, 1, gridLayout->columnCount());
        }

        QString fileName;
        if (fileDialog.exec())
        {
            QStringList fileList = fileDialog.selectedFiles();

            for (const QString& file : fileList)
            {
                if (!file.isEmpty())
                {
                    fileName = file;
                    break;
                }
            }
        }

        if (!fileName.isEmpty())
        {
            CSVExportSettings* exportSettings = nullptr;

            if (customizeWidget)
            {
                customizeWidget->FinalizeSettings();
                exportSettings = customizeWidget->GetExportSettings();
            }

            emit ExportToCSVRequest(fileName.toStdString().c_str(), exportSettings);
        }
    }

    bool ChannelProfilerWidget::AllowCSVExport() const
    {
        return (m_aggregator ? m_aggregator->CanExportToCSV() : false);
    }

    void ChannelProfilerWidget::UpdateActivationIcon()
    {
        if (IsActive())
        {
            enableChannel->setIcon(m_activeIcon);
        }
        else
        {
            enableChannel->setIcon(m_inactiveIcon);
        }
    }

    bool ChannelProfilerWidget::IsInCaptureMode(CaptureMode captureMode) const
    {
        return m_captureMode == captureMode;
    }

    void ChannelProfilerWidget::ConfigureUI()
    {
        switch (m_captureMode)
        {
        case CaptureMode::Configuration:
            enableChannel->setEnabled(true);
            drillDown->setVisible(false);
            exportData->setVisible(false);
            break;
        case CaptureMode::Capturing:
            enableChannel->setEnabled(false);
            drillDown->setVisible(false);
            exportData->setVisible(false);
            break;
        case CaptureMode::Inspecting:
            enableChannel->setEnabled(true);
            drillDown->setVisible(true);
            exportData->setVisible(AllowCSVExport());
            exportData->setEnabled(AllowCSVExport());
            break;
        default:
            break;
        }
    }
}
