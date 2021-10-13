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
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <SharedPreview/SharedPreviewUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace SharedPreviewUtils
        {
            Data::AssetId GetAssetId(
                AzToolsFramework::Thumbnailer::SharedThumbnailKey key,
                const Data::AssetType& assetType,
                const Data::AssetId& defaultAssetId)
            {
                // if it's a source thumbnail key, find first product with a matching asset type
                auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
                if (sourceKey)
                {
                    bool foundIt = false;
                    AZStd::vector<Data::AssetInfo> productsAssetInfo;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID,
                        sourceKey->GetSourceUuid(), productsAssetInfo);
                    if (!foundIt)
                    {
                        return defaultAssetId;
                    }
                    auto assetInfoIt = AZStd::find_if(
                        productsAssetInfo.begin(), productsAssetInfo.end(),
                        [&assetType](const Data::AssetInfo& assetInfo)
                        {
                            return assetInfo.m_assetType == assetType;
                        });
                    if (assetInfoIt == productsAssetInfo.end())
                    {
                        return defaultAssetId;
                    }

                    return assetInfoIt->m_assetId;
                }

                // if it's a product thumbnail key just return its assetId
                auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
                if (productKey && productKey->GetAssetType() == assetType)
                {
                    return productKey->GetAssetId();
                }
                return defaultAssetId;
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

            AZStd::unordered_set<AZ::Uuid> GetSupportedAssetTypes()
            {
                return { RPI::AnyAsset::RTTI_Type(), RPI::MaterialAsset::RTTI_Type(), RPI::ModelAsset::RTTI_Type() };
            }

            bool IsSupportedAssetType(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            {
                for (const AZ::Uuid& typeId : SharedPreviewUtils::GetSupportedAssetTypes())
                {
                    const AZ::Data::AssetId& assetId = SharedPreviewUtils::GetAssetId(key, typeId);
                    if (assetId.IsValid())
                    {
                        if (typeId == RPI::AnyAsset::RTTI_Type())
                        {
                            AZ::Data::AssetInfo assetInfo;
                            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                                assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                            return AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), "lightingpreset.azasset");
                        }
                        return true;
                    }
                }

                return false;
            }
        } // namespace SharedPreviewUtils
    } // namespace LyIntegration
} // namespace AZ
