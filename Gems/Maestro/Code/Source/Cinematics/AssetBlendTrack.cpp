/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Maestro/Types/AssetBlends.h>
#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>
#include "AssetBlendTrack.h"
#include "Maestro/Types/AnimValueType.h"

#define LOOP_TRANSITION_TIME 1.0f


//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CAssetBlendTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    return TAnimTrack<AZ::IAssetBlendKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CAssetBlendTrack::SerializeKey(AZ::IAssetBlendKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        // Read the AssetId made up of the guid and the sub id.
        key.m_assetId.SetInvalid();
        const char* assetIdGuidStr = keyNode->getAttr("assetIdGuid");
        if (assetIdGuidStr != nullptr && assetIdGuidStr[0] != 0)
        {
            AZ::Uuid guid(assetIdGuidStr, strlen(assetIdGuidStr));
            AZ::u32 assetIdSubId = 0;
            keyNode->getAttr("assetIdSubId", assetIdSubId);
            key.m_assetId = AZ::Data::AssetId(guid, assetIdSubId);
        }

        const char* descriptionIdStr = keyNode->getAttr("description");
        if (descriptionIdStr != nullptr)
        {
            key.m_description = descriptionIdStr;
        }

        key.m_duration = 0;
        key.m_endTime = 0;
        key.m_startTime = 0;
        key.m_bLoop = false;
        key.m_speed = 1;
        keyNode->getAttr("length", key.m_duration);
        keyNode->getAttr("end", key.m_endTime);
        keyNode->getAttr("speed", key.m_speed);
        keyNode->getAttr("loop", key.m_bLoop);
        keyNode->getAttr("start", key.m_startTime);
        keyNode->getAttr("blendInTime", key.m_blendInTime);
        keyNode->getAttr("blendOutTime", key.m_blendOutTime);
    }
    else
    {
        if (key.m_assetId.IsValid())
        {            
            XmlString temp = key.m_assetId.m_guid.ToString<AZStd::string>().c_str();
            keyNode->setAttr("assetIdGuid", temp);
            keyNode->setAttr("assetIdSubId", key.m_assetId.m_subId);
        }
        if (!key.m_description.empty())
        {
            XmlString temp = key.m_description.c_str();
            keyNode->setAttr("description", temp);
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
        if (key.m_startTime != 0)
        {
            keyNode->setAttr("start", key.m_startTime);
        }
        if (key.m_blendInTime != 0)
        {
            keyNode->setAttr("blendInTime", key.m_blendInTime);
        }
        if (key.m_blendOutTime != 0)
        {
            keyNode->setAttr("blendOutTime", key.m_blendOutTime);
        }
    }
}

void CAssetBlendTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    if (m_keys[key].m_assetId.IsValid())
    {
        description = m_keys[key].m_description.c_str();

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

float CAssetBlendTrack::GetKeyDuration(int key) const
{
    assert(key >= 0 && key < (int)m_keys.size());
    const float EPSILON = 0.001f;
    if (m_keys[key].m_bLoop)
    {
        float lastTime = m_timeRange.end;
        if (key + 1 < (int)m_keys.size())
        {
            // EPSILON is required to ensure the correct ordering when getting nearest keys.
            lastTime = m_keys[key + 1].time + std::min(LOOP_TRANSITION_TIME,
                    GetKeyDuration(key + 1) - EPSILON);
        }
        // duration is unlimited but cannot last past end of track or time of next key on track.
        return std::max(lastTime - m_keys[key].time, 0.0f);
    }
    else
    {
        return m_keys[key].GetActualDuration();
    }
}

//////////////////////////////////////////////////////////////////////////
static bool AssetBlendTrackVersionConverter(
    AZ::SerializeContext& serializeContext,
    AZ::SerializeContext::DataElementNode& rootElement)
{
    if (rootElement.GetVersion() < 3)
    {
        rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
    }

    return true;
}

template<>
inline void TAnimTrack<AZ::IAssetBlendKey>::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<TAnimTrack<AZ::IAssetBlendKey>, IAnimTrack>()
            ->Version(3, &AssetBlendTrackVersionConverter)
            ->Field("Flags", &TAnimTrack<AZ::IAssetBlendKey>::m_flags)
            ->Field("Range", &TAnimTrack<AZ::IAssetBlendKey>::m_timeRange)
            ->Field("ParamType", &TAnimTrack<AZ::IAssetBlendKey>::m_nParamType)
            ->Field("Keys", &TAnimTrack<AZ::IAssetBlendKey>::m_keys)
            ->Field("Id", &TAnimTrack<AZ::IAssetBlendKey>::m_id);
    }
}

