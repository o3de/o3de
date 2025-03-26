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

    void CCharacterTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = 0;
        if (!m_keys[key].m_animation.empty())
        {
            description = m_keys[key].m_animation.c_str();
            if (m_keys[key].m_bLoop)
            {
                float lastTime = m_timeRange.end;
                if (key + 1 < (int)m_keys.size())
                {
                    lastTime = m_keys[key + 1].time;
                }
                // duration is unlimited but cannot last past end of track or time of next key on track.
                duration = lastTime - m_keys[key].time;
            }
            else
            {
                if (m_keys[key].m_speed == 0)
                {
                    m_keys[key].m_speed = 1.0f;
                }
                duration = m_keys[key].GetActualDuration();
            }
        }
    }

    float CCharacterTrack::GetKeyDuration(int key) const
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        const float EPSILON = 0.001f;
        if (m_keys[key].m_bLoop)
        {
            float lastTime = m_timeRange.end;
            if (key + 1 < (int)m_keys.size())
            {
                // EPSILON is required to ensure the correct ordering when getting nearest keys.
                lastTime = m_keys[key + 1].time + AZStd::min(LoopTransitionTime, GetKeyDuration(key + 1) - EPSILON);
            }
            // duration is unlimited but cannot last past end of track or time of next key on track.
            return AZStd::max(lastTime - m_keys[key].time, 0.0f);
        }
        else
        {
            return m_keys[key].GetActualDuration();
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

    AnimValueType CCharacterTrack::GetValueType()
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
