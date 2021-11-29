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
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
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
                return { RPI::ModelAsset::RTTI_Type(), RPI::MaterialAsset::RTTI_Type(), RPI::AnyAsset::RTTI_Type() };
            }

            bool IsSupportedAssetType(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            {
                return GetSupportedAssetInfo(key).m_assetId.IsValid();
            }

            AZ::Data::AssetInfo GetSupportedAssetInfo(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            {
                const auto& supportedTypeIds = GetSupportedAssetTypes();

                // if it's a source thumbnail key, find first product with a matching asset type
                auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
                if (sourceKey)
                {
                    bool foundIt = false;
                    AZStd::vector<AZ::Data::AssetInfo> productsAssetInfo;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID,
                        sourceKey->GetSourceUuid(), productsAssetInfo);

                    // Search the product assets for a matching asset type ID in the order of the supported type IDs, which are organized by priority
                    for (const auto& typeId : supportedTypeIds)
                    {
                        for (const auto& assetInfo : productsAssetInfo)
                        {
                            if (assetInfo.m_assetType == typeId)
                            {
                                return assetInfo;
                            }
                        }
                    }
                    return AZ::Data::AssetInfo();
                }

                // if it's a product thumbnail key just return its assetId
                AZ::Data::AssetInfo assetInfo;
                auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
                if (productKey &&
                    AZStd::find(supportedTypeIds.begin(), supportedTypeIds.end(), productKey->GetAssetType()) != supportedTypeIds.end())
                {
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, productKey->GetAssetId());
                }
                return assetInfo;
            }

            AZ::Data::AssetId GetSupportedAssetId(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, const AZ::Data::AssetId& defaultAssetId)
            {
                const AZ::Data::AssetInfo assetInfo = GetSupportedAssetInfo(key);
                return assetInfo.m_assetId.IsValid() ? assetInfo.m_assetId : defaultAssetId;
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
