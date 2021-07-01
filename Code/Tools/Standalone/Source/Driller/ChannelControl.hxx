/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CHANNEL_CONTROL_H
#define CHANNEL_CONTROL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>

#include "DrillerNetworkMessages.h"

#include "Source/Driller/DrillerDataTypes.h"

// Generated Files
#include <Source/Driller/ui_ChannelControl.h>
#endif

namespace Driller
{
    class AnnotationsProvider;    
    class ChannelConfigurationDialog;

    /*
    Channel Control intermediates between one data Aggregator, the application's main window, and the renderer.
    This maintains state used by the renderer and passes changes both up and down via signal/slot.
    */

    class ChannelControl
        : public QWidget
        , private Ui::ChannelControl
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ChannelControl,AZ::SystemAllocator,0);
        ChannelControl( const char* channelName, AnnotationsProvider* ptrAnnotations, QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~ChannelControl(void);

        struct 
        {            
            FrameNumberType m_EndFrame;
            FrameNumberType m_FramesInView;
            int m_ContractedHeight;
            FrameNumberType m_ScrubberFrame;
            FrameNumberType m_FrameOffset;
            FrameNumberType m_LoopBegin;
            FrameNumberType m_LoopEnd;
        } m_State;

        bool m_isLive;

        QList<QWidget*> m_OpenDrills;
        QList<QPoint> m_OpenDrillsPositions;

        bool IsSetup() const;
        void SignalSetup();

        bool IsActive() const;
        const AZStd::list< ChannelProfilerWidget* >& GetProfilers() const;        

        // Temporary solution, since eventually we'll want to display all of the information
        // we have at once. For now, since we can only have one. Get a "main" window.
        ChannelProfilerWidget* GetMainProfiler() const;
        ChannelProfilerWidget* AddAggregator(Aggregator* aggregator);
        bool RemoveAggregator(Aggregator* aggregator);

        void SetAllProfilersEnabled(bool enabled);        
        AZ::Crc32 GetChannelId() const;
        
        void SetDataPointsInView( FrameNumberType count );
        void SetEndFrame( FrameNumberType frame );
        void SetSliderOffset( FrameNumberType frame );
        void SetLoopBegin( FrameNumberType frame );
        void SetLoopEnd( FrameNumberType frame );

        bool IsInCaptureMode(CaptureMode captureMode) const;

        public slots:
            void SetCaptureMode(CaptureMode captureMode);
            void OnContractedToggled( bool toggleState );            
            void OnSuccessfulDrillDown(QWidget* drillerWidget);            
            void SetScrubberFrame( FrameNumberType frame );
            void OnDrillDestroyed(QObject *drill);
            void OnShowCommand();
            void OnHideCommand();
            void OnRefreshView();
            void OnActivationChanged(ChannelProfilerWidget*,bool activated);
            void OnConfigureChannel();
            void OnDialogClosed(QDialog* dialog);

            void OnConfigurationChanged();
            void OnNormalizedRangeChanged();

        signals:
            
            void OnCaptureModeChanged(CaptureMode captureMode);
            void RequestScrollToFrame( FrameNumberType frame );
            void InformOfMouseClick(Qt::MouseButton button, FrameNumberType frame, FrameNumberType range, int modifiers );
            void InformOfMouseMove( FrameNumberType frame, FrameNumberType range, int modifiers );
            void InformOfMouseRelease(Qt::MouseButton button, FrameNumberType frame, FrameNumberType range, int modifiers );
            void InformOfMouseWheel( FrameNumberType frame, int wheelAmount, FrameNumberType range, int modifiers );
            QWidget* DrillDownRequest(FrameNumberType atFrame);
            void OptionsRequest();
            void ExpandedContracted();

            void AddConfigurationWidgets(QLayout* configurationLayout);
            
            QString GetInspectionFileName() const;

    private:

        void ConnectProfilerWidget(ChannelProfilerWidget* profilerWidget);
        void ConfigureUI();

        bool m_isSetup;

        CaptureMode m_captureMode;
        AZ::Crc32 m_channelId;
        AZStd::list< ChannelProfilerWidget* > m_profilerWidgets;

        ChannelConfigurationDialog* m_configurationDialog;
    };
}


#endif // CHANNEL_CONTROL_H
