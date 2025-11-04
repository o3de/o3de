/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

class QString;

namespace AssetProcessor
{
    class ScanFolderInfo;

    //! Interface for requesting scanfolder/relative path info for a path
    struct IPathConversion
    {
        AZ_RTTI(IPathConversion, "{8838D113-8BC0-4270-BF4D-4DFEAB628102}");

        virtual ~IPathConversion() = default;

        virtual bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const = 0;

        //! given a full file name (assumed already fed through the normalization funciton), return the first matching scan folder
        virtual const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& fullFileName) const = 0;

        virtual const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 id) const = 0;

        // Get the scan folder ID for the intermediate asset path.
        virtual const AZ::s64 GetIntermediateAssetScanFolderId() const
        {
            return -1;
        }
    };
}
