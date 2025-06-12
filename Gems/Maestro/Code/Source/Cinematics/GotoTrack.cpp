/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "GotoTrack.h"
#include "Maestro/Types/AnimValueType.h"

namespace Maestro
{
    CGotoTrack::CGotoTrack()
    {
        m_flags = 0;
        m_DefaultValue = -1.0f;
    }

    AnimValueType CGotoTrack::GetValueType() const
    {
        return AnimValueType::DiscreteFloat;
    }

    void CGotoTrack::GetValue(float time, float& value, bool applyMultiplier) const
    {
        size_t nTotalKeys(m_keys.size());

        value = m_DefaultValue;

        if (nTotalKeys < 1)
        {
            return;
        }

        size_t nKey(0);
        for (nKey = 0; nKey < nTotalKeys; ++nKey)
        {
            if (time >= m_keys[nKey].time)
            {
                value = m_keys[nKey].m_fValue;
            }
            else
            {
                break;
            }
        }

        if (applyMultiplier && m_trackMultiplier != 1.0f && m_trackMultiplier > AZ::Constants::Tolerance)
        {
            value /= m_trackMultiplier;
        }
    }

    void CGotoTrack::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
    {
        if (!bDefault)
        {
            if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
            {
                AZ_Warning("GotoTrack",false,"SetValue(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                    time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
                AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
            }

            IDiscreteFloatKey oKey;
            if (applyMultiplier && m_trackMultiplier != 1.0f)
            {
                oKey.SetValue(value * m_trackMultiplier);
            }
            else
            {
                oKey.SetValue(value);
            }
            SetKeyAtTime(time, &oKey);
        }
        else
        {
            if (applyMultiplier && m_trackMultiplier != 1.0f)
            {
                m_DefaultValue = value * m_trackMultiplier;
            }
            else
            {
                m_DefaultValue = value;
            }
        }
    }

    void CGotoTrack::SerializeKey(IDiscreteFloatKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            keyNode->getAttr("time", key.time);
            keyNode->getAttr("value", key.m_fValue);

            keyNode->getAttr("flags", key.flags);
        }
        else
        {
            keyNode->setAttr("time", key.time);
            keyNode->setAttr("value", key.m_fValue);

            int flags = key.flags;
            if (flags != 0)
            {
                keyNode->setAttr("flags", flags);
            }
        }
    }

    void CGotoTrack::GetKeyInfo(int keyIndex, const char*& description, [[maybe_unused]] float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }


        static char str[64];
        description = str;
        const float& k = m_keys[keyIndex].m_fValue;
        azsnprintf(str, AZ_ARRAY_SIZE(str), "%.2f", k);
    }

    void CGotoTrack::SetKeyAtTime(float time, IKey* key)
    {
        if (!key)
        {
            AZ_Assert(key, "Expected valid key pointer.");
            return;
        }

        if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_WarningOnce("GotoTrack", false, "SetValue(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        key->time = time;

        bool found = false;
        // Find key with given time.
        for (int i = 0; i < GetNumKeys(); i++)
        {
            float keyt = m_keys[i].time;
            if (fabs(keyt - time) < GetMinKeyTimeDelta())
            {
                key->flags = m_keys[i].flags; // Reserve the flag value.
                SetKey(i, key);
                found = true;
                break;
            }
            // if (keyt > time)
            // break;
        }
        if (!found)
        {
            // Key with this time not found.
            // Create a new one.
            int keyIndex = CreateKey(time);
            // Reserve the flag value.
            key->flags = m_keys[keyIndex].flags; // Reserve the flag value.
            SetKey(keyIndex, key);
        }

        SortKeys();
    }

    static bool GotoTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<IDiscreteFloatKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<IDiscreteFloatKey>, IAnimTrack>()
                ->Version(3, &GotoTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<IDiscreteFloatKey>::m_flags)
                ->Field("Range", &TAnimTrack<IDiscreteFloatKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<IDiscreteFloatKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<IDiscreteFloatKey>::m_keys)
                ->Field("Id", &TAnimTrack<IDiscreteFloatKey>::m_id);
        }
    }

    void CGotoTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<IDiscreteFloatKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CGotoTrack, TAnimTrack<IDiscreteFloatKey>>()->Version(1);
        }
    }

} // namespace Maestro
