/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Maestro/Types/AssetBlends.h>
#include <Maestro/Types/AssetBlendKey.h>
#include "AssetBlendTrack.h"
#include "Maestro/Types/AnimValueType.h"

namespace Maestro
{

    namespace
    {
        constexpr float LoopTransitionTime = 1.0f;
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
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

            if (key.m_speed < AZ::Constants::Tolerance)
            {
                key.m_speed = 1;
            }
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
            if (key.m_duration > AZ::Constants::Tolerance)
            {
                keyNode->setAttr("length", key.m_duration);
            }
            if (key.m_endTime > AZ::Constants::Tolerance)
            {
                keyNode->setAttr("end", key.m_endTime);
            }
            if (AZStd::abs(key.m_speed - 1.0f) > AZ::Constants::Tolerance)
            {
                AZStd::clamp(key.m_speed, AZ::IAssetBlendKey::s_minSpeed, AZ::IAssetBlendKey::s_maxSpeed);
                keyNode->setAttr("speed", key.m_speed);
            }
            if (key.m_bLoop)
            {
                keyNode->setAttr("loop", key.m_bLoop);
            }
            if (key.m_startTime > AZ::Constants::Tolerance)
            {
                keyNode->setAttr("start", key.m_startTime);
            }
            if (key.m_blendInTime > AZ::Constants::Tolerance)
            {
                keyNode->setAttr("blendInTime", key.m_blendInTime);
            }
            if (key.m_blendOutTime > AZ::Constants::Tolerance)
            {
                keyNode->setAttr("blendOutTime", key.m_blendOutTime);
            }
        }
    }

    void CAssetBlendTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        if (m_keys[keyIndex].m_assetId.IsValid())
        {
            description = m_keys[keyIndex].m_description.c_str();

            if (m_keys[keyIndex].m_bLoop)
            {
                float lastTime = m_timeRange.end;
                if (keyIndex + 1 < (int)m_keys.size())
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

    float CAssetBlendTrack::GetKeyDuration(int keyIndex) const
    {
        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return 0.0f;
        }

        const float EPSILON = 0.001f;
        if (m_keys[keyIndex].m_bLoop)
        {
            float lastTime = m_timeRange.end;
            if (keyIndex + 1 < (int)m_keys.size())
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

    static bool AssetBlendTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
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

    AnimValueType CAssetBlendTrack::GetValueType() const
    {
        return AnimValueType::AssetBlend;
    }

    void CAssetBlendTrack::GetValue(float time, AssetBlends<AZ::Data::AssetData>& value) const
    {
        // Start by clearing all the assets.
        AssetBlends<AZ::Data::AssetData> result;
        result.m_assetBlends.clear();

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
                result.m_assetBlends.push_back(AssetBlend(
                    key.m_assetId,
                    key.m_startTime + (segmentLength * segmentPercent),
                    key.m_blendInTime,
                    key.m_blendOutTime,
                    key.m_speed,
                    key.m_bLoop));
            }

            // Find the nearest previous key
            if (key.time < time && key.m_assetId.IsValid())
            {
                if (abs(time - key.time) < previousKeyTimeDistance)
                {
                    previousKeyTimeDistance = abs(time - key.time);
                    previousKey = key;
                    foundPreviousKey = true;
                }
            }

            // Find the nearest next key
            if (key.time > time && key.m_assetId.IsValid())
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
        if (result.m_assetBlends.empty() && !m_keys.empty())
        {
            // Check for looping the animation on the last key
            if (foundPreviousKey && previousKey.m_bLoop)
            {
                float localTime = time - previousKey.time;
                float segmentLength = previousKey.GetValidEndTime() - previousKey.m_startTime;
                float segmentPercent = static_cast<float>(fmod(localTime / (segmentLength / previousKey.GetValidSpeed()), 1.0));
                result.m_assetBlends.push_back(AssetBlend(
                    previousKey.m_assetId,
                    previousKey.m_startTime + (segmentLength * segmentPercent),
                    previousKey.m_blendInTime,
                    previousKey.m_blendOutTime,
                    previousKey.m_speed,
                    previousKey.m_bLoop));
            }
            else
            {
                // Nothing set, just freeze frame on the first or last frame of the nearest animation
                if (!foundPreviousKey && foundNextKey)
                {
                    result.m_assetBlends.push_back(AssetBlend(
                        nextKey.m_assetId,
                        nextKey.m_startTime,
                        nextKey.m_blendInTime,
                        nextKey.m_blendOutTime,
                        nextKey.m_speed,
                        nextKey.m_bLoop));
                }
                else if (foundPreviousKey)
                {
                    // Add a small fudge factor to the end time so the animation will be frozen on the last frame if we play off the end
                    // of an animation and there is no other animation set on the tack.
                    result.m_assetBlends.push_back(AssetBlend(
                        previousKey.m_assetId,
                        previousKey.GetValidEndTime() - 0.001f,
                        previousKey.m_blendInTime,
                        previousKey.m_blendOutTime,
                        previousKey.m_speed,
                        previousKey.m_bLoop));
                }
            }
        }

        // Return the updated assetBlend
        value = result;
    }

    void CAssetBlendTrack::SetKeysAtTime(float time, const AssetBlends<AZ::Data::AssetData>& value)
    {
        if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_WarningOnce("AssetBlendTrack", false, "SetKeysAtTime(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        ClearKeys();
        for (const auto& blend : value.m_assetBlends)
        {
            if (!blend.m_assetId.IsValid())
            {
                continue; // filter out blends for invalid AssetIds
            }

            AZ::IAssetBlendKey key;
            key.m_assetId = blend.m_assetId;
            key.m_description.clear(); // could be parsed to asset filename after requesting AssetData by Id
            key.m_blendInTime = blend.m_blendInTime;
            key.m_blendOutTime = blend.m_blendOutTime;
            // IKey (key.flags nullified in ctor)
            key.time = time + blend.m_time;

            // Check that the key for the asset is unique in time-line
            bool isUnique = true;
            for (const auto& previousKey : m_keys)
            {
                if ((previousKey.m_assetId == key.m_assetId) && (AZStd::abs(previousKey.time - key.time) < GetMinKeyTimeDelta()))
                {
                    isUnique = false;
                    break;
                }
            }
            if (isUnique)
            {
                m_keys.push_back(key);
            }
        }

        SortKeys(); // sorting by key.time

        m_lastTime = time;
        m_currKey = 0;
        m_fMinKeyValue = 0;
        m_fMaxKeyValue = 0;
        const auto keysCount = m_keys.size();
        for (size_t i = 0; i < keysCount; ++i)
        {
            auto& key = m_keys[i];
            // Try to restore values for ITimeRangeKey - not all values can be restored, information on duration and looping is missing
            key.m_startTime = key.time;
            if (keysCount > 1 && i < keysCount - 1) // not the last key ?
            {
                key.m_duration = m_keys[i + 1].time - key.time; // prolong to the next key
            }
            else
            {
                key.m_duration =
                    key.m_blendInTime + key.m_blendOutTime + AZ::Constants::Tolerance; // set at least enough time to fade in/out
            }
            key.m_endTime = key.m_startTime + key.m_duration;
            key.m_speed = 1;
            key.m_bLoop = false;

            // Accumulate values for TAnimTrack<AZ::IAssetBlendKey>
            m_fMinKeyValue = (key.time < m_fMinKeyValue) ? key.time : m_fMinKeyValue;
            m_fMaxKeyValue = (key.time > m_fMaxKeyValue) ? key.time : m_fMaxKeyValue;
            key.m_bLoop = i < keysCount - 1 && key.m_endTime < m_keys[i + 1].m_startTime;
        }
        // TAnimTrack<AZ::IAssetBlendKey>
        if (keysCount == 0)
        {
            m_timeRange.start = 0.f;
            m_timeRange.end = 0.f;
        }
        else
        {
            m_timeRange.start = time;
            m_timeRange.end = m_keys.back().m_endTime;
        }
    }

    void CAssetBlendTrack::FilterBlends(
        const AssetBlends<AZ::Data::AssetData>& value, AssetBlends<AZ::Data::AssetData>& filteredValue) const
    {
        filteredValue.m_assetBlends.clear();
        for (const auto& nextBlend : value.m_assetBlends)
        {
            if (nextBlend.m_assetId.IsValid())
            {
                bool isUnique = true;
                for (const auto& previousBlend : filteredValue.m_assetBlends)
                {
                    if ((previousBlend.m_assetId == nextBlend.m_assetId) &&
                        (AZStd::abs(previousBlend.m_time - nextBlend.m_time) < GetMinKeyTimeDelta()))
                    {
                        isUnique = false;
                        break;
                    }
                }
                if (isUnique)
                {
                    filteredValue.m_assetBlends.push_back(nextBlend);
                }
            }
        }
    }

    void CAssetBlendTrack::ClearKeys()
    {
        // TAnimTrack
        m_keys.clear();
        m_currKey = 0;
        m_lastTime = -1;
        m_timeRange.Clear();
        m_fMinKeyValue = 0;
        m_fMaxKeyValue = 0;
    }

    void CAssetBlendTrack::SetValue(float time, const AssetBlends<AZ::Data::AssetData>& value, bool bDefault)
    {
        if (bDefault)
        {
            SetDefaultValue(time, value);
            return;
        }
        SetKeysAtTime(time, value);
    }

    void CAssetBlendTrack::SetDefaultValue(const AssetBlends<AZ::Data::AssetData>& defaultValue)
    {
        SetDefaultValue(0.f, defaultValue);
    }

    void CAssetBlendTrack::SetDefaultValue(float time, const AssetBlends<AZ::Data::AssetData>& defaultValue)
    {
        AssetBlends<AZ::Data::AssetData> fileteredValue;
        FilterBlends(defaultValue, fileteredValue);
        m_defaultValue = fileteredValue;

        SetKeysAtTime(time, fileteredValue);
    }

    void CAssetBlendTrack::GetDefaultValue(AssetBlends<AZ::Data::AssetData>& defaultValue) const
    {
        defaultValue = m_defaultValue;
    }

    float CAssetBlendTrack::GetEndTime() const
    {
        return m_timeRange.end;
    }

    void CAssetBlendTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<AZ::IAssetBlendKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAssetBlendTrack, TAnimTrack<AZ::IAssetBlendKey>>()->Version(1);
        }
    }

} // namespace Maestro
