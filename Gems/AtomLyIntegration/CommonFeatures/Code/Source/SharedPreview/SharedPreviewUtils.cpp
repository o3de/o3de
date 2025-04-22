/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/EditorAssetSystemAPI.h>
#include <AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/Util/Util.h>
#include <SharedPreview/SharedPreviewUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace SharedPreviewUtils
        {
            AZStd::vector<AZ::Uuid> GetSupportedAssetTypes()
            {
                return { RPI::ModelAsset::RTTI_Type(), RPI::MaterialAsset::RTTI_Type(), RPI::MaterialTypeAsset::RTTI_Type(),
                         RPI::AnyAsset::RTTI_Type() };
            }

            bool IsSupportedAssetType(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            {
                return GetSupportedAssetInfo(key).m_assetId.IsValid();
            }

            AZ::Data::AssetInfo GetSupportedAssetInfo(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            {
                AZStd::vector<AZ::Data::AssetInfo> productsAssetInfo;

                // if it's a source thumbnail key, find first product with a matching asset type
                auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
                if (sourceKey)
                {
                    bool foundIt = false;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID,
                        sourceKey->GetSourceUuid(), productsAssetInfo);
                }

                // if it's a product thumbnail key just return its assetInfo
                auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
                if (productKey)
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, productKey->GetAssetId());
                    productsAssetInfo.push_back(assetInfo);
                }

                // Search the product assets for a matching asset type ID in priority order of the supported type IDs
                for (const auto& typeId : GetSupportedAssetTypes())
                {
                    for (const auto& assetInfo : productsAssetInfo)
                    {
                        if (assetInfo.m_assetType == typeId)
                        {
                            const auto& path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                            if (!AtomToolsFramework::IsDocumentPathPreviewable(path))
                            {
                                continue;
                            }

                            // Reject any assets that don't match supported source file extensions
                            if (assetInfo.m_assetType == RPI::AnyAsset::RTTI_Type())
                            {
                                if (!path.ends_with(AZ::Render::LightingPreset::Extension) &&
                                    !path.ends_with(AZ::Render::ModelPreset::Extension))
                                {
                                    continue;
                                }
                            }

                            return assetInfo;
                        }
                    }
                }

                return AZ::Data::AssetInfo();
            }

            AZ::Data::AssetId GetAssetIdForProductPath(const AZStd::string_view productPath)
            {
                if (!productPath.empty())
                {
                    return AZ::RPI::AssetUtils::GetAssetIdForProductPath(productPath.data());
                }
                return AZ::Data::AssetId();
            }

            QString WordWrap(const QString& string, int maxLength)
            {
                QString result;
                int length = 0;

                for (const QChar& c : string)
                {
                    if (c == '\n')
                    {
                        length = 0;
                    }
                    else if (length > maxLength)
                    {
                        result.append('\n');
                        length = 0;
                    }
                    else
                    {
                        length++;
                    }
                    result.append(c);
                }
                return result;
            }
        } // namespace SharedPreviewUtils
    } // namespace LyIntegration
} // namespace AZ
