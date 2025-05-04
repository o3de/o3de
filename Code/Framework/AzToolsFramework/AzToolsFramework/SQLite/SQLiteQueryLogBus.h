/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    namespace SQLite
    {
        class SQLiteQueryLogEvents
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            virtual void LogQuery(const char* statement, const AZStd::string& params) = 0;
            virtual void LogResultId(AZ::s64 rowId) = 0;
        };

        typedef AZ::EBus<SQLiteQueryLogEvents> SQLiteQueryLogBus;

    } // namespace SQLite
} // namespace AzToolsFramework

AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::SQLite::SQLiteQueryLogEvents);
