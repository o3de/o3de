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
#include <Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            Data::AssetId GetAssetId(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, const Data::AssetType& assetType)
            {
                static const Data::AssetId invalidAssetId;

                // if it's a source thumbnail key, find first product with a matching asset type
                auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
                if (sourceKey)
                {
                    bool foundIt = false;
                    AZStd::vector<Data::AssetInfo> productsAssetInfo;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, sourceKey->GetSourceUuid(), productsAssetInfo);
                    if (!foundIt)
                    {
                        return invalidAssetId;
                    }
                    auto assetInfoIt = AZStd::find_if(productsAssetInfo.begin(), productsAssetInfo.end(),
                        [&assetType](const Data::AssetInfo& assetInfo)
                        {
                            return assetInfo.m_assetType == assetType;
                        });
                    if (assetInfoIt == productsAssetInfo.end())
                    {
                        return invalidAssetId;
                    }

                    return assetInfoIt->m_assetId;
                }

                // if it's a product thumbnail key just return its assetId
                auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
                if (productKey && productKey->GetAssetType() == assetType)
                {
                    return productKey->GetAssetId();
                }
                return invalidAssetId;
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
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
