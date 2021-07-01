/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetMemoryAnalyzer_precompiled.h"

#include "ExportJSON.h"

#include "AssetMemoryAnalyzer.h"
#include "AssetMemoryAnalyzerSystemComponent.h"
#include "FormatUtils.h"

#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/filewritestream.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/std/sort.h>
#include <imgui/imgui.h>
#include <ImGuiBus.h>

namespace AssetMemoryAnalyzer
{
    namespace
    {
        template<class WriterT>
        static void OutputAllocationInfo(WriterT& writer, size_t count, size_t bytes)
        {
            writer.StartObject();
            writer.Key("count");
            writer.Int((int)count);
            writer.Key("kb");
            writer.String(FormatUtils::FormatKB(bytes));
            writer.EndObject();
        }

        template<class WriterT>
        static void OutputAllocationInfo(WriterT& writer, const Data::Summary& summary)
        {
            OutputAllocationInfo(writer, summary.m_allocationCount, summary.m_allocatedMemory);
        }
    }

    ExportJSON::ExportJSON()
    {
    }

    ExportJSON::~ExportJSON()
    {
    }

    void ExportJSON::Init(AssetMemoryAnalyzerSystemComponent* owner)
    {
        m_owner = owner;
    }

    void ExportJSON::OutputJSON(const char* path)
    {
        using namespace Data;
        using namespace rapidjson;

        AZStd::shared_ptr<FrameAnalysis> analysis = m_owner->GetAnalysis();

        if (!analysis)
        {
            return;
        }

        StringBuffer buff;
        PrettyWriter<StringBuffer> writer(buff);
        size_t idCounter = 0;

        AZStd::function<void(const AssetInfo&, int)> recurse;
        recurse = [&recurse, &writer, &idCounter](const AssetInfo& asset, int depth)
        {
            writer.StartObject();

            writer.Key("id");
            writer.Int(idCounter++);

            writer.Key("label");
            writer.String(asset.m_id ? asset.m_id : "Root");

            writer.Key("heap");
            OutputAllocationInfo(writer, asset.m_totalSummary[(int)AllocationCategories::HEAP]);

            writer.Key("vram");
            OutputAllocationInfo(writer, asset.m_totalSummary[(int)AllocationCategories::VRAM]);

            if (!asset.m_allocationPoints.empty() || !asset.m_childAssets.empty())
            {
                writer.Key("_children");
                writer.StartArray();

                if (!asset.m_allocationPoints.empty())
                {
                    writer.StartObject();
                    writer.Key("id");
                    writer.Int(idCounter++);

                    writer.Key("label");
                    writer.String("<local allocations>");

                    writer.Key("heap");
                    OutputAllocationInfo(writer, asset.m_localSummary[(int)AllocationCategories::HEAP]);

                    writer.Key("vram");
                    OutputAllocationInfo(writer, asset.m_localSummary[(int)AllocationCategories::VRAM]);

                    writer.Key("_children");
                    writer.StartArray();

                    for (const auto& ap : asset.m_allocationPoints)
                    {
                        Summary heapSummary;
                        Summary vramSummary;

                        writer.StartObject();
                        writer.Key("id");
                        writer.Int(idCounter++);

                        writer.Key("label");

                        switch (ap.m_codePoint->m_category)
                        {
                        case AllocationCategories::HEAP:
                            writer.String(FormatUtils::FormatCodePoint(*ap.m_codePoint));
                            heapSummary.m_allocationCount = ap.m_allocations.size();
                            heapSummary.m_allocatedMemory = ap.m_totalAllocatedMemory;
                            break;

                        case AllocationCategories::VRAM:
                            writer.String(ap.m_codePoint->m_file);
                            vramSummary.m_allocationCount = ap.m_allocations.size();
                            vramSummary.m_allocatedMemory = ap.m_totalAllocatedMemory;
                            break;
                        }

                        writer.Key("heap");
                        OutputAllocationInfo(writer, heapSummary);

                        writer.Key("vram");
                        OutputAllocationInfo(writer, vramSummary);

                        writer.EndObject();
                    }

                    writer.EndArray();
                    writer.EndObject();
                }

                for (const auto& childInfo : asset.m_childAssets)
                {
                    recurse(childInfo, depth + 1);
                }

                writer.EndArray();
            }

            writer.EndObject();
        };

        writer.StartArray();
        recurse(analysis->GetRootAsset(), 0);
        writer.EndArray();

        auto fs = AZ::IO::FileIOBase::GetDirectInstance();
        AZ::IO::HandleType hdl;

        if (!fs->Open(path, AZ::IO::OpenMode::ModeWrite, hdl))
        {
            AZ_Assert(false, "Unable to open file for writing: %s", path);
        }

        fs->Write(hdl, buff.GetString(), buff.GetSize());
        fs->Close(hdl);

        AZ_Printf("Debug", "Exported asset allocation map to %s", path);
    }
}
