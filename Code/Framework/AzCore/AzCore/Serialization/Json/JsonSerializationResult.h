/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    namespace JsonSerializationResult
    {
        // Note the order of the various components is important since the Json Serializer will return
        // the most severe code, so these need to be ordered from least to most severe and from least to
        // most detailed.

        //! The task that was being performed when the issue we detected.
        enum class Tasks : uint8_t
        {
            RetrieveInfo = 1,   //!< Task to retrieve information from a location such as the Serialize Context.
            CreateDefault,      //!< Task to create a default instance.
            Convert,            //!< Task to convert a value from one type to another.
            Clear,              //!< Task to clear a field/value.
            ReadField,          //!< Task to read a field from JSON to a value.
            WriteValue,         //!< Task to write a value to a JSON field.
            Merge,              //!< Task to merge two JSON values/documents together.
            CreatePatch,        //!< Task to create a patch to transform one value/document to another.
            Import              //!< Task to import a JSON document.
        };

        //! Describes how the task was processed.
        enum class Processing : uint8_t
        {
            Completed = 1,      //!< Processing fully completed.
            Altered,            //!< Processing encountered an issue but was able to continue on an alternative path. The input will not match the expected output.
            PartialAlter,       //!< Processing of object/array encountered one or more fields/values with altered processing.
            Halted,             //!< Processing couldn't fully complete. This indicates a severe problem.
        };

        //! The result of the task.
        enum class Outcomes : uint16_t
        {
            Success = 1,        //!< Task completed successfully.
            Skipped,            //!< Task skipped a field or value.
            PartialSkip,        //!< Task skipped one or more fields when processing an object/array.
            DefaultsUsed,       //!< Task completed, only defaults were used.
            PartialDefaults,    //!< Task completed, but some defaults were used.
            Unavailable,        //!< The task tried to use space that's not available.
            Unsupported,        //!< An unsupported action was requested.
            TypeMismatch,       //!< Source and target are unrelated so the operation is not possible.
            TestFailed,         //!< A test against a value failed.
            Missing,            //!< A required field or value was missing.
            Invalid,            //!< A field or element has an invalid value.
            Unknown,            //!< The task encountered unknown or missing information.
            Catastrophic        //!< A general failure occurred.
        };

        class Result;

        //! A lightweight result returned by all major functions in the Json Serialization.
        //! This is essentially a 32 bit integer but with additional functionality to be
        //! more descriptive than a standard integer result code.
        union ResultCode
        {
        public:
            explicit ResultCode(Tasks task);
            ResultCode(Tasks task, Outcomes result);
            
            bool HasDoneWork() const;

            ResultCode& Combine(ResultCode other);
            ResultCode& Combine(const Result& other);
            static ResultCode Combine(ResultCode lhs, ResultCode rhs);

            Tasks GetTask() const;
            Processing GetProcessing() const;
            Outcomes GetOutcome() const;

            //! Append the provided string with a description of the result code.
            //! @output The string to append to.
            //! @path Path to the value/field/type that resulted in this result code.
            void AppendToString(AZ::OSString& output, AZStd::string_view path) const;

            //! Append the provided string with a description of the result code.
            //! @output The string to append to.
            //! @path Path to the value/field/type that resulted in this result code.
            void AppendToString(AZStd::string& output, AZStd::string_view path) const;

            //! Creates a string with the description of the results code.
            //! @path Path to the value/field/type that resulted in this result code.
            AZStd::string ToString(AZStd::string_view path) const;

            //! Creates a string with the description of the results code.
            //! @path Path to the value/field/type that resulted in this result code.
            AZ::OSString ToOSString(AZStd::string_view path) const;

        private:
            explicit ResultCode(uint32_t code);

            struct
            {
                Tasks m_task;
                Processing m_processing;
                Outcomes m_outcome;
            } m_options;
            uint32_t m_code;
        };

        using JsonIssueCallback = AZStd::function<ResultCode(AZStd::string_view message, ResultCode result, AZStd::string_view path)>;

        //! Result is a wrapper around ResultCode to encourage serializer to report results
        //! at least once per load/store call. The most common use is to call context.Report
        //! which returns a Result (which can also be used as a ResultCode).
        class Result
        {
        public:
            Result(const JsonIssueCallback& callback, AZStd::string_view message, ResultCode result, AZStd::string_view path);
            Result(const JsonIssueCallback& callback, AZStd::string_view message, Tasks task, Outcomes outcome, AZStd::string_view path);

            operator ResultCode() const;
            ResultCode GetResultCode() const;

        private:
            ResultCode m_result;
        };
    } // namespace JsonSerializationResult
} // namespace AZ
