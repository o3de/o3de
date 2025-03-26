/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "SceneNode.h"
#include "SelectTrack.h"
#include "Maestro/Types/AnimValueType.h"

namespace Maestro
{

    void CSelectTrack::SerializeKey(ISelectKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* szSelection;
            szSelection = keyNode->getAttr("node");
            key.szSelection = szSelection;
            AZ::u64 id64;
            if (keyNode->getAttr("CameraAzEntityId", id64))
            {
                key.cameraAzEntityId = AZ::EntityId(id64);
            }
            keyNode->getAttr("BlendTime", key.fBlendTime);
        }
        else
        {
            keyNode->setAttr("node", key.szSelection.c_str());
            if (key.cameraAzEntityId.IsValid())
            {
                AZ::u64 id64 = static_cast<AZ::u64>(key.cameraAzEntityId);
                keyNode->setAttr("CameraAzEntityId", id64);
            }
            keyNode->setAttr("BlendTime", key.fBlendTime);
        }
    }

    AnimValueType CSelectTrack::GetValueType()
    {
        return AnimValueType::Select;
    };

    void CSelectTrack::GetKeyInfo(int index, const char*& description, float& duration)
    {
        AZ_Assert(index >= 0 && index < (int)m_keys.size(), "Key index %i is out of range", index);
        CheckValid();
        auto& key = m_keys[index];
        if (key.CheckValid())
        {
            duration = key.fDuration;
            description = key.szSelection.c_str();
        }
        else
        {
            description = nullptr;
            duration = 0;
        }
    }

    void CSelectTrack::SetKey(int index, IKey* key)
    {
        const int numKeys = (int)m_keys.size();
        if (index < 0 || index >= numKeys || !key)
        {
            AZ_Assert(index >= 0 && index < numKeys, "CSelectTrack::SetKey(%d): Key index is out of range", index);
            AZ_Assert(key != 0, "CSelectTrack::SetKey(%d): Key cannot be null!", index);
            return;
        }

        ISelectKey oldKey = m_keys[index];
        ISelectKey newKey = *((ISelectKey*)key);
        if (oldKey.cameraAzEntityId != newKey.cameraAzEntityId)
        {
            newKey.ResetCameraProperies(); // stored camera parameters are now probably invalid
        }

        m_keys[index] = newKey; // Store the key

        CalculateDurationForEachKey(); // and sorted timeline

        if(!newKey.IsValid() || newKey.IsInitialized())
        {
            return; // Invalid or already initialized key - no need to search for stored camera parameters.
        }

        // Try to find a valid key with same camera controller EntityId in order to copy stored camera parameters.
        for (int idx = 0; idx < numKeys; ++idx)
        {
            if (idx != index)
            {
                ISelectKey& aKey = m_keys[idx];
                if (aKey.IsInitialized() && (aKey.cameraAzEntityId == newKey.cameraAzEntityId))
                {
                    m_keys[index].CopyCameraProperies(aKey);
                    return;
                }
            }
        }
        // Key keeps invalid camera properties, and until animation reset and re-activation,
        // the needed camera properties cannot be requested (could have been changed while playing animation).
    }

    void CSelectTrack::CalculateDurationForEachKey()
    {
        const int numKeys = (int)m_keys.size();
        if (numKeys < 1)
        {
            return;
        }

        SortKeys();

        const auto timeRangeEnd =
            (GetNode() && GetNode()->GetSequence())
            ? GetNode()->GetSequence()->GetTimeRange().end : m_keys[numKeys - 1].time;

        for (int currKeyIdx = 0; currKeyIdx < numKeys; ++currKeyIdx)
        {
            ISelectKey& currKey = m_keys[currKeyIdx];
            // find a next valid key in timeline
            int nextKeyNumber = -1;
            ISelectKey nextKey;
            for (int nextKeyIdx = currKeyIdx + 1; nextKeyIdx < numKeys; ++nextKeyIdx)
            {
                nextKey = m_keys[nextKeyIdx];
                if (nextKey.IsValid())
                {
                    nextKeyNumber = nextKeyIdx;
                    break;
                }
            }
            if (nextKeyNumber > 0)
            {
                currKey.fDuration = nextKey.time - currKey.time;
            }
            else
            {
                currKey.fDuration = AZStd::max(timeRangeEnd - currKey.time, 0.0f);
            }
        }
    }

    int CSelectTrack::GetActiveKey(float time, ISelectKey* key)
    {
        if (key == nullptr)
        {
            return -1;
        }

        const int nkeys = static_cast<int>(m_keys.size());
        if (nkeys == 0)
        {
            m_lastTime = time;
            m_currKey = -1;
            return m_currKey;
        }
        const int lastKeyIdx = nkeys - 1;

        CheckValid();

        m_lastTime = time;

        if (m_keys[0].time > time)
        {
            m_currKey = -1;
            return m_currKey; // Time is before first key.
        }

        if (m_currKey < 0)
        {
            m_currKey = 0;
        }

        // Start searching for a range between valid keys, containing given time, from the current key.
        auto currSelectKey = m_keys[m_currKey];
        if (currSelectKey.IsValid() && time >= currSelectKey.time)
        {
            for (int next = m_currKey + 1; next < nkeys; ++next)
            {
                auto nextSelectKey = m_keys[next];
                if (nextSelectKey.IsValid() && time >= nextSelectKey.time)
                {
                    m_currKey = next;
                    currSelectKey = nextSelectKey;
                }
                if ((next >= lastKeyIdx) || (nextSelectKey.IsValid() && (time <= nextSelectKey.time)))
                {
                    *key = currSelectKey;
                    return m_currKey;
                }
            }
        }

        // If not found from the current middle || last key, restart searching from the beginning.
        for (int idx = 0; idx < nkeys; ++idx)
        {
            auto& nextSelectKey = m_keys[idx];
            if (nextSelectKey.IsValid() && (time >= nextSelectKey.time)) // skip invalid keys
            {
                m_currKey = idx;
                *key = currSelectKey;
                return m_currKey;
            }
        }

        m_currKey = -1; // no valid keys in sequence
        return m_currKey;
    }

    static bool SelectTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<ISelectKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<ISelectKey>, IAnimTrack>()
                ->Version(3, &SelectTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<ISelectKey>::m_flags)
                ->Field("Range", &TAnimTrack<ISelectKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<ISelectKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<ISelectKey>::m_keys)
                ->Field("Id", &TAnimTrack<ISelectKey>::m_id);
        }
    }

    void CSelectTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<ISelectKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CSelectTrack, TAnimTrack<ISelectKey>>()->Version(1);
        }
    }

} // namespace Maestro
