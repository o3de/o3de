/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/tuple.h>
#include "SQLiteConnection.h"

namespace AzToolsFramework
{
    namespace SQLite
    {
        namespace Internal
        {
            // Collection of helper functions to allow calling the appropriate GetColumn function based on the return type ...
            template<typename T> T GetColumnValue(SQLite::Statement* statement, int index);

            template<typename T, typename AZStd::enable_if_t<AZStd::is_enum<T>::value>* = nullptr>
            T GetColumnEnum(SQLite::Statement* statement, int index)
            {
                // In the case of enums, try to cast to the underlying type and see if we have a GetColumnValue implementation for that type
                using EnumType = typename AZStd::underlying_type<T>::type;

                return static_cast<T>(GetColumnValue<EnumType>(statement, index));
            }

            template<typename T, typename AZStd::enable_if_t<!AZStd::is_enum<T>::value>* = nullptr>
            T GetColumnEnum(SQLite::Statement* statement, int index)
            {
                // Non-enum type with no implementation
                static_assert(!AZStd::is_same<T, T>::value, "Type not implemented");
            }

            template<typename T>
            T GetColumnValue(SQLite::Statement* statement, int index)
            {
                // Generic case, if this is an enum type we'll try to cast to the underlying type, otherwise fail
                return GetColumnEnum<T>(statement, index);
            }

            template<>
            AZStd::string GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZ::Uuid GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AzToolsFramework::AssetDatabase::PathOrUuid GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            double GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZ::s32 GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZ::u32 GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZ::s64 GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZ::u64 GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            const void* GetColumnValue(SQLite::Statement* statement, int index);

            template<>
            AZStd::bitset<64> GetColumnValue(SQLite::Statement* statement, int index);
        }

        // Represents a single column in an SQLite result which is bound to a variable reference where the result will be stored
        template<typename T>
        struct BoundColumn
        {
            // columnName = exact match of the name of the column as returned by the SQLite query
            // boundMember = reference to the variable where the result will be stored when Fetch is called.  This class does not store the data itself
            BoundColumn(const char* columnName, T& boundMember) : m_name(columnName), m_boundMember(boundMember) {}

            // Retrieves the value stored in the current row for this column and saves the value to the bound member reference
            bool Fetch(SQLite::Statement* statement)
            {
                if (m_index == -1)
                {
                    m_index = statement->FindColumn(m_name);

                    if (m_index == -1)
                    {
                        AZ_Error("AzToolsFramework::SQLite", false, "Failed to find column %s for query", m_name);
                        return false;
                    }
                }

                m_boundMember = Internal::GetColumnValue<T>(statement, m_index);
                return true;
            }

            const char* m_name{};
            int m_index{ -1 };
            T& m_boundMember;
        };

        // Helper to allow type-deduction when creating a BoundColumn
        template<typename T>
        BoundColumn<T> MakeColumn(const char* columnName, T& boundMember)
        {
            return BoundColumn<T>(columnName, boundMember);
        }

        // Represents a collection of BoundColumns
        // Allows for easily fetching the values of every contained column in a single Fetch call
        template<typename... T>
        struct BoundColumnSet
        {
            static constexpr size_t ColumnCount = sizeof...(T);
            using ColumnTupleType = AZStd::tuple<BoundColumn<T>...>;

            static_assert(ColumnCount > 0, "BoundColumnSet must contain at least one column");

            BoundColumnSet(ColumnTupleType columns) : m_columns(columns) {}

            // Retrieves the value stored in the current row for every column in the set and saves the values to the bound member references
            bool Fetch(SQLite::Statement* statement)
            {
                return FetchImpl(statement, AZStd::make_index_sequence<ColumnCount>());
            }

        private:
            template<size_t... TIndices>
            bool FetchImpl(SQLite::Statement* statement, AZStd::index_sequence<TIndices...>)
            {
                bool results[] = {AZStd::get<TIndices>(m_columns).Fetch(statement)...};

                // Check if any of the Fetch results were false
                for(bool result : results)
                {
                    if(!result)
                    {
                        return false;
                    }
                }

                return true;
            }

        public:
            ColumnTupleType m_columns;
        };

        // Helper to allow type-deduction when creating a BoundColumnSet
        template<typename... T>
        BoundColumnSet<T...> MakeColumns(BoundColumn<T>... cols)
        {
            return BoundColumnSet<T...>(AZStd::make_tuple(AZStd::forward<BoundColumn<T>>(cols)...));
        }

        // Combines multiple BoundColumnSets into a single BoundColumnSet object
        template<typename... T>
        auto CombineColumns(T... columnSets)
        {
            return MakeColumnsFromTuple(AZStd::tuple_cat(columnSets.m_columns...));
        }

        // Helper to allow type-deduction when creating a BoundColumnSet from a tuple
        template<typename... T>
        auto MakeColumnsFromTuple(AZStd::tuple<BoundColumn<T>...> tuple)
        {
            return BoundColumnSet<T...>(AZStd::move(tuple));
        }
    } // namespace SQLite
} // namespace AZFramework
