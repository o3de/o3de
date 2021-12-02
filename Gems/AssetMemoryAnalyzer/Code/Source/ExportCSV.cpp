/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExportCSV.h"

#include "AssetMemoryAnalyzer.h"
#include "AssetMemoryAnalyzerSystemComponent.h"
#include "FormatUtils.h"

#include <AzCore/IO/FileIO.h>

namespace AssetMemoryAnalyzer
{
    ExportCSV::ExportCSV()
    {
    }

    ExportCSV::~ExportCSV()
    {
    }

    void ExportCSV::Init(AssetMemoryAnalyzerSystemComponent* owner)
    {
        m_owner = owner;
    }

    void ExportCSV::OutputCSV(const char* path)
    {
        using namespace Data;

        AZStd::shared_ptr<FrameAnalysis> analysis = m_owner->GetAnalysis();

        if (!analysis)
        {
            return;
        }

        auto fs = AZ::IO::FileIOBase::GetDirectInstance();
        AZ::IO::HandleType hdl;

        if (!fs->Open(path, AZ::IO::OpenMode::ModeWrite, hdl))
        {
            AZ_Assert(false, "Unable to open file for writing: %s", path);
        }

        const AZStd::string header("Label,Heap Count,Heap kb,VRAM Count,VRAM kb\n");
        fs->Write(hdl, header.c_str(), header.length());
        char lineBuffer[4096];
        const auto& rootAsset = analysis->GetRootAsset();

        size_t length = snprintf(lineBuffer, sizeof(lineBuffer), "<uncategorized>,%d,%0.2f,%d,%0.2f\n",
            rootAsset.m_localSummary[(int)AllocationCategories::HEAP].m_allocationCount,
            rootAsset.m_localSummary[(int)AllocationCategories::HEAP].m_allocatedMemory / 1024.0f,
            rootAsset.m_localSummary[(int)AllocationCategories::VRAM].m_allocationCount,
            rootAsset.m_localSummary[(int)AllocationCategories::VRAM].m_allocatedMemory / 1024.0f
        );
        fs->Write(hdl, lineBuffer, length);

        for (const auto& child : analysis->GetRootAsset().m_childAssets)
        {
            length = snprintf(lineBuffer, sizeof(lineBuffer), "%s,%d,%0.2f,%d,%0.2f\n",
                child.m_id,
                child.m_totalSummary[(int)AllocationCategories::HEAP].m_allocationCount,
                child.m_totalSummary[(int)AllocationCategories::HEAP].m_allocatedMemory / 1024.0f,
                child.m_totalSummary[(int)AllocationCategories::VRAM].m_allocationCount,
                child.m_totalSummary[(int)AllocationCategories::VRAM].m_allocatedMemory / 1024.0f
            );

            fs->Write(hdl, lineBuffer, length);
        }

        fs->Close(hdl);

        AZ_Printf("Debug", "Exported asset allocation list to %s", path);
    }
}
