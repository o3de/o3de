/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    namespace RPI
    {
        //! Provides a common way to report errors and warnings when processing Atom assets with JsonSerialization.
        class ATOM_RPI_EDIT_API JsonReportingHelper
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
