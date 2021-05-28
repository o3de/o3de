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

#if !defined(Q_MOC_RUN)
#include <Source/Driller/DrillerAggregator.hxx>
#include <Source/Driller/DrillerAggregatorOptions.hxx>
#include <Source/Driller/GenericCustomizeCSVExportWidget.hxx>
#include "EventTraceDataParser.h"

#include <AzCore/JSON/document.h>
#include <AzCore/RTTI/RTTI.h>

#include <QProcess>
#endif

namespace Driller
{
    class EventTraceDataAggregator
        : public Aggregator
    {
        Q_OBJECT;
    public:
        AZ_RTTI(EventTraceDataAggregator, "{D82CC9CF-5477-4A3E-8809-C064D19963F8}");
        AZ_CLASS_ALLOCATOR(EventTraceDataAggregator, AZ::SystemAllocator, 0);

        EventTraceDataAggregator(int identity = 0);
        virtual ~EventTraceDataAggregator();

        static AZ::u32 DrillerId()
        {
            return EventTraceDataParser::GetDrillerId();
        }

        AZ::u32 GetDrillerId() const override
        {
            return DrillerId();
        }

        static const char* ChannelName()
        {
            return "ChromeTracing";
        }

        AZ::Crc32 GetChannelId() const override
        {
            return AZ::Crc32(ChannelName());
        }

        AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() override
        {
            return &m_parser;
        }

        bool CanExportToCSV() const override
        {
            return true;
        }

        void ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings) override;

        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*) override {}
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*) override {}
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*) override {}

    public slots:
        float ValueAtFrame(FrameNumberType frame) override;
        QColor GetColor() const override;
        QString GetChannelName() const override;
        QString GetName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        void OptionsRequest()  override {}

        QWidget* DrillDownRequest(FrameNumberType frame) override;

    private:
        rapidjson::Document MakeJsonRepresentation(FrameNumberType frameStart, FrameNumberType frameEnd) const;
        void ExportChromeTrace(AZ::IO::SystemFile& file, FrameNumberType frameStart, FrameNumberType frameEnd) const;
        void ExportChromeTrace(const QString& filename, FrameNumberType frameStart, FrameNumberType frameEnd) const;

        EventTraceDataParser m_parser;
    };
}
