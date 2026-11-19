/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvTexture.h"

#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
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

        // StandardPBR and friends carry one map; StandardMultilayerPBR one per layer, layers 2 and 3 behind a switch.
        struct Candidate
        {
            const char* m_property;
            const char* m_enable;
            const char* m_layer;
        };
        constexpr Candidate Candidates[] = {
            { "baseColor.textureMap", nullptr, "Base Colour" },
            { "layer1.baseColor.textureMap", nullptr, "Layer 1" },
            { "layer2.baseColor.textureMap", "blend.enableLayer2", "Layer 2" },
            { "layer3.baseColor.textureMap", "blend.enableLayer3", "Layer 3" },
        };

        // The material's value for a property, or null when the material type has no such property.
        const AZ::RPI::MaterialPropertyValue* FindValue(const AZ::RPI::MaterialAsset& material, const char* name)
        {
            const AZ::RPI::MaterialPropertiesLayout* layout = material.GetMaterialPropertiesLayout();
            if (layout == nullptr)
            {
                return nullptr;
            }
            const AZ::RPI::MaterialPropertyIndex index = layout->FindPropertyIndex(AZ::Name(name));
            const auto& values = material.GetPropertyValues();
            return index.IsValid() && index.GetIndex() < values.size() ? &values[index.GetIndex()] : nullptr;
        }

        // A Material component override of an image, which it stores as a bare asset id (or, before saving, an asset).
        bool OverrideImage(const AZStd::any& value, AZ::Data::AssetId& image)
        {
            if (value.is<AZ::Data::AssetId>())
            {
                image = AZStd::any_cast<AZ::Data::AssetId>(value);
                return true;
            }
            if (value.is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
            {
                image = AZStd::any_cast<AZ::Data::Asset<AZ::RPI::ImageAsset>>(value).GetId();
                return true;
            }
            if (value.is<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>())
            {
                image = AZStd::any_cast<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(value).GetId();
                return true;
            }
            if (value.is<AZ::Data::Asset<AZ::Data::AssetData>>())
            {
                image = AZStd::any_cast<AZ::Data::Asset<AZ::Data::AssetData>>(value).GetId();
                return true;
            }
            return false;
        }
    } // namespace

    const UvTextureCache::CachedImage& UvTextureCache::Image(const AZ::Data::AssetId& imageId)
    {
        CachedImage& cached = m_images[imageId];
        if (cached.m_path.isEmpty())
        {
            // Products are GPU formats; the source file is what Qt can open.
            bool found = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, imageId.m_guid, info, watchFolder);
            if (!found)
            {
                return cached;
            }
            cached.m_path = QString::fromUtf8(watchFolder.c_str()) + QStringLiteral("/") + QString::fromUtf8(info.m_relativePath.c_str());
        }
        // Re-read when the file changes on disk, so an edited texture shows without reopening the pane.
        const QDateTime modified = QFileInfo(cached.m_path).lastModified();
        if (modified == cached.m_modified)
        {
            return cached;
        }
        cached.m_modified = modified;
        QImage image(cached.m_path);
        if (!image.isNull() && (image.width() > MaxTextureSize || image.height() > MaxTextureSize))
        {
            image = image.scaled(MaxTextureSize, MaxTextureSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        cached.m_image = image.isNull() ? QImage() : image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        cached.m_name = QFileInfo(cached.m_path).fileName();
        return cached;
    }

    UvTextureCache::Result UvTextureCache::Find(AZ::Data::AssetId materialId, const PropertyOverrides* overrides)
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
        auto held = m_materials.find(materialId);
        if (held == m_materials.end())
        {
            // Holding the handle keeps the load going between calls.
            held = m_materials
                       .emplace(
                           materialId,
                           AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                               materialId, AZ::Data::AssetLoadBehavior::PreLoad))
                       .first;
        }
        AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset = held->second;
        if (asset.IsReady())
        {
            // A material saved in the Material Editor reloads into new data; follow it.
            const AZ::Data::Asset<AZ::RPI::MaterialAsset> current =
                AZ::Data::AssetManager::Instance().FindAsset<AZ::RPI::MaterialAsset>(materialId, AZ::Data::AssetLoadBehavior::Default);
            if (current.IsReady() && current.Get() != asset.Get())
            {
                asset = current;
            }
        }
        else if (asset.IsError() || !asset.GetId().IsValid())
        {
            return {};
        }
        else
        {
            Result pending;
            pending.m_pending = true;
            return pending;
        }

        const auto overridden = [overrides](const char* name) -> const AZStd::any*
        {
            if (overrides == nullptr)
            {
                return nullptr;
            }
            const auto it = overrides->find(AZ::Name(name));
            return it != overrides->end() && !it->second.empty() ? &it->second : nullptr;
        };

        Result result;
        for (const Candidate& candidate : Candidates)
        {
            if (FindValue(*asset.Get(), candidate.m_property) == nullptr)
            {
                continue; // not this material type's property
            }
            if (candidate.m_enable != nullptr)
            {
                // The layer switch, from the entity's override if it has one; a switched-off layer never shows on the mesh.
                bool enabled = true;
                if (const AZStd::any* value = overridden(candidate.m_enable); value != nullptr && value->is<bool>())
                {
                    enabled = AZStd::any_cast<bool>(*value);
                }
                else if (const AZ::RPI::MaterialPropertyValue* stored = FindValue(*asset.Get(), candidate.m_enable);
                         stored != nullptr && stored->Is<bool>())
                {
                    enabled = stored->GetValue<bool>();
                }
                if (!enabled)
                {
                    continue;
                }
            }
            AZ::Data::AssetId imageId;
            const AZStd::any* value = overridden(candidate.m_property);
            if (value == nullptr || !OverrideImage(*value, imageId))
            {
                const AZ::RPI::MaterialPropertyValue* stored = FindValue(*asset.Get(), candidate.m_property);
                if (stored->Is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
                {
                    imageId = stored->GetValue<AZ::Data::Asset<AZ::RPI::ImageAsset>>().GetId();
                }
            }
            if (!imageId.IsValid())
            {
                continue;
            }
            const CachedImage& image = Image(imageId);
            if (!image.m_image.isNull())
            {
                result.m_textures.push_back({ image.m_image, image.m_name, QString::fromUtf8(candidate.m_layer) });
            }
        }
        return result;
    }
} // namespace WhiteBox
