/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <QDateTime>
#include <QImage>
#include <QString>

namespace AZ::RPI
{
    class MaterialAsset;
}

namespace WhiteBox
{
    //! The base colour textures of a material, read from their source images so the UV editor can draw them.
    class UvTextureCache
    {
    public:
        //! One base colour map: the material's own, or one layer's of a multilayer material.
        struct Texture
        {
            QImage m_image;
            QString m_name;  //!< The texture's file name, for display.
            QString m_layer; //!< "Base Colour", or "Layer 1" to "Layer 3" for a multilayer material.
        };

        //! What Find learned about a material.
        struct Result
        {
            AZStd::vector<Texture> m_textures; //!< Empty when the material has no base colour texture Qt can read.
            bool m_pending = false;            //!< The material is still loading; ask again later.
        };

        //! A Material component's property overrides for one slot (same shape as AZ::Render::MaterialPropertyOverrideMap).
        using PropertyOverrides = AZStd::unordered_map<AZ::Name, AZStd::any>;

        //! @param materialId invalid means the built-in White Box material.
        //! @param overrides the entity's instance overrides for the slot, whose textures and layer switches win over the asset's.
        Result Find(AZ::Data::AssetId materialId, const PropertyOverrides* overrides = nullptr);

    private:
        struct CachedImage
        {
            QImage m_image;
            QString m_name;
            QString m_path;       //!< Source file; empty until the asset system has resolved it.
            QDateTime m_modified; //!< The file's time when m_image was read, to notice edits.
        };
        const CachedImage& Image(const AZ::Data::AssetId& imageId);

        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::MaterialAsset>> m_materials;
        AZStd::unordered_map<AZ::Data::AssetId, CachedImage> m_images;
    };
} // namespace WhiteBox
