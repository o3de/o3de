/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_TRACE_MESSAGE_DATAAGGREGATOR_H
#define DRILLER_TRACE_MESSAGE_DATAAGGREGATOR_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/DrillerAggregatorOptions.hxx"

#include "TraceMessageDataParser.h"

#include <AzCore/RTTI/RTTI.h>
#endif

namespace AZ { class ReflectContext; }

namespace Driller
{
    class TraceMessageDataAggregatorSavedState;

    /**
     * Trace message driller data aggregator.
     */
    class TraceMessageDataAggregator : public Aggregator
    {
        Q_OBJECT;
        TraceMessageDataAggregator(const TraceMessageDataAggregator&) = delete;
    public:
        AZ_RTTI(TraceMessageDataAggregator, "{CA33E0B0-6E16-4D8C-B3D0-C833AC8574C6}");
        AZ_CLASS_ALLOCATOR(TraceMessageDataAggregator,AZ::SystemAllocator,0);

        TraceMessageDataAggregator(int identity = 0);

        static AZ::u32 DrillerId()                                            { return TraceMessageHandlerParser::GetDrillerId(); }
        virtual AZ::u32 GetDrillerId() const                                { return DrillerId(); }

        static const char* ChannelName() { return "Logging"; }
        AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser()     { return &m_parser; }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

    //////////////////////////////////////////////////////////////////////////
    // Aggregator

        // emit all annotations that match the provider's filter, given the start and end frame:
        virtual void EmitAllAnnotationsForFrameRange( int startFrameInclusive, int endFrameInclusive , AnnotationsProvider* ptrProvider);

        // emit all channels that you are aware of existing within that frame range (You may emit duplicate channels, they will be ignored)
        virtual void EmitAnnotationChannelsForFrameRange( int startFrameInclusive, int endFrameInclusive , AnnotationsProvider* ptrProvider);

    public slots:
        float ValueAtFrame( FrameNumberType frame ) override;
        QColor GetColor() const override;        
        QString GetName() const override;
        QString GetChannelName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void OptionsRequest() override;
        virtual void OnDataViewDestroyed(QObject*);

    public:
        TraceMessageHandlerParser            m_parser;            ///< Parser for this aggregator

        QObject*                                                    m_dataView;                
        AZStd::intrusive_ptr<TraceMessageDataAggregatorSavedState>    m_persistentState;

    public:
        static void Reflect(AZ::ReflectContext* context);
    };

}

#endif
