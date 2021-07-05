/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "SpecializedDependencyScanner.h"
#include <AzCore/std/string/regex.h>

namespace AssetProcessor
{

    /// Scans a given file stream for anything that looks like a path, asset ID, or UUID.
    class LineByLineDependencyScanner : public SpecializedDependencyScanner
    {
    public:
        bool ScanFileForPotentialDependencies(AZ::IO::GenericStream& fileStream, PotentialDependencies& potentialDependencies, int maxScanIteration) override;
        bool DoesScannerMatchFileData(AZ::IO::GenericStream& fileStream) override;
        bool DoesScannerMatchFileExtension(const AZStd::string& fullPath) override;

        AZStd::string GetVersion() const override;
        AZStd::string GetName() const override;

        AZ::Crc32 GetScannerCRC() const override;

        enum class SearchResult
        {
            Completed,
            ScanLimitHit,
        };

    protected:
        SearchResult ScanStringForMissingDependencies(
            const AZStd::string& scanString,
            int maxScanIteration,
            const AZStd::regex& subIdRegex,
            const AZStd::regex& uuidRegex,
            const AZStd::regex& pathRegex,
            PotentialDependencies& potentialDependencies);
    };
}
