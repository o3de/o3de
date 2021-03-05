/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "SQLiteBoundColumnSet.h"
#include "AzCore/std/containers/bitset.h"

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