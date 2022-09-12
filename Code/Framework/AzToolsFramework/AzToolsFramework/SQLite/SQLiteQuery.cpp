/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SQLiteQuery.h"
#include <AzToolsFramework/SQLite/SQLiteQueryLogBus.h>

namespace AzToolsFramework
{
    namespace SQLite
    {
        namespace Internal
        {
            void LogQuery(const char* statement, const AZStd::string& params)
            {
                SQLiteQueryLogBus::Broadcast(&SQLiteQueryLogBus::Events::LogQuery, statement, params);
            }

            void LogResultId(AZ::s64 rowId)
            {
                SQLiteQueryLogBus::Broadcast(&SQLiteQueryLogBus::Events::LogResultId, rowId);
            }

            bool Bind(Statement* statement, int index, const AZ::Uuid& value)
            {
                return statement->BindValueUuid(index, value);
            }

            bool Bind(Statement* statement, int index, const AssetDatabase::PathOrUuid& value)
            {
                return statement->BindValuePathOrUuid(index, value);
            }

            bool Bind(Statement* statement, int index, double value)
            {
                return statement->BindValueDouble(index, value);
            }

            bool Bind(Statement* statement, int index, AZ::s32 value)
            {
                return statement->BindValueInt(index, value);
            }

            bool Bind(Statement* statement, int index, AZ::u32 value)
            {
                return statement->BindValueInt(index, value);
            }

            bool Bind(Statement* statement, int index, const char* value)
            {
                return statement->BindValueText(index, value);
            }

            bool Bind(Statement* statement, int index, AZ::s64 value)
            {
                return statement->BindValueInt64(index, value);
            }

            bool Bind(Statement* statement, int index, AZ::u64 value)
            {
                return statement->BindValueInt64(index, value);
            }

            bool Bind(Statement* statement, int index, const SqlBlob& value)
            {
                return statement->BindValueBlob(index, value.m_data, value.m_dataSize);
            }
        } // namespace Internal
    } // namespace SQLite

    namespace AssetDatabase
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
            // Store the stringified version of the UUID in m_path
            // We need to do this because when serializing the value to sqlite, the string needs to be stored somewhere for the entire
            // duration of the query If we were to return a temporary, it would go out of scope before the query is finalized We don't want
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
    } // namespace AssetDatabase
} // namespace AZFramework

std::ostream& std::operator<<(ostream& out, const AZ::Uuid& uuid)
{
    return out << uuid.ToString<AZStd::string>().c_str();
}

std::ostream& std::operator<<(ostream& out, const AzToolsFramework::SQLite::SqlBlob&)
{
    return out << "[Blob]";
}

std::ostream& std::operator<<(ostream& out, const AzToolsFramework::AssetDatabase::PathOrUuid& pathOrUuid)
{
    return out << pathOrUuid.ToString().c_str();
}
