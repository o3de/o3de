/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SQLiteBoundColumnSet.h"
#include "AzCore/std/containers/bitset.h"
#include <AzToolsFramework/AssetDatabase/PathOrUuid.h>

namespace AzToolsFramework
{
    namespace SQLite
    {
        namespace Internal
        {
            template<>
            const void* GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnBlob(index);
            }

            template<>
            AZ::u64 GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnInt64(index);
            }

            template<>
            AZ::s64 GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnInt64(index);
            }

            template<>
            AZ::u32 GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnInt(index);
            }

            template<>
            AZ::s32 GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnInt(index);
            }

            template<>
            double GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnDouble(index);
            }

            template<>
            AZ::Uuid GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnUuid(index);
            }

            template<>
            AssetDatabase::PathOrUuid GetColumnValue(SQLite::Statement* statement, int index)
            {
                return AssetDatabase::PathOrUuid::Create(statement->GetColumnText(index));
            }

            template<>
            AZStd::string GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnText(index);
            }

            template<>
            AZStd::bitset<64> GetColumnValue(SQLite::Statement* statement, int index)
            {
                return statement->GetColumnInt64(index);
            }
        } // namespace Internal
    } // namespace SQLite
} // namespace AZFramework
