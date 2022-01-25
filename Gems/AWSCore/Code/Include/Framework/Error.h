/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AWSCore
{

    class JsonReader;

    //! Encapsulates the data produced when a service API call fails.
    struct Error
    {
        AZ_TYPE_INFO(Error, "{4256E22F-441A-4CDA-92D9-B943C97E92ED}")
        AZ_CLASS_ALLOCATOR(Error, AZ::SystemAllocator, 0)

        /// Identifies the type of error. Intended for use by programs.
        ///
        ///    NetworkError - if there was a problem sending the request or 
        ///    receiving the response. e.g. due to timeouts or proxy issues.
        ///
        ///    ClientError - if the service determines that the client request
        ///    was invalid in some way.
        ///
        ///    ServiceError - if the service had trouble executing the request.
        ///
        ///    ContentError - if an error occurs when producing or consuming 
        ///    JSON format content.
        ///
        /// Services may return other error types.

        AZStd::string type;

        static const AZStd::string TYPE_NETWORK_ERROR;  // "NetworkError"
        static const AZStd::string TYPE_CLIENT_ERROR;   // "ClientError"
        static const AZStd::string TYPE_SERVICE_ERROR;  // "ServiceError"
        static const AZStd::string TYPE_CONTENT_ERROR;  // "ContentError"

        //! Describes the error. Intended for use by humans.
        AZStd::string message;

        bool OnJsonKey(const char* key, JsonReader& reader);

        static void Reflect(AZ::ReflectContext* reflection);
    };

} // namespace AWSCore


