/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include "SQLiteConnection.h"
#include <tuple>
#include <sstream>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class PathOrUuid;
    }

    namespace SQLite
    {
        struct SqlBlob;
    }
}

namespace std
{
    ostream& operator<<(ostream& out, const AZ::Uuid& uuid);
    ostream& operator<<(ostream& out, const AzToolsFramework::SQLite::SqlBlob&);
    ostream& operator<<(ostream& out, const AzToolsFramework::AssetDatabase::PathOrUuid& pathOrUuid);
}

namespace AzToolsFramework
{
    namespace SQLite
    {
        //! Represents a binary data blob.  Needed so that Bind can accept a pointer and size as a single type
        struct SqlBlob
        {
            void* m_data;
            int m_dataSize;
        };

        //! Represents a single query parameter, where T is the type of the field
        template<typename T>
        struct SqlParam
        {
            explicit SqlParam(const char* parameterName) : m_parameterName(parameterName) {}

            const char* m_parameterName;
        };

        //////////////////////////////////////////////////////////////////////////

        namespace Internal
        {
            void LogQuery(const char* statement, const AZStd::string& params);
            void LogResultId(AZ::s64 rowId);

            bool Bind(Statement* statement, int index, const AZ::Uuid& value);
            bool Bind(Statement* statement, int index, const AssetDatabase::PathOrUuid& value);
            bool Bind(Statement* statement, int index, double value);
            bool Bind(Statement* statement, int index, AZ::s32 value);
            bool Bind(Statement* statement, int index, AZ::u32 value);
            bool Bind(Statement* statement, int index, const char* value);
            bool Bind(Statement* statement, int index, AZ::s64 value);
            bool Bind(Statement* statement, int index, AZ::u64 value);
            bool Bind(Statement* statement, int index, const SqlBlob& value);
        }

        //! Helper object used to provide a query callback that needs to accept multiple arguments
        //! This is just a slightly easier-to-use/less verbose (at the callsite) alternative to AZStd::bind'ing the arguments to the callback
        template<typename T>
        struct SqlQueryResultRunner
        {
            using HandlerFunc = AZStd::function<bool(T&)>;

            SqlQueryResultRunner(bool bindSucceeded, const HandlerFunc& handler, const char* statementName, StatementAutoFinalizer autoFinalizer) :
                m_bindSucceeded(bindSucceeded),
                m_handler(handler),
                m_statementName(statementName),
                m_autoFinalizer(AZStd::move(autoFinalizer))
            {

            }

            template<typename TCallback, typename... TArgs>
            bool Query(const TCallback& callback, TArgs... args)
            {
                if (m_bindSucceeded)
                {
                    return callback(m_statementName, m_autoFinalizer.Get(), m_handler, args...);
                }

                return false;
            }

            const char* m_statementName;
            StatementAutoFinalizer m_autoFinalizer;
            const HandlerFunc& m_handler;
            bool m_bindSucceeded;
        };

        template<typename... T>
        class SqlQuery
        {
        public:
            SqlQuery(const char* statementName, const char* statement, const char* logName, SqlParam<T>&&... parameters) :
                m_statementName(statementName),
                m_statement(statement),
                m_logName(logName),
                m_parameters(parameters...)
            {

            }

            //! Bind both prepares and binds the args - call it on an empty autoFinalizer and it will prepare
            //! the query for you and return a ready-to-go autoFinalizer that has a valid statement ready to
            //! step()
            bool Bind(Connection& connection, StatementAutoFinalizer& autoFinalizer, const T&... args) const
            {
                // bind is meant to prepare the auto finalizer and prepare the connection, so assert if the
                // programmer has accidentally already bound it or prepared it first.
                AZ_Assert(!autoFinalizer.Get(), "Do not call Bind() on an autofinalizer that is already attached to a connection.");
                autoFinalizer = StatementAutoFinalizer(connection, m_statementName);
                Statement* statement = autoFinalizer.Get();

                if (statement == nullptr)
                {
                    AZ_Error(m_logName, false, "Could not find statement %s", m_statementName);
                    return false;
                }

                bool result = BindInternal<0>(statement, args...);

                AZStd::string debugParams;
                ArgsToString<0>(debugParams, args...);
                Internal::LogQuery(m_statement, debugParams);

                return result;
            }

