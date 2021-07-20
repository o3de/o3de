/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <Source/Driller/Workspaces/Workspace.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>

#include "EventTraceDataAggregator.h"
#include "EventTraceEvents.h"

#include <Source/Driller/EventTrace/moc_EventTraceDataAggregator.cpp>

#include <QFileInfo>
#include <QCoreApplication>
#include <QFontInfo>
#include <QDir>

namespace Driller
{
    namespace Platform
    {
        void LaunchExplorerSelect(const QString& filePath);
    }

    namespace
    {
        const QString ExportFolder = "EventTrace";
    }

    EventTraceDataAggregator::EventTraceDataAggregator(int identity)
        : Aggregator(identity)
    {
        m_parser.SetAggregator(this);
    }

    EventTraceDataAggregator::~EventTraceDataAggregator()
    {
    }

    float EventTraceDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        const float maxEventsPerFrame = 1000.0f; // just a scale number
        float numEventsPerFrame = static_cast<float>(NumOfEventsAtFrame(frame));
        return AZStd::GetMin(numEventsPerFrame / maxEventsPerFrame, 1.0f) * 2.0f - 1.0f;
    }

    QColor EventTraceDataAggregator::GetColor() const
    {
        return QColor(0, 255, 255);
    }

    QString EventTraceDataAggregator::GetName() const
    {
        return "Chrome Tracing";
    }

    QString EventTraceDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString EventTraceDataAggregator::GetDescription() const
    {
        return "Timed scope driller";
    }

    QString EventTraceDataAggregator::GetToolTip() const
    {
        return "Timed scope event profiler which exports to Chrome Tracing";
    }

    AZ::Uuid EventTraceDataAggregator::GetID() const
    {
        return AZ::Uuid("{3E47A533-55A4-4E36-B420-063270E4D5DF}");
    }

    rapidjson::Document EventTraceDataAggregator::MakeJsonRepresentation(
        FrameNumberType frameBegin,
        FrameNumberType frameEnd) const
    {
        AZ_Assert(frameBegin >= 0 && frameEnd < GetFrameCount() && frameBegin <= frameEnd, "Invalid frame range for chrome trace export");

        rapidjson::Document rootObj(rapidjson::kObjectType);
        auto& allocator = rootObj.GetAllocator();

        rapidjson::Value traceEvents(rapidjson::kArrayType);

        auto addMemberStr = [&allocator](rapidjson::Value& obj, const char* key, const char* str)
            {
                rapidjson::Value k(rapidjson::StringRef(key), allocator);
                rapidjson::Value v(rapidjson::StringRef(str), allocator);
                obj.AddMember(k.Move(), v.Move(), allocator);
            };

        auto addMemberU64 = [&allocator](rapidjson::Value& obj, const char* key, AZ::u64 value)
            {
                rapidjson::Value k(rapidjson::StringRef(key), allocator);
                rapidjson::Value v(static_cast<uint64_t>(value));
                obj.AddMember(k.Move(), v.Move(), allocator);
            };

        auto addMemberObj = [&allocator](rapidjson::Value& obj, const char* key, rapidjson::Value& value)
        {
            rapidjson::Value k(rapidjson::StringRef(key), allocator);
            obj.AddMember(k.Move(), value.Move(), allocator);
        };

        const EventNumberType FrameBeginIndex = GetFirstIndexAtFrame(frameBegin);
        const EventNumberType FrameEndIndex = GetFirstIndexAtFrame(frameEnd) + NumOfEventsAtFrame(frameEnd);

        for (EventNumberType index = FrameBeginIndex; index < FrameEndIndex; ++index)
        {
            const DrillerEvent& event = static_cast<const DrillerEvent&>(*GetEvents()[index]);

            switch (event.GetEventType())
            {
            case EventTrace::ET_SLICE:
            {
                const EventTrace::SliceEvent& slice = static_cast<const EventTrace::SliceEvent&>(event);
                rapidjson::Value obj(rapidjson::kObjectType);
                addMemberStr(obj, "name", slice.m_Name);
                addMemberStr(obj, "cat", slice.m_Category);
                addMemberStr(obj, "ph", "X");
                addMemberU64(obj, "ts", slice.m_Timestamp);
                addMemberU64(obj, "dur", slice.m_Duration);
                addMemberU64(obj, "tid", slice.m_ThreadId);
                addMemberU64(obj, "pid", 0);

                traceEvents.PushBack(obj, allocator);
            } break;

            case EventTrace::ET_INSTANT:
            {
                const EventTrace::InstantEvent& instant = static_cast<const EventTrace::InstantEvent&>(event);

                rapidjson::Value obj(rapidjson::kObjectType);
                addMemberStr(obj, "name", instant.m_Name);
                addMemberStr(obj, "cat", instant.m_Category);
                addMemberStr(obj, "ph", "i");
                addMemberU64(obj, "ts", instant.m_Timestamp);
                addMemberStr(obj, "s", instant.GetScopeName());
                addMemberU64(obj, "tid", instant.m_ThreadId);
                addMemberU64(obj, "pid", 0);

                traceEvents.PushBack(obj, allocator);

            } break;

            case EventTrace::ET_THREAD_INFO:
            {
                const EventTrace::ThreadInfoEvent& threadInfo = static_cast<const EventTrace::ThreadInfoEvent&>(event);
                rapidjson::Value obj(rapidjson::kObjectType);

                rapidjson::Value args(rapidjson::kObjectType);
                addMemberStr(args, "name", threadInfo.m_Name);

                addMemberStr(obj, "name", "thread_name");
                addMemberStr(obj, "ph", "M");
                addMemberU64(obj, "pid", 0);
                addMemberU64(obj, "tid", threadInfo.m_ThreadId);
                addMemberObj(obj, "args", args);

                traceEvents.PushBack(obj, allocator);
            } break;
            }
        }

        addMemberObj(rootObj, "traceEvents", traceEvents);
        return rootObj;
    }

    QWidget* EventTraceDataAggregator::DrillDownRequest(FrameNumberType atFrame)
    {
        const FrameNumberType FrameCountToExport = 10;
        const FrameNumberType FrameCountToExportDiv2 = FrameCountToExport / 2;

        QString filename = "Frame_" + QString::number(atFrame) + ".chrometrace";
        QFileInfo fileInfo(QCoreApplication::applicationDirPath() + "/" + ExportFolder, filename);
        ExportChromeTrace(fileInfo.absoluteFilePath(), atFrame - FrameCountToExportDiv2, atFrame + FrameCountToExportDiv2);
        Platform::LaunchExplorerSelect(fileInfo.absoluteFilePath());
        return nullptr;
    }

    void EventTraceDataAggregator::ExportChromeTrace(const QString& filename, FrameNumberType frameStart, FrameNumberType frameEnd) const
    {
        AZ::IO::SystemFile exportFile;
        if (exportFile.Open(filename.toStdString().c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            ExportChromeTrace(exportFile, frameStart, frameEnd);
        }
        exportFile.Close();
    }

    void EventTraceDataAggregator::ExportChromeTrace(AZ::IO::SystemFile& file, FrameNumberType frameStart, FrameNumberType frameEnd) const
    {
        frameStart = AZStd::max(frameStart, 0);
        frameEnd   = AZStd::min(frameEnd, (FrameNumberType)GetFrameCount() - 1);

        if (frameStart <= frameEnd)
        {
            rapidjson::Document jsonRep = MakeJsonRepresentation(frameStart, frameEnd);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            jsonRep.Accept(writer);

            file.Write(buffer.GetString(), buffer.GetSize());
        }
    }

    void EventTraceDataAggregator::ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings)
    {
        (void)exportSettings;
        ExportChromeTrace(file, 0, (FrameNumberType)GetFrameCount() - 1);
    }
}
