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

            enum class ExportAction : int
            {
                Nothing = 0,
                UseExisting = 1,
                GenerateNew = 2,
            };

            struct ExportItem
            {
                AZ::Data::AssetId m_assetId;
                AZStd::string m_exportPath;
                ExportAction m_exportAction = ExportAction::GenerateNew;
            };

            using ExportItemsContainer = AZStd::vector<ExportItem>;

            // Generates and opens a dialog for configuring material data export paths and actions
            bool OpenExportDialog(ExportItemsContainer& exportItems);

            // Attemts to construct and save material source data from a product asset
            bool ExportMaterialSourceData(const ExportItem& exportItem);
        } // namespace EditorMaterialComponentExporter
    } // namespace Render
} // namespace AZ
