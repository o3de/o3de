/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/Benchmark/BenchmarkSettingsAsset.h>
#include <AzFramework/Translation/TranslationDef.h>

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
                    QT_TRANSLATE_NOOP("AzFramework", "Benchmark Settings Asset"),
                    QT_TRANSLATE_NOOP("AzFramework", "Settings file for generating assets for benchmark purposes"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_primaryAssetByteSize,
                        QT_TRANSLATE_NOOP("AzFramework", "Asset Buffer Size"),
                        QT_TRANSLATE_NOOP("AzFramework", "Size of the test buffer in the primary asset in bytes"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_dependentAssetByteSize,
                        QT_TRANSLATE_NOOP("AzFramework", "Dependent Asset Buffer Size"),
                        QT_TRANSLATE_NOOP("AzFramework", "Size of the test buffer in each dependent asset in bytes"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_dependencyDepth,
                        QT_TRANSLATE_NOOP("AzFramework", "Dependency Depth"),
                        QT_TRANSLATE_NOOP("AzFramework", "Depth of the asset dependency tree"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkSettingsAsset::m_numAssetsPerDependency,
                        QT_TRANSLATE_NOOP("AzFramework", "Assets Per Dependency"),
                        QT_TRANSLATE_NOOP("AzFramework", "Number of assets to generate for each dependency in the tree"))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BenchmarkSettingsAsset::m_assetStorageType,
                        QT_TRANSLATE_NOOP("AzFramework", "Asset Storage"),
                        QT_TRANSLATE_NOOP("AzFramework", "Serializaton format to use for each asset (binary, text)"))
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_BINARY, "Binary")
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_XML, "XML")
                        ->EnumAttribute(AZ::DataStream::StreamType::ST_JSON, "JSON")
                    ;
            }
        }
    }
} // namespace AzFramework
