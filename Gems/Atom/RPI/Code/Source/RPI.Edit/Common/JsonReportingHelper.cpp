/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>

namespace AZ
{
    namespace RPI
    {

        void JsonReportingHelper::Attach(JsonSerializerSettings& settings)
        {
            settings.m_reporting = [this](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
            {
                return Reporting(message, resultCode, path);
            };
        }

        void JsonReportingHelper::Attach(JsonDeserializerSettings& settings)
        {
            settings.m_reporting = [this](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
            {
                return Reporting(message, resultCode, path);
            };
        }

        bool JsonReportingHelper::WarningsReported() const
        {
            return m_warningsReported;
        }

        bool JsonReportingHelper::ErrorsReported() const
        {
            return m_errorsReported;
        }

        AZStd::string JsonReportingHelper::GetErrorMessage() const
        {
            return m_firstErrorMessage;
        }

        AZ::JsonSerializationResult::ResultCode JsonReportingHelper::Reporting(AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, [[maybe_unused]] AZStd::string_view path)
        {
            if (result.GetOutcome() == JsonSerializationResult::Outcomes::Skipped)
            {
                m_warningsReported = true;
                AZ_Warning("JSON", false, "Skipped unrecognized field '%.*s'", AZ_STRING_ARG(path));
            }
            else if (result.GetProcessing() != JsonSerializationResult::Processing::Completed ||
                result.GetOutcome() >= JsonSerializationResult::Outcomes::Unavailable)
            {
                if (result.GetOutcome() >= JsonSerializationResult::Outcomes::Catastrophic)
                {
                    m_errorsReported = true;
                    AZ_Error("JSON", false, "'%.*s': %.*s - %s", AZ_STRING_ARG(path), AZ_STRING_ARG(message), result.ToString("").c_str());

                    if (m_firstErrorMessage.empty())
                    {
                        m_firstErrorMessage = message;
                    }
                }
                else
                {
                    m_warningsReported = true;
                    AZ_Warning("JSON", false, "'%.*s': %.*s - %s", AZ_STRING_ARG(path), AZ_STRING_ARG(message), result.ToString("").c_str());
                }
            }

            return result;
        }

    } // namespace RPI
} // namespace AZ
