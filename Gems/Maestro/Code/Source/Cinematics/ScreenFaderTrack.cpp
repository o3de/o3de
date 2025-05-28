/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

#include "ScreenFaderTrack.h"


namespace Maestro
{

    CScreenFaderTrack::CScreenFaderTrack()
    {
        m_activeTextureNumber = -1;
        SetScreenFaderTrackDefaults();
    }

    CScreenFaderTrack::~CScreenFaderTrack()
    {
        m_preloadedTextures.clear();
    }

    void CScreenFaderTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        static char desc[32];
        duration = m_keys[keyIndex].m_fadeTime;
        azstrcpy(desc, AZ_ARRAY_SIZE(desc), m_keys[keyIndex].m_fadeType == IScreenFaderKey::eFT_FadeIn ? "In" : "Out");
        description = desc;
    }

    void CScreenFaderTrack::SerializeKey(IScreenFaderKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            keyNode->getAttr("fadeTime", key.m_fadeTime);
            Vec3 color(0, 0, 0);
            keyNode->getAttr("fadeColor", color);
            key.m_fadeColor = AZ::Color(color.x, color.y, color.z, 1.f);
            int fadeType = 0;
            keyNode->getAttr("fadeType", fadeType);
            key.m_fadeType = IScreenFaderKey::EFadeType(fadeType);
            int fadeChangeType(0);
            if (keyNode->getAttr("fadeChangeType", fadeChangeType))
            {
                key.m_fadeChangeType = IScreenFaderKey::EFadeChangeType(fadeChangeType);
            }
            else
            {
                key.m_fadeChangeType = IScreenFaderKey::eFCT_Linear;
            }
            const char* str = keyNode->getAttr("texture");
            key.m_strTexture = str;
            keyNode->getAttr("useCurColor", key.m_bUseCurColor);
        }
        else
        {
            keyNode->setAttr("fadeTime", key.m_fadeTime);
            Vec3 color(key.m_fadeColor.GetR(), key.m_fadeColor.GetG(), key.m_fadeColor.GetB());
            keyNode->setAttr("fadeColor", color);
            keyNode->setAttr("fadeType", (int)key.m_fadeType);
            keyNode->setAttr("fadeChangeType", (int)key.m_fadeChangeType);
            keyNode->setAttr("texture", key.m_strTexture.c_str());
            keyNode->setAttr("useCurColor", key.m_bUseCurColor);
        }
    }

    void CScreenFaderTrack::SetFlags(int flags)
    {
        // call base class implementation. I'm avoiding the use of the Microsoft specific __super::SetFlags(flags) because it is not
        // platform agnostic
        TAnimTrack<IScreenFaderKey>::SetFlags(flags);

        if (flags & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            // when we disable, 'clear' the screen fader effect to avoid the possibility of leaving the Editor in a faded state
            m_bTextureVisible = false;
            m_drawColor = AZ::Vector4(.0f, .0f, .0f, .0f);
        }
    }

    void CScreenFaderTrack::SetKey(int keyIndex, IKey* key)
    {
        if (!key || keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(key, "Invalid key pointer.");
            AZ_Assert(keyIndex >= 0 && keyIndex < GetNumKeys(), "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        IScreenFaderKey& screenFaderKey = *(static_cast<IScreenFaderKey*>(key));
        screenFaderKey.m_fadeTime = AZStd::clamp(screenFaderKey.m_fadeTime, GetMinKeyTimeDelta(), m_timeRange.end - screenFaderKey.time);

        m_keys[keyIndex] = screenFaderKey;

        SortKeys();
    }

    void CScreenFaderTrack::PreloadTextures()
    {
        if (!m_preloadedTextures.empty())
        {
            m_preloadedTextures.clear();
        }

        const int nKeysCount = GetNumKeys();
        if (nKeysCount > 0)
        {
            m_preloadedTextures.reserve(nKeysCount);
            for (int nKeyIndex = 0; nKeyIndex < nKeysCount; ++nKeyIndex)
            {
                IScreenFaderKey key;
                GetKey(nKeyIndex, &key);
                const auto& texturePath = key.m_strTexture;
                if (texturePath.empty())
                {
                    m_preloadedTextures.push_back(nullptr);
                    continue;
                }

                // The file may not be in the AssetCatalog at this point if it is still processing or doesn't exist on disk.
                // Use GenerateAssetIdTEMP instead of GetAssetIdByPath so that it will return a valid AssetId anyways
                AZ::Data::AssetId streamingImageAssetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    streamingImageAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP, texturePath.c_str());
                streamingImageAssetId.m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();

                auto streamingImageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::RPI::StreamingImageAsset>(
                    streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

                AZ::Data::Instance<AZ::RPI::Image> image = AZ::RPI::StreamingImage::FindOrCreate(streamingImageAsset);

                AZ_Error("ScreenFaderTrack", image, "PreloadTextures(): Failed to find or create an image instance from image asset '%s'", texturePath.c_str());

                m_preloadedTextures.push_back(image);
            }
        }
    }

    AZ::Data::Instance<AZ::RPI::Image> CScreenFaderTrack::GetActiveTexture() const
    {
        return (m_activeTextureNumber >= 0 && m_activeTextureNumber < static_cast<int>(m_preloadedTextures.size()))
            ? m_preloadedTextures[m_activeTextureNumber]
            : nullptr;
    }

    void CScreenFaderTrack::SetScreenFaderTrackDefaults()
    {
        m_bTextureVisible = false;
        m_drawColor = AZ::Vector4(1, 1, 1, 1);
    }

    bool CScreenFaderTrack::SetActiveTexture(int keyIndex)
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d)", keyIndex, GetNumKeys());
            return false;
        }

        auto pTexture = GetActiveTexture();
        m_activeTextureNumber = keyIndex;

        // Check if textures should be reloaded.
        bool bNeedTexReload = pTexture == nullptr; // Not yet loaded
        if (pTexture)
        {
            IScreenFaderKey key;
            GetKey(keyIndex, &key);
            const auto& texturePath = key.m_strTexture;
            if (texturePath.empty())
            {
                return true; // nothing to do
            }

            AZStd::string activeTexturePath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                activeTexturePath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, pTexture->GetAssetId());

            if (texturePath != activeTexturePath)
            {
                bNeedTexReload = true; // Loaded, but a different texture
            }
        }
        if (bNeedTexReload)
        {
            // OK, try to reload.
            PreloadTextures();
            pTexture = GetActiveTexture();
        }
        return pTexture != 0;
    }

    static bool ScreenFaderTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<IScreenFaderKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<IScreenFaderKey>, IAnimTrack>()
                ->Version(3, &ScreenFaderTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<IScreenFaderKey>::m_flags)
                ->Field("Range", &TAnimTrack<IScreenFaderKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<IScreenFaderKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<IScreenFaderKey>::m_keys)
                ->Field("Id", &TAnimTrack<IScreenFaderKey>::m_id);
        }
    }

    void CScreenFaderTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<IScreenFaderKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CScreenFaderTrack, TAnimTrack<IScreenFaderKey>>()->Version(1);
        }
    }

} // namespace Maestro
