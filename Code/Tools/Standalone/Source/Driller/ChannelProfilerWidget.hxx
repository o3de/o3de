/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifndef DRILLER_CHANNELPROFILERWIDGET_H
#define DRILLER_CHANNELPROFILERWIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>

#include "Source/Driller/DrillerDataTypes.h"

// Generated Files
#include <Source/Driller/ui_ChannelProfilerWidget.h>
#endif

namespace Driller
{
    class Aggregator;
    class ChannelControl;    
    class CSVExportSettings;
    class ChannelConfigurationWidget;
    
    class ChannelProfilerWidget
        : public QWidget
        , private Ui::ChannelProfilerWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ChannelProfilerWidget, AZ::SystemAllocator,0);
        
        ChannelProfilerWidget(ChannelControl* channelControl, Aggregator* aggregator);
        virtual ~ChannelProfilerWidget();

        Aggregator* GetAggregator() const;
        
        bool IsActive() const;
        void SetIsActive(bool isActive);

        QString GetName() const;
        AZ::Uuid GetID();

        ChannelConfigurationWidget* CreateConfigurationWidget();
        
    public slots:
        void OnActivationToggled();
        void SetCaptureMode(CaptureMode mode);
        
        void OnDrillDown();
        void OnDrillDestroyed(QObject* drill);
        
        void OnExportToCSV();        
    
    signals:
        void OnActivationChanged(ChannelProfilerWidget*,bool activated);
        
        QWidget* DrillDownRequest(FrameNumberType atFrame);
        void ExportToCSVRequest(const char* filename, CSVExportSettings* customizeWidget);
        
        void OnSuccessfulDrillDown(QWidget* widget);
    
    private:

        bool AllowCSVExport() const;
        void UpdateActivationIcon();
        
        bool IsInCaptureMode(CaptureMode captureMode) const;
        void ConfigureUI();        
        
        ChannelControl* m_channelControl;
        QWidget*        m_drilledWidget;
        Aggregator*     m_aggregator;
    
        CaptureMode m_captureMode;
        bool m_isActive;

        QImage m_inactiveImage;
        QImage m_activeImage;
        QIcon m_activeIcon;
        QIcon m_inactiveIcon;
    };
}

#endif
