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

    namespace
    {
        constexpr float MinTimePrecision = 0.01f;
    }

    CGotoTrack::CGotoTrack()
    {
        m_flags = 0;
        m_DefaultValue = -1.0f;
    }

    AnimValueType CGotoTrack::GetValueType()
    {
        return AnimValueType::DiscreteFloat;
    }

    void CGotoTrack::GetValue(float time, float& value, bool applyMultiplier)
    {
        size_t nTotalKeys(m_keys.size());

        value = m_DefaultValue;

        if (nTotalKeys < 1)
        {
            return;
        }

        CheckValid();

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

        if (applyMultiplier && m_trackMultiplier != 1.0f)
        {
            value /= m_trackMultiplier;
        }
    }

    void CGotoTrack::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
    {
        if (!bDefault)
        {
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

    void CGotoTrack::GetKeyInfo(int index, const char*& description, [[maybe_unused]] float& duration)
    {
        static char str[64];
        description = str;
        AZ_Assert(index >= 0 && index < GetNumKeys(), "Key index %i is invalid", index);
        float& k = m_keys[index].m_fValue;
        sprintf_s(str, "%.2f", k);
    }

    void CGotoTrack::SetKeyAtTime(float time, IKey* key)
    {
        AZ_Assert(key != 0, "key is null");

        key->time = time;

        bool found = false;
        // Find key with given time.
        for (size_t i = 0; i < m_keys.size(); i++)
        {
            float keyt = m_keys[i].time;
            if (fabs(keyt - time) < MinTimePrecision)
            {
                key->flags = m_keys[i].flags; // Reserve the flag value.
                SetKey(static_cast<int>(i), key);
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
