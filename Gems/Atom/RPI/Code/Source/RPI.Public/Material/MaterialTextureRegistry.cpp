/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/MaterialTextureRegistry.h>

namespace AZ::RPI
{
    void MaterialTextureRegistry::Init(const uint32_t maxTextures)
    {
        // we might be re-using this registry, so reset everything
        *this = {};
        m_maxTextures = maxTextures;
        m_materialTextures.resize(maxTextures, nullptr);
        m_materialTexturesReferenceCount.resize(maxTextures, 0);
    }

    AZStd::vector<const RHI::ImageView*> MaterialTextureRegistry::CollectTextureViews()
    {
        AZStd::vector<const RHI::ImageView*> result;
        result.resize(m_materialTextures.size(), nullptr);
        for (int index = 0; index < m_materialTextures.size(); ++index)
        {
            if (m_materialTextures[index])
            {
                result[index] = m_materialTextures[index]->GetImageView();
            }
        }
        return result;
    }

    int32_t MaterialTextureRegistry::RegisterMaterialTexture(const Data::Instance<Image>& image)
    {
        int32_t textureIndex = -1;
        auto it = m_materialTexturesMap.find(image->GetAssetId());
        if (it == m_materialTexturesMap.end())
        {
            if (m_textureIndices.Count() < m_maxTextures)
            {
                textureIndex = m_textureIndices.Aquire();
                m_materialTextures[textureIndex] = image;
                m_materialTexturesMap[image->GetAssetId()] = textureIndex;
                m_materialTexturesReferenceCount[textureIndex] = 1;
            }
            else
            {
                AZ_Assert(
                    false,
                    "Rejecting texture %s, a Material can't reference more than %d textures.",
                    image->GetAssetId().ToFixedString().c_str(),
                    m_maxTextures);
            }
        }
        else
        {
            textureIndex = it->second;
            m_materialTexturesReferenceCount[textureIndex]++;
        }
        return textureIndex;
    }

    void MaterialTextureRegistry::ReleaseMaterialTexture(int32_t textureIndex)
    {
        if (textureIndex < 0 || textureIndex >= static_cast<int32_t>(m_maxTextures))
        {
            return;
        }
        if (m_materialTexturesReferenceCount[textureIndex] == 1)
        {
            m_materialTexturesMap.erase(m_materialTextures[textureIndex]->GetAssetId());
            m_materialTextures[textureIndex] = nullptr;
            m_materialTexturesReferenceCount[textureIndex] = 0;
            m_textureIndices.Release(textureIndex);
        }
        else if (m_materialTexturesReferenceCount[textureIndex] > 1)
        {
            m_materialTexturesReferenceCount[textureIndex]--;
        }
    }

} // namespace AZ::RPI
