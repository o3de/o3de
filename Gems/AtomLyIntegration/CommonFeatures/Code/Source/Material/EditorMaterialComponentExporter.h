/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentExporter
        {
            // Attemts to generate a display label for a material slot by parsing its file name
            AZStd::string GetLabelByAssetId(const AZ::Data::AssetId& assetId);

            // Generates a destination file path for exporting material source data
            AZStd::string GetExportPathByAssetId(const AZ::Data::AssetId& assetId);

            struct ExportItem
            {
                bool m_enabled = true;
                bool m_exists = false;
                bool m_overwrite = false;
                AZ::Data::AssetId m_assetId;
                AZStd::string m_exportPath;
            };

            using ExportItemsContainer = AZStd::vector<ExportItem>;

            // Generates and opens a dialog for configuring material data export paths and actions
            bool OpenExportDialog(ExportItemsContainer& exportItems);

            // Attemts to construct and save material source data from a product asset
            bool ExportMaterialSourceData(const ExportItem& exportItem);
        } // namespace EditorMaterialComponentExporter
    } // namespace Render
} // namespace AZ
