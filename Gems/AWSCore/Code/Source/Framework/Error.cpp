/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Framework/Error.h>
#include <Framework/JsonObjectHandler.h>

namespace AWSCore
{
    
    const AZStd::string Error::TYPE_NETWORK_ERROR{"NetworkError"};
    const AZStd::string Error::TYPE_CLIENT_ERROR{"ClientError"};
    const AZStd::string Error::TYPE_SERVICE_ERROR{"ServiceError"};
    const AZStd::string Error::TYPE_CONTENT_ERROR{"ContentError"};

    void Error::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<Error>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<Error>()
                ->Property("type", BehaviorValueProperty(&Error::type))
                ->Property("message", BehaviorValueProperty(&Error::message))
                ;
        }
    }

    bool Error::OnJsonKey(const char* key, JsonReader& reader)
    {
        if (strcmp(key, "errorType") == 0)
        {
            return reader.Accept(type);
        }
        if (strcmp(key, "errorMessage") == 0)
        {
            return reader.Accept(message);
        }
        // Fail if there is any unexpected content. For errors, This will cause all the content to be used 
        // in the error message.
        return false; 
    }

} // namespace AWSCore
