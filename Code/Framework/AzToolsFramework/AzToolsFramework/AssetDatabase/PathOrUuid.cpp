/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetDatabase/PathOrUuid.h>

namespace AzToolsFramework::AssetDatabase
{
    PathOrUuid::PathOrUuid(AZStd::string path)
        : m_path(AZStd::move(path))
    {
    }

    PathOrUuid::PathOrUuid(const char* path)
        : m_path(path)
    {
    }

    PathOrUuid::PathOrUuid(AZ::Uuid uuid)
        : m_uuid(AZStd::move(uuid))
    {
        // Store the stringified version of the UUID in m_path.
        // We need to do this because when serializing the value to sqlite, the string needs to be stored somewhere for the entire
        // duration of the query. If we were to return a temporary, it would go out of scope before the query is finalized. We don't want
        // to store with brackets or dashes because this allows sqlite compatibility: hex(UUID_BLOB) returns a string of hex without any
        // dashes or brackets, allowing for queries like select * from sources join sourcedependencies on hex(sourceguid) =
        // dependsonsource
        m_path = m_uuid.ToFixedString(false, false);
    }

    PathOrUuid PathOrUuid::Create(AZStd::string val)
    {
        AZ::Uuid uuid = AZ::Uuid::CreateStringPermissive(val.c_str());

        if (uuid.IsNull())
        {
            return PathOrUuid(AZStd::move(val));
        }

        return PathOrUuid(uuid);
    }

    bool PathOrUuid::IsUuid() const
    {
        return !m_uuid.IsNull();
    }

    const AZ::Uuid& PathOrUuid::GetUuid() const
    {
        return m_uuid;
    }

    const AZStd::string& PathOrUuid::GetPath() const
    {
        return m_path;
    }

    const AZStd::string& PathOrUuid::ToString() const
    {
        // This looks wrong but actually we store m_uuid.ToString() in m_path.  See above for reasoning
        return m_path;
    }

    bool PathOrUuid::operator==(const PathOrUuid& rhs) const
    {
        if (IsUuid())
        {
            return rhs.IsUuid() && m_uuid == rhs.m_uuid;
        }

        return !rhs.IsUuid() && m_path == rhs.m_path;
    }

    PathOrUuid::operator bool() const
    {
        if (!m_uuid.IsNull())
        {
            return true;
        }

        return !m_path.empty();
    }
}

AZStd::size_t AZStd::hash<AzToolsFramework::AssetDatabase::PathOrUuid>::operator()(
    const AzToolsFramework::AssetDatabase::PathOrUuid& obj) const
{
    AZStd::size_t seed = 0;

    if (obj.IsUuid())
    {
        AZStd::hash_combine(seed, obj.GetUuid());
    }
    else
    {
        AZStd::hash_combine(seed, obj.GetPath());
    }

    return seed;
}
