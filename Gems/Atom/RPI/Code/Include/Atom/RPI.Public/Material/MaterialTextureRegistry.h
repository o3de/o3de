/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>

namespace AZ::RPI
{
    // Manages an array of texture-assets, which are stored in "Texture2D m_textures[];",
    // only used if both AZ_TRAIT_SINGLE_MATERIAL_USE_TEXTURE_ARRAY AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL are set
    class MaterialTextureRegistry
    {
    public:
        // The first sampler will be the default sampler state, and will not be released
        void Init(const uint32_t maxTextures);
        int32_t RegisterMaterialTexture(const Data::Instance<Image>& image);
        void ReleaseMaterialTexture(int32_t textureIndex);
        AZStd::vector<const RHI::ImageView*> CollectTextureViews();

    private:
        uint32_t m_maxTextures{ 0 };
        AZStd::vector<Data::Instance<Image>> m_materialTextures;
        AZStd::vector<int32_t> m_materialTexturesReferenceCount;
        AZStd::unordered_map<Data::AssetId, int32_t> m_materialTexturesMap;
        PersistentIndexAllocator<int32_t> m_textureIndices;
    };

} // namespace AZ::RPI
