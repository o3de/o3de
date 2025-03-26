/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "TimeRangesTrack.h"

namespace Maestro
{

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    bool CTimeRangesTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        return TAnimTrack<ITimeRangeKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    }

    void CTimeRangesTrack::SerializeKey(ITimeRangeKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            key.m_duration = 0;
            key.m_endTime = 0;
            key.m_startTime = 0;
            key.m_speed = 1;
            keyNode->getAttr("length", key.m_duration);
            keyNode->getAttr("end", key.m_endTime);
            keyNode->getAttr("speed", key.m_speed);
            keyNode->getAttr("start", key.m_startTime);
            keyNode->getAttr("loop", key.m_bLoop);
        }
        else
        {
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
            if (key.m_startTime != 0)
            {
                keyNode->setAttr("start", key.m_startTime);
            }
            if (key.m_bLoop)
            {
                keyNode->setAttr("loop", key.m_bLoop);
            }
        }
    }

    float CTimeRangesTrack::GetKeyDuration(int key) const
    {
        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        return m_keys[key].GetActualDuration();
    }

    void CTimeRangesTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        duration = GetKeyDuration(key);
        description = "";
    }

    int CTimeRangesTrack::GetActiveKeyIndexForTime(const float time)
    {
        const unsigned int numKeys = static_cast<unsigned int>(m_keys.size());

        if (numKeys == 0 || m_keys[0].time > time)
        {
            return -1;
        }

        int lastFound = 0;

        for (unsigned int i = 0; i < numKeys; ++i)
        {
            ITimeRangeKey& key = m_keys[i];
            if (key.time > time)
            {
                break;
            }
            else if (key.time <= time)
            {
                lastFound = i;
            }
        }

        return lastFound;
    }

    static bool TimeRangesTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ITimeRangeKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ITimeRangeKey>, IAnimTrack>()
                ->Version(3, &TimeRangesTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ITimeRangeKey>::m_flags)
                ->Field("Range", &TAnimTrack<ITimeRangeKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ITimeRangeKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ITimeRangeKey>::m_keys)
                ->Field("Id", &TAnimTrack<ITimeRangeKey>::m_id);
        }
    }

    void CTimeRangesTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ITimeRangeKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CTimeRangesTrack, TAnimTrack<ITimeRangeKey>>()->Version(1);
        }
    }

} // namespace Maestro
