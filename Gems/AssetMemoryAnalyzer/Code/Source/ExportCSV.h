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
