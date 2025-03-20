/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringTrack.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>

namespace Maestro
{
    AnimValueType CStringTrack::GetValueType() const
    {
        return AnimValueType::String;
    }

    int CStringTrack::CreateKey(float time)
    {
        if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_WarningOnce("StringTrack", false, "CreateKey(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        const auto existingKeyIndex = FindKey(time);
        if (existingKeyIndex >= 0)
        {
            AZ_Error("StringTrack", false, "CreateKey(%f): Key (%d) at this time already exists in track (%s).",
                time, existingKeyIndex, (GetNode() ? GetNode()->GetName() : ""))
            return -1;
        }

        int numKeys = GetNumKeys();
        SortKeys();
        IStringKey key;
        GetValue(time, key.m_strValue); // copy last key value or default value
        key.time = time;
        SetNumKeys(numKeys + 1);
        SetKey(numKeys, &key);

        SortKeys();

        return FindKey(time);
    }

    void CStringTrack::GetValue(float time, AZStd::string& value) const
    {
        value = m_defaultValue;

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
        else
        {
            IStringKey key(value);
            SetKeyAtTime(time, &key);
        }
    }

    void CStringTrack::SetKeyAtTime(float time, IKey* key)
    {
        if (!key)
        {
            AZ_Assert(false, "Invalid key pointer.");
            return;
        }

        if (((m_timeRange.end - m_timeRange.start) > AZ::Constants::Tolerance) && (time < m_timeRange.start || time > m_timeRange.end))
        {
            AZ_WarningOnce("StringTrack", false, "SetKeyAtTime(%f): Time is out of range (%f .. %f) in track (%s), clamped.",
                time, m_timeRange.start, m_timeRange.end, (GetNode() ? GetNode()->GetName() : ""));
            AZStd::clamp(time, m_timeRange.start, m_timeRange.end);
        }

        key->time = time;

        // Find key with given time.
        bool found = false;
        for (int i = 0; i < GetNumKeys(); i++)
        {
            float keyTime = m_keys[i].time;
            if (AZStd::abs(keyTime - time) < GetMinKeyTimeDelta()) // Found a close key? -> replace it
            {
                key->flags = m_keys[i].flags; // Reserve the flag value.
                SetKey(i, key);
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

        SortKeys();
    }

    void CStringTrack::GetKeyInfo([[maybe_unused]] int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        static auto str = m_keys[keyIndex].m_strValue;
        constexpr const size_t allowdLength = 30;
        if (str.length() > allowdLength)
        {
            // Get a sub-string for description.
            // First check if its a path, then shorten description to filename.
            AZStd::string fileName;
            if (AzFramework::StringFunc::AssetDatabasePath::Split(str.c_str(), nullptr, nullptr, nullptr, &fileName, nullptr) && fileName.length() > 1)
            {
                str = fileName;
            }
            else // General string, take the tail sub-string.
            {
                AzFramework::StringFunc::TrimWhiteSpace(str, true, true);
                if (str.length() > allowdLength)
                {
                    AzFramework::StringFunc::LChop(str, str.length() - allowdLength);
                }
            }
        }
        description = str.c_str();
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
