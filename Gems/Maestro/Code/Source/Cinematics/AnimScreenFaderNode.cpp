/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include "AnimScreenFaderNode.h"
#include "ScreenFaderTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    namespace AnimScreenFaderNodeHelper
    {
        static bool s_screenFaderNodeParamsInitialized = false;
        static AZStd::vector<CAnimNode::SParamInfo> s_screenFaderNodeParams;

        static void AddSupportedParams(const char* sName, AnimParamType paramId, AnimValueType valueType)
        {
            CAnimNode::SParamInfo param;
            param.name = sName;
            param.paramType = paramId;
            param.valueType = valueType;
            param.flags = IAnimNode::eSupportedParamFlags_MultipleTracks;
            s_screenFaderNodeParams.push_back(param);
        }    

        static bool CalculateIsolatedKeyColor(const IScreenFaderKey& key, float fTime, AZ::Vector4& colorOut)
        {
            float ratio = fTime - key.time;

            if (ratio < 0.f)
            {
                return false;
            }

            if (key.m_fadeTime == 0.f)
            {
                colorOut.Set(key.m_fadeColor.GetR(), key.m_fadeColor.GetG(), key.m_fadeColor.GetB(), key.m_fadeColor.GetA());
                if (key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
                {
                    colorOut.SetW(0.f);
                }
                else
                {
                    colorOut.SetW(1.f);
                }
            }
            else
            {
                colorOut.Set(key.m_fadeColor.GetR(), key.m_fadeColor.GetG(), key.m_fadeColor.GetB(), key.m_fadeColor.GetA());
                ratio = ratio / key.m_fadeTime;
                if (key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
                {
                    colorOut.SetW(AZStd::max(0.f, 1.f - ratio));
                }
                else
                {
                    colorOut.SetW(AZStd::min(1.f, ratio));
                }
            }

            return true;
        }
    } // namespace AnimScreenFaderNodeHelper

    CAnimScreenFaderNode::CAnimScreenFaderNode(const int id)
        : CAnimNode(id, AnimNodeType::ScreenFader)
        , m_bActive(false)
        , m_lastActivatedKey(-1)
        , m_texPrecached(false)
    {
        m_startColor = AZ::Vector4(1, 1, 1, 1);
        CAnimScreenFaderNode::Initialize();
        PrecacheTexData();
    }

    CAnimScreenFaderNode::CAnimScreenFaderNode()
        : CAnimScreenFaderNode(0)
    {
    }

    CAnimScreenFaderNode::~CAnimScreenFaderNode()
    {
    }

    void CAnimScreenFaderNode::Initialize()
    {
        using namespace AnimScreenFaderNodeHelper;
        if (!s_screenFaderNodeParamsInitialized)
        {
            s_screenFaderNodeParamsInitialized = true;
            s_screenFaderNodeParams.reserve(1);
            AddSupportedParams("Fader", AnimParamType::ScreenFader, AnimValueType::Unknown);
        }
    }

    void CAnimScreenFaderNode::Animate(SAnimContext& ac)
    {
        const auto nScreenFaderTracksNumber = static_cast<unsigned int>(m_tracks.size());

        for (unsigned int nFaderTrackNo = 0; nFaderTrackNo < nScreenFaderTracksNumber; ++nFaderTrackNo)
        {
            CScreenFaderTrack* pTrack =static_cast<CScreenFaderTrack*>(GetTrackForParameter(AnimParamType::ScreenFader, nFaderTrackNo));

            if (!pTrack)
            {
                AZ_Assert(false, "AnimTrack at %u is null", nFaderTrackNo);
                continue;
            }

            if (pTrack->GetNumKeys() == 0)
            {
                continue;
            }

            if (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
            {
                continue;
            }

            if (pTrack->IsMasked(ac.trackMask))
            {
                continue;
            }

            if (ac.singleFrame)
            {
                m_lastActivatedKey = -1;
            }

            IScreenFaderKey key;
            int nActiveKeyIndex = pTrack->GetActiveKey(ac.time, &key);

            if (nActiveKeyIndex >= 0)
            {
                if (m_lastActivatedKey != nActiveKeyIndex)
                {
                    m_lastActivatedKey = nActiveKeyIndex;
                    m_bActive = true;

                    if (!key.m_strTexture.empty())
                    {
                        if (pTrack->SetActiveTexture(nActiveKeyIndex))
                        {
                            pTrack->SetTextureVisible(true);
                        }
                        else
                        {
                            pTrack->SetTextureVisible(false);
                        }
                    }
                    else
                    {
                        pTrack->SetTextureVisible(false);
                    }
                }

                if (m_bActive || key.m_fadeTime + key.time > ac.time)
                {
                    float ratio = (key.m_fadeTime > 0) ? (ac.time - key.time) / key.m_fadeTime : 1.f;
                    if (ratio < 0.f)
                    {
                        ratio = 0.f;
                    }
                    ratio = MIN(ratio, 1.f);

                    switch (key.m_fadeChangeType)
                    {
                    case IScreenFaderKey::eFCT_Square:
                        ratio = ratio * ratio;
                        break;

                    case IScreenFaderKey::eFCT_CubicSquare:
                        ratio = ratio * ratio * ratio;
                        break;

                    case IScreenFaderKey::eFCT_SquareRoot:
                        ratio = sqrt(ratio);
                        break;

                    case IScreenFaderKey::eFCT_Sin:
                        ratio = sinf(ratio * 3.14159265f * 0.5f);
                        break;
                    }

                    if (!key.m_bUseCurColor || nActiveKeyIndex == 0)
                    {
                        m_startColor = key.m_fadeColor.GetAsVector4();
                    }
                    else
                    {
                        IScreenFaderKey preKey;
                        pTrack->GetKey(nActiveKeyIndex - 1, &preKey);
                        AnimScreenFaderNodeHelper::CalculateIsolatedKeyColor(preKey, ac.time, m_startColor);
                    }

                    if (key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
                    {
                        if (!key.m_bUseCurColor || nActiveKeyIndex == 0)
                        {
                            m_startColor.SetW(1.f);
                        }
                        key.m_fadeColor.SetA(0.f);
                    }
                    else
                    {
                        if (!key.m_bUseCurColor || nActiveKeyIndex == 0)
                        {
                            m_startColor.SetW(0.f);
                        }
                        key.m_fadeColor.SetA(1.f);
                    }

                    pTrack->SetDrawColor(m_startColor + (key.m_fadeColor.GetAsVector4() - m_startColor) * ratio);

                    if (pTrack->GetDrawColor().GetW() < 0.01f)
                    {
                        m_bActive = IsAnyTextureVisible();
                    }
                    else
                    {
                        m_bActive = true;
                    }
                }
            }
            else
            {
                pTrack->SetTextureVisible(false);
                m_bActive = IsAnyTextureVisible();
            }
        }
        // Actual drawing made by Render() is postponed till OnPostRender() event.
    }

    void CAnimScreenFaderNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::ScreenFader);
    }

    void CAnimScreenFaderNode::OnReset()
    {
        CAnimNode::OnReset();
        m_bActive = false;
    }

    void CAnimScreenFaderNode::Activate(bool bActivate)
    {
        if (bActivate)
        {
            m_bActive = false;
        }

        if (m_texPrecached == false)
        {
            PrecacheTexData();
        }
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    void CAnimScreenFaderNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);

        if (bLoading)
        {
            PrecacheTexData();
        }
    }

    void CAnimScreenFaderNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimScreenFaderNode, CAnimNode>()->Version(1);
        }
    }

    unsigned int CAnimScreenFaderNode::GetParamCount() const
    {
        return static_cast<unsigned int>(AnimScreenFaderNodeHelper::s_screenFaderNodeParams.size());
    }

    CAnimParamType CAnimScreenFaderNode::GetParamType(unsigned int nIndex) const
    {
        using namespace AnimScreenFaderNodeHelper;
        if (nIndex < s_screenFaderNodeParams.size())
        {
            return s_screenFaderNodeParams[nIndex].paramType;
        }
        return AnimParamType::Invalid;
    }

    void CAnimScreenFaderNode::SetFlags(int flags)
    {
        // call base class implementation. I'm avoiding the use of the Microsoft specific __super::SetFlags(flags) because it is not
        // platform agnostic
        CAnimNode::SetFlags(flags);

        if (flags & eAnimNodeFlags_Disabled)
        {
            // for screen faders, when disabling, we want to reset so the screen doesn't stay partially faded if a fade was in
            // effect when disabled
            OnReset();
        }
    }

    bool CAnimScreenFaderNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
    {
        using namespace AnimScreenFaderNodeHelper;
        for (size_t i = 0; i < s_screenFaderNodeParams.size(); ++i)
        {
            if (s_screenFaderNodeParams[i].paramType == paramId)
            {
                info = s_screenFaderNodeParams[i];
                return true;
            }
        }
        return false;
    }

    void CAnimScreenFaderNode::Render()
    {
        if (!m_bActive)
        {
            return;
        }

        const auto paramCount = static_cast<int>(m_tracks.size());
        for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            CScreenFaderTrack* pTrack = static_cast<CScreenFaderTrack*>(GetTrackForParameter(AnimParamType::ScreenFader, paramIndex));

            if (!pTrack)
            {
                continue;
            }

            // TODO : https://github.com/o3de/o3de/issues/6169, legacy code was:
            #if 0
            if (gEnv->pRenderer)
            {
                int textureId = -1;
                if (pTrack->IsTextureVisible())
                {
                    textureId = (pTrack->GetActiveTexture() != 0) ? pTrack->GetActiveTexture()->GetTextureID() : -1;
                }
                gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
                gEnv->pRenderer->Draw2dImage(0, 0, m_screenWidth, m_screenHeight, textureId, 0.0f, 1.0f, 1.0f, 0.0f, 0.f,
                pTrack->GetDrawColor().x, pTrack->GetDrawColor().y, pTrack->GetDrawColor().z, pTrack->GetDrawColor().w, 0.f);
            }
            #endif // 0

            // Actual  code could be something like this:
            AZ::Data::Instance<AZ::RPI::Image> image = nullptr; // Legal for no texture, just color fading
            if (pTrack->IsTextureVisible())
            {
                image = pTrack->GetActiveTexture();
            }

            #if 0
            auto draw2d = AZ::Interface<IDraw2dSystem>::Get();
            if (!draw2d)
            {
                AZ_Assert(false, "Draw2dSystem not found.");
                return;
            }

            const auto color = pTrack->GetDrawColor();
            IDraw2dSystem::ImageOptions imageOptions;
            imageOptions.m_color = color.GetAsVector3();
            imageOptions.m_clamp = true;
            imageOptions.m_renderState.m_blendState.m_enable = true;
            imageOptions.m_renderState.m_blendState.m_blendSource = AZ::RHI::BlendFactor::AlphaSource;
            imageOptions.m_renderState.m_blendState.m_blendDest = AZ::RHI::BlendFactor::AlphaSourceInverse;
            imageOptions.m_renderState.m_depthState.m_enable = false;

            draw2d->DrawImage(image, AZ::Vector2::CreateZero(), windowSize, color.GetW(), 0.0f, nullptr, nullptr, &imageOptions);
            #endif // 0
        }
    }

    bool CAnimScreenFaderNode::IsAnyTextureVisible() const
    {
        auto const paramCount = static_cast<unsigned int>(m_tracks.size());
        for (unsigned int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            CScreenFaderTrack* pTrack = static_cast<CScreenFaderTrack*>(GetTrackForParameter(AnimParamType::ScreenFader, paramIndex));
            if (!pTrack)
            {
                continue;
            }

            if (pTrack->IsTextureVisible())
            {
                return true;
            }
        }

        return false;
    }

    void CAnimScreenFaderNode::PrecacheTexData()
    {
        auto const paramCount = static_cast<unsigned int>(m_tracks.size());
        for (unsigned int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            IAnimTrack* pTrack = m_tracks[paramIndex].get();
            if (!pTrack)
            {
                continue;
            }

            switch (m_tracks[paramIndex]->GetParameterType().GetType())
            {
            case AnimParamType::ScreenFader:
                {
                    CScreenFaderTrack* pFaderTrack = static_cast<CScreenFaderTrack*>(pTrack);
                    pFaderTrack->PreloadTextures();
                }
                break;
            }
        }
        m_texPrecached = true;
    }

} // namespace Maestro
