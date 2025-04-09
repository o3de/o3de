/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include "CharacterTrack.h"
#include "Maestro/Types/AnimValueType.h"

namespace Maestro
{

    namespace
    {
        constexpr float LoopTransitionTime = 1.0f;
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    bool CCharacterTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        if (bLoading)
        {
            xmlNode->getAttr("AnimationLayer", m_iAnimationLayer);
        }
        else
        {
            xmlNode->setAttr("AnimationLayer", m_iAnimationLayer);
        }

        return TAnimTrack<ICharacterKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    }

    void CCharacterTrack::SerializeKey(ICharacterKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* str;

            str = keyNode->getAttr("anim");
            key.m_animation = str;

            key.m_duration = 0;
            key.m_endTime = 0;
            key.m_startTime = 0;
            key.m_bLoop = false;
            key.m_bBlendGap = false;
            key.m_bInPlace = false;
            key.m_speed = 1;
            keyNode->getAttr("length", key.m_duration);
            keyNode->getAttr("end", key.m_endTime);
            keyNode->getAttr("speed", key.m_speed);
            keyNode->getAttr("loop", key.m_bLoop);
            keyNode->getAttr("blendGap", key.m_bBlendGap);
            keyNode->getAttr("inplace", key.m_bInPlace);
            keyNode->getAttr("start", key.m_startTime);

            if (key.m_speed < AZ::Constants::Tolerance)
            {
                key.m_speed = 1.0f;
            }
        }
        else
        {
            if (!key.m_animation.empty())
            {
                XmlString temp = key.m_animation.c_str();
                keyNode->setAttr("anim", temp);
            }
            if (key.m_duration > 0)
            {
                keyNode->setAttr("length", key.m_duration);
            }
            if (key.m_endTime > 0)
            {
                keyNode->setAttr("end", key.m_endTime);
            }
            if (key.m_speed != 1)
            {
                keyNode->setAttr("speed", key.m_speed);
            }
            if (key.m_bLoop)
            {
                keyNode->setAttr("loop", key.m_bLoop);
            }
            if (key.m_bBlendGap)
            {
                keyNode->setAttr("blendGap", key.m_bBlendGap);
            }
            if (key.m_bInPlace)
            {
                keyNode->setAttr("inplace", key.m_bInPlace);
            }
            if (key.m_startTime != 0)
            {
                keyNode->setAttr("start", key.m_startTime);
            }
        }
    }

    void CCharacterTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        const auto numKeys = GetNumKeys();
        if (keyIndex < 0 || keyIndex >= numKeys)
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, numKeys);
            return;
        }

        if (!m_keys[keyIndex].m_animation.empty())
        {
            description = m_keys[keyIndex].m_animation.c_str();
            if (m_keys[keyIndex].m_bLoop)
            {
                float lastTime = m_timeRange.end;
                if (keyIndex + 1 < numKeys)
                {
                    lastTime = m_keys[keyIndex + 1].time;
                }
                // duration is unlimited but cannot last past end of track or time of next key on track.
                duration = lastTime - m_keys[keyIndex].time;
            }
            else
            {
                duration = m_keys[keyIndex].GetActualDuration();
            }
        }
    }

    float CCharacterTrack::GetKeyDuration(int keyIndex) const
    {
        const auto numKeys = GetNumKeys();
        if (keyIndex < 0 || keyIndex >= numKeys)
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, numKeys);
            return 0.0f;
        }

        const float EPSILON = 0.001f;
        if (m_keys[keyIndex].m_bLoop)
        {
            float lastTime = m_timeRange.end;
            if (keyIndex + 1 < numKeys)
            {
                // EPSILON is required to ensure the correct ordering when getting nearest keys.
                lastTime = m_keys[keyIndex + 1].time + AZStd::min(LoopTransitionTime, GetKeyDuration(keyIndex + 1) - EPSILON);
            }
            // duration is unlimited but cannot last past end of track or time of next key on track.
            return AZStd::max(lastTime - m_keys[keyIndex].time, 0.0f);
        }
        else
        {
            return m_keys[keyIndex].GetActualDuration();
        }
    }

    static bool CharacterTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ICharacterKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ICharacterKey>, IAnimTrack>()
                ->Version(3, &CharacterTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ICharacterKey>::m_flags)
                ->Field("Range", &TAnimTrack<ICharacterKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ICharacterKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ICharacterKey>::m_keys)
                ->Field("Id", &TAnimTrack<ICharacterKey>::m_id);
        }
    }

    AnimValueType CCharacterTrack::GetValueType() const
    {
        return AnimValueType::CharacterAnim;
    }

    void CCharacterTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ICharacterKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CCharacterTrack, TAnimTrack<ICharacterKey>>()->Version(1)->Field(
                "AnimationLayer", &CCharacterTrack::m_iAnimationLayer);
        }
    }

} // namespace Maestro
