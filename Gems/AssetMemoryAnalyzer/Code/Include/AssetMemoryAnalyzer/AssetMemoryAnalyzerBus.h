/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
