/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvTexture.h"

#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Name/Name.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QFileInfo>

namespace WhiteBox
{
    namespace
    {
        // The material White Box renders with when neither the face nor the entity names one.
        constexpr const char* BuiltInMaterialPath = "materials/whiteboxdefault.azmaterial";
        // Big textures are shrunk once, so painting the view stays cheap.
        constexpr int MaxTextureSize = 1024;

        UvTextureCache::Result ReadTexture(const AZ::RPI::MaterialAsset& material)
        {
            UvTextureCache::Result result;
            const AZ::RPI::MaterialPropertiesLayout* layout = material.GetMaterialPropertiesLayout();
            if (layout == nullptr)
            {
                return result;
            }
            const AZ::RPI::MaterialPropertyIndex index = layout->FindPropertyIndex(AZ::Name("baseColor.textureMap"));
            const auto& values = material.GetPropertyValues();
            if (!index.IsValid() || index.GetIndex() >= values.size())
            {
                return result;
            }
            const AZ::RPI::MaterialPropertyValue& value = values[index.GetIndex()];
            if (!value.Is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
            {
                return result;
            }
            const AZ::Data::AssetId imageId = value.GetValue<AZ::Data::Asset<AZ::RPI::ImageAsset>>().GetId();
            if (!imageId.IsValid())
            {
                return result;
            }
            // Products are GPU formats; the source file is what Qt can open.
            bool found = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, imageId.m_guid, info, watchFolder);
            if (!found)
            {
                return result;
            }
            const QString path = QString::fromUtf8(watchFolder.c_str()) + QStringLiteral("/") + QString::fromUtf8(info.m_relativePath.c_str());
            QImage image(path);
            if (image.isNull())
            {
                return result;
            }
            if (image.width() > MaxTextureSize || image.height() > MaxTextureSize)
            {
                image = image.scaled(MaxTextureSize, MaxTextureSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            result.m_image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            result.m_name = QFileInfo(path).fileName();
            return result;
        }
    } // namespace

    UvTextureCache::Result UvTextureCache::Find(AZ::Data::AssetId materialId)
    {
        if (!materialId.IsValid())
        {
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                materialId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, BuiltInMaterialPath,
                azrtti_typeid<AZ::RPI::MaterialAsset>(), false);
            if (!materialId.IsValid())
            {
                return {};
            }
        }
        if (const auto done = m_done.find(materialId); done != m_done.end())
        {
            return done->second;
        }
        auto loading = m_loading.find(materialId);
        if (loading == m_loading.end())
        {
            // Holding the handle keeps the load going between calls.
            loading = m_loading
                          .emplace(
                              materialId,
                              AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                                  materialId, AZ::Data::AssetLoadBehavior::PreLoad))
                          .first;
        }
        const AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset = loading->second;
        if (asset.IsReady())
        {
            Result result = ReadTexture(*asset.Get());
            m_loading.erase(loading);
            return m_done.emplace(materialId, AZStd::move(result)).first->second;
        }
        if (asset.IsError() || !asset.GetId().IsValid())
        {
            m_loading.erase(loading);
            return m_done.emplace(materialId, Result{}).first->second;
        }
        Result pending;
        pending.m_pending = true;
        return pending;
    }
} // namespace WhiteBox
