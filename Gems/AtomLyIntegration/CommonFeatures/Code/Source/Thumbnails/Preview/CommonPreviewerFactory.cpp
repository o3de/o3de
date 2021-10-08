/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Source/Thumbnails/Preview/CommonPreviewer.h>
#include <Source/Thumbnails/Preview/CommonPreviewerFactory.h>
#include <Source/Thumbnails/ThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        AzToolsFramework::AssetBrowser::Previewer* CommonPreviewerFactory::CreatePreviewer(QWidget* parent) const
        {
            return new CommonPreviewer(parent);
        }

        bool CommonPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            AZ::Data::AssetId assetId = Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::ModelAsset::RTTI_Type());
            if (assetId.IsValid())
            {
                return true;
            }

            assetId = Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::MaterialAsset::RTTI_Type());
            if (assetId.IsValid())
            {
                return true;
            }

            assetId = Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::AnyAsset::RTTI_Type());
            if (assetId.IsValid())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                return AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), "lightingpreset.azasset");
            }

            return false;
        }

        const QString& CommonPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
