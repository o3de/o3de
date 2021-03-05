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

#include <AzCore/EBus/EBus.h>

namespace AssetMemoryAnalyzer
{
    class FrameAnalysis;

    class AssetMemoryAnalyzerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Enables or disables the AssetMemoryAnalyzer.
        virtual void SetEnabled(bool enabled = true) = 0;

        // Exports a CSV file that may be imported into a spreadsheet. Top-level assets only, due to the limitations of CSV. Path is optional, defaults to @log@/assetmem-<TIMESTAMP>.csv
        virtual void ExportCSVFile(const char* path = nullptr) = 0;

        // Exports a JSON file that may be viewed by the web viewer. Path is optional, defaults to @log@/assetmem-<TIMESTAMP>.json
        virtual void ExportJSONFile(const char* path = nullptr) = 0;

        // Retrieves a frame analysis. (Generally used for testing purposes; use of the gem's private headers are required to inspect this.)
        virtual AZStd::shared_ptr<FrameAnalysis> GetAnalysis() = 0;
    };
    using AssetMemoryAnalyzerRequestBus = AZ::EBus<AssetMemoryAnalyzerRequests>;
} // namespace AssetMemoryAnalyzer