//////////////////////////////////////////////////////////////////////////
AnimValueType CAssetBlendTrack::GetValueType()
{
    return AnimValueType::AssetBlend; 
}

//////////////////////////////////////////////////////////////////////////
void CAssetBlendTrack::GetValue(float time, Maestro::AssetBlends<AZ::Data::AssetData>& value)
{
    // Start by clearing all the assets.
    m_assetBlend.m_assetBlends.clear();

    // Keep track of the nearest keys to be used if not key is found at the input time.
    bool foundPreviousKey = false;
    bool foundNextKey = false;
    float previousKeyTimeDistance = FLT_MAX;
    float nextKeyTimeDistance = FLT_MAX;
    AZ::IAssetBlendKey previousKey;
    AZ::IAssetBlendKey nextKey;

    // Check each key to see if it has an asset that is in time range right now.
    for (auto key : m_keys)
    {
        float localTime = time - key.time;
        float segmentLength = key.GetValidEndTime() - key.m_startTime;

        if (key.IsInRange(time) && key.m_assetId.IsValid())
        {
            float segmentPercent = localTime / (segmentLength / key.GetValidSpeed());
            m_assetBlend.m_assetBlends.push_back(Maestro::AssetBlend(key.m_assetId, key.m_startTime + (segmentLength * segmentPercent), key.m_blendInTime, key.m_blendOutTime));
        }

        // Find the nearest previous key
        if (key.time < time)
        {
            if (abs(time - key.time) < previousKeyTimeDistance)
            {
                previousKeyTimeDistance = abs(time - key.time);
                previousKey = key;
                foundPreviousKey = true;
            }

        }

        // Find the nearest next key
        if (key.time > time)
        {
            if (abs(time - key.time) < nextKeyTimeDistance)
            {
                nextKeyTimeDistance = abs(time - key.time);
                nextKey = key;
                foundNextKey = true;
            }
        }
    }

    // If no asset blends have been added, and there is a key somewhere in the time line,
    // add the first or last frame of the key.
    if (m_assetBlend.m_assetBlends.empty() && !m_keys.empty())
    {
        // Check for looping the anim on the last key
        if (foundPreviousKey && previousKey.m_bLoop)
        {
            float localTime = time - previousKey.time;
            float segmentLength = previousKey.GetValidEndTime() - previousKey.m_startTime;
            float segmentPercent = static_cast<float>(fmod(localTime / (segmentLength / previousKey.GetValidSpeed()), 1.0));
            m_assetBlend.m_assetBlends.push_back(Maestro::AssetBlend(previousKey.m_assetId, previousKey.m_startTime + (segmentLength * segmentPercent), previousKey.m_blendInTime, previousKey.m_blendOutTime));
        }
        else
        {
            // Nothing set, just freeze frame on the first or last frame of the nearest animation
            if (!foundPreviousKey && foundNextKey)
            {
                m_assetBlend.m_assetBlends.push_back(Maestro::AssetBlend(nextKey.m_assetId, nextKey.m_startTime, nextKey.m_blendInTime, nextKey.m_blendOutTime));
            }
            else if (foundPreviousKey)
            {
                // Add a small fudge factor to the end time so the animation will be frozen on the last frame if we play off the end
                // of an animation and there is no other animation set on the tack.
                m_assetBlend.m_assetBlends.push_back(Maestro::AssetBlend(previousKey.m_assetId, previousKey.GetValidEndTime() - 0.001f, previousKey.m_blendInTime, previousKey.m_blendOutTime));
            }
        }
    }

    // Return the updated assetBlend
    value = m_assetBlend;
}

//////////////////////////////////////////////////////////////////////////
void CAssetBlendTrack::SetDefaultValue(const Maestro::AssetBlends<AZ::Data::AssetData>& defaultValue)
{
    m_defaultValue = defaultValue;
    Invalidate();
}

//////////////////////////////////////////////////////////////////////////
float CAssetBlendTrack::GetEndTime() const
{
    return m_timeRange.end; 
}

//////////////////////////////////////////////////////////////////////////
void CAssetBlendTrack::Reflect(AZ::ReflectContext* context)
{
    TAnimTrack<AZ::IAssetBlendKey>::Reflect(context);

    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAssetBlendTrack, TAnimTrack<AZ::IAssetBlendKey>>()
            ->Version(1);
    }
}
