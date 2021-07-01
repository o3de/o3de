/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/Benchmark/BenchmarkSettingsAsset.h>

namespace AzFramework
{
    void BenchmarkSettingsAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BenchmarkSettingsAsset, AZ::Data::AssetData>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("PrimaryAssetByteSize", &BenchmarkSettingsAsset::m_primaryAssetByteSize)
                ->Field("DependentAssetByteSize", &BenchmarkSettingsAsset::m_dependentAssetByteSize)
                ->Field("DependencyDepth", &BenchmarkSettingsAsset::m_dependencyDepth)
                ->Field("NumAssetsPerDependency", &BenchmarkSettingsAsset::m_numAssetsPerDependency)
                ->Field("AssetStorageType", &BenchmarkSettingsAsset::m_assetStorageType)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<BenchmarkSettingsAsset>(
                    "Benchmark Settings Asset", "Settings file for generating assets for benchmark purposes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_primaryAssetByteSize, "Asset Buffer Size", "Size of the test buffer in the primary asset in bytes")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_dependentAssetByteSize, "Dependent Asset Buffer Size", "Size of the test buffer in each dependent asset in bytes")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_dependencyDepth, "Dependency Depth", "Depth of the asset dependency tree")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_numAssetsPerDependency, "Assets Per Dependency", "Number of assets to generate for each dependency in the tree")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BenchmarkSettingsAsset::m_assetStorageType, "Asset Storage", "Serializaton format to use for each asset (binary, text)")
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_BINARY, "Binary")
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_XML, "XML")
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_JSON, "JSON")
                    ;
            }
        }
    }
} // namespace AzFramework
