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
