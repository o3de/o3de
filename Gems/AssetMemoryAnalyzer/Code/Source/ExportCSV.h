/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>


namespace AssetMemoryAnalyzer
{
    class AssetMemoryAnalyzerSystemComponent;

    // This class provides the service of exporting a capture of asset memory to a JSON file that is viewable in the web viewer.
    class ExportCSV 
    {
    public:
        AZ_TYPE_INFO(AssetMemoryAnalyzer::ExportCSV, "{FEA7D137-EA93-4366-85C2-DCBCE00B3376}");
        AZ_CLASS_ALLOCATOR(ExportCSV, AZ::OSAllocator, 0);

        ExportCSV();
        ~ExportCSV();

        void Init(AssetMemoryAnalyzerSystemComponent* owner);
        void OutputCSV(const char* path);

    private:
        AssetMemoryAnalyzerSystemComponent* m_owner;
    };
}
