/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringTrack.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>

namespace Maestro
{
    AnimValueType CStringTrack::GetValueType()
    {
        return AnimValueType::String;
    }

    int CStringTrack::CreateKey(float time)
    {
        CheckValid(); // sort keys
        const int nkey = GetNumKeys();
        IStringKey key;
        GetValue(time, key.m_strValue); // copy last key value or default value
        key.time = time;
        SetNumKeys(nkey + 1);
        SetKey(nkey, &key);
        Invalidate();

        return nkey;
    }

    void CStringTrack::GetValue(float time, AZStd::string& value)
    {
        value = m_defaultValue;

        CheckValid();

        int nkeys = static_cast<int>(m_keys.size());
        if (nkeys < 1)
        {
            return;
        }

        for (int idx = 0; idx < nkeys; ++idx)
        {
            if (time >= m_keys[idx].time)
            {
                value = m_keys[idx].m_strValue;
            }
        }
    }

    void CStringTrack::SetValue(float time, const AZStd::string& value, bool bDefault)
    {
        if (bDefault)
        {
            SetDefaultValue(value);
        }
        else if (time > m_timeRange.end)
        {
            return;
        }
        else
        {
            IStringKey key(value);
            SetKeyAtTime(time, &key);
        }
        Invalidate();
    }

    void CStringTrack::SetKeyAtTime(float time, IKey* key)
    {
        if (!key)
        {
            AZ_Assert(false, "SetKeyAtTime given a null pointer to key.");
            return;
        }

        AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        key->time = time;

        // Find key with given time.
        CheckValid();
        constexpr float MinTimePrecision = 0.01f;
        bool found = false;
        for (size_t i = 0; i < m_keys.size(); i++)
        {
            float keyt = m_keys[i].time;
            if (AZStd::abs(keyt - time) < MinTimePrecision) // Found a close key?
            {
                key->flags = m_keys[i].flags; // Reserve the flag value.
                SetKey(static_cast<int>(i), key);
                found = true;
                break;
            }
        }
        if (!found)
        {
            // Key with this time not found -> create a new one.
            int keyIndex = CreateKey(time);
            key->flags = m_keys[keyIndex].flags; // Reserve the flag value.
            SetKey(keyIndex, key);
        }
    }

    void CStringTrack::GetKeyInfo([[maybe_unused]] int index, const char*& description, float& duration)
    {
        description = 0;
        duration = 0;
    }

    static bool StringTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<IStringKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<IStringKey>, IAnimTrack>()
                ->Version(3, &StringTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<IStringKey>::m_flags)
                ->Field("Range", &TAnimTrack<IStringKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<IStringKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<IStringKey>::m_keys)
                ->Field("Id", &TAnimTrack<IStringKey>::m_id);
        }
    }

    void CStringTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<IStringKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CStringTrack, TAnimTrack<IStringKey>>()->Version(1)->Field(
                "DefaultValue", &CStringTrack::m_defaultValue);
            ;
        }
    }
} // namespace Maestro
