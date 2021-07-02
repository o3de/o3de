/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_UNSUPPORTED_DATAAGGREGATOR_H
#define DRILLER_UNSUPPORTED_DATAAGGREGATOR_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/DrillerAggregatorOptions.hxx"

#include "UnsupportedDataParser.h"

#include <Source/Driller/DrillerDataTypes.h>
#endif

namespace Driller
{
    /**
     * Unsupported driller data drilling aggregator.
     */
    class UnsupportedDataAggregator : public Aggregator
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(UnsupportedDataAggregator,AZ::SystemAllocator,0);

        UnsupportedDataAggregator(AZ::u32 drillerId);

        virtual AZ::u32 GetDrillerId() const    { return m_parser.GetDrillerId(); }
        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser()         { return &m_parser; }

        static const char* ChannelName() { return "Unsupported"; }
        AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*){}
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider*){}
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*){}

    //////////////////////////////////////////////////////////////////////////
    // Aggregator
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

    public:
        UnsupportedHandlerParser            m_parser;            ///< Parser for this aggregator
    };

}

#endif
