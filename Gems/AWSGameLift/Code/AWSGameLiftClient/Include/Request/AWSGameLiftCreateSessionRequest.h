/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Session/SessionRequests.h>

namespace AWSGameLift
{
    //! AWSGameLiftCreateSessionRequest
    //! GameLift create session on fleet request which corresponds to Amazon GameLift
    //! CreateGameSessionRequest
    struct AWSGameLiftCreateSessionRequest
        : public AzFramework::CreateSessionRequest
    {
    public:
        AZ_RTTI(AWSGameLiftCreateSessionRequest, "{69612D5D-F899-4DEB-AD63-4C497ABC5C0D}", AzFramework::CreateSessionRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftCreateSessionRequest() = default;
        virtual ~AWSGameLiftCreateSessionRequest() = default;

        // A unique identifier for the alias associated with the fleet to create a game session in.
        AZStd::string m_aliasId;

        // A unique identifier for the fleet to create a game session in.
        AZStd::string m_fleetId;

        // Custom string that uniquely identifies the new game session request.
        // This is useful for ensuring that game session requests with the same idempotency token are processed only once. 
        AZStd::string m_idempotencyToken;
    };
} // namespace AWSGameLift