            //! BindAndStep will execute the given statement and then clean up afterwards.  It is for
            //! calls that perform some operation on the database rather than a query that you need the result of.
            bool BindAndStep(Connection& connection, const T&... args) const
            {
                StatementAutoFinalizer autoFinalizer;

                if (!Bind(connection, autoFinalizer, args...))
                {
                    return false;
                }

                if (autoFinalizer.Get()->Step() == Statement::SqlError)
                {
                    AZStd::string argString;
                    ArgsToString<0>(argString, args...);
                    AZ_Warning(m_logName, false, "Failed to execute statement %s.\nQuery: %s\nParams: %s", m_statementName, m_statement, argString.c_str());
                    return false;
                }

                Internal::LogResultId(connection.GetLastRowID());

                return true;
            }

            //! Similar to Bind, this will prepare and bind the args.  Additionally, it will then call the callback with (statementName, statement, handler)
            //! The statement will be finalized automatically as part of this call
            template<typename THandler, typename TCallback>
            bool BindAndQuery(Connection& connection, THandler&& handler, const TCallback& callback, const T&... args) const
            {
                StatementAutoFinalizer autoFinal;

                if (!Bind(connection, autoFinal, args...))
                {
                    return false;
                }

                return callback(m_statementName, autoFinal.Get(), AZStd::forward<THandler>(handler));
            }

            //! Similar to Bind, this will prepare and bind the args.
            //! Returns a ResultRunner object that can be passed a callback and any number of arguments to forward to the callback *in addition* to the already supplied (statementName, statement, handler) arguments
            //! The statement will be finalized automatically when the ResultRunner goes out of scope
            template<typename TResultEntry>
            SqlQueryResultRunner<TResultEntry> BindAndThen(Connection& connection, const AZStd::function<bool(TResultEntry&)>& handler, const T&... args) const
            {
                StatementAutoFinalizer autoFinal;

                bool result = Bind(connection, autoFinal, args...);

                return SqlQueryResultRunner<TResultEntry>(result, handler, m_statementName, AZStd::move(autoFinal));
            }

        private:

            // Handles the 0-parameter case
            template<int TIndex>
            static bool BindInternal(Statement*)
            {
                return true;
            }

            template<int TIndex, typename T2>
            bool BindInternal(Statement* statement, const T2& value)  const
            {
                const SqlParam<T2>& sqlParam = std::get<TIndex>(m_parameters);

                int index = statement->GetNamedParamIdx(sqlParam.m_parameterName);

                if (!index)
                {
                    AZ_Error(m_logName, false, "Could not find the index for placeholder %s in statement %s ", sqlParam.m_parameterName, m_statementName);
                    return false;
                }

                Internal::Bind(statement, index, value);

                return true;
            }

            template<int TIndex, typename T2, typename... TArgs>
            bool BindInternal(Statement* statement, const T2& value, const TArgs&... args) const
            {
                return BindInternal<TIndex>(statement, value)
                    && BindInternal<TIndex + 1>(statement, args...);
            }

            template<int TIndex, typename TArg, typename...TArgs>
            void ArgsToString(AZStd::string& argsStringOutput, const TArg& arg, const TArgs&... args) const
            {
                const SqlParam<TArg>& sqlParam = std::get<TIndex>(m_parameters);

                std::ostringstream paramStringStream;

                if(!argsStringOutput.empty())
                {
                    paramStringStream << ", ";
                }

                paramStringStream << sqlParam.m_parameterName << " = `" << arg << "`";
                argsStringOutput.append(paramStringStream.str().c_str());

                ArgsToString<TIndex + 1>(argsStringOutput, args...);
            }

            // Handles 0-parameter case
            template<int TIndex>
            static void ArgsToString(AZStd::string&)
            {

            }

        public:
            const char* m_statementName;
            const char* m_statement;
            const char* m_logName;
            std::tuple<SqlParam<T>...> m_parameters;
        };

        template<typename TSqlQuery>
        void AddStatement(Connection* connection, const TSqlQuery& sqlQuery)
        {
            connection->AddStatement(sqlQuery.m_statementName, sqlQuery.m_statement);
        }

        template<typename... T>
        SqlQuery<T...> MakeSqlQuery(const char* statementName, const char* statement, const char* logName, SqlParam<T>&&... parameters)
        {
            return SqlQuery<T...>(statementName, statement, logName, AZStd::forward<SqlParam<T>>(parameters)...);
        }
    } // namespace SQLite
} // namespace AZFramework
