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

#pragma once

#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    namespace RPI
    {
        //! Provides a common way to report errors and warnings when processing Atom assets with JsonSerialization.
        class JsonReportingHelper
        {
        public:
            //! Attach this helper to the JsonSerializerSettings reporting callback
            void Attach(JsonSerializerSettings& settings);

            //! Attach this helper to the JsonDeserializerSettings reporting callback
            void Attach(JsonDeserializerSettings& settings);

            bool WarningsReported() const;

            bool ErrorsReported() const;

            AZStd::string GetErrorMessage() const;

        private:
            AZ::JsonSerializationResult::ResultCode Reporting(AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path);

            bool m_warningsReported = false;
            bool m_errorsReported = false;
            AZStd::string m_firstErrorMessage;
        };

    } // namespace RPI
} // namespace AZ
