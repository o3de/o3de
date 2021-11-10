/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace Aws
{
    namespace GameLift
    {
        class GameLiftClient;
    }
} // namespace Aws

namespace AWSGameLift
{
    //! IAWSGameLiftInternalRequests
    //! GameLift Gem internal interface which is used to fetch gem global GameLift client
    class IAWSGameLiftInternalRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftInternalRequests, "{DC0CC1C4-21EE-4A41-B428-D12D697F88A2}");

        IAWSGameLiftInternalRequests() = default;
        virtual ~IAWSGameLiftInternalRequests() = default;

        //! GetGameLiftClient
        //! Get GameLift client to interact with Amazon GameLift service
        //! @return Shared pointer to the GameLift client
        virtual AZStd::shared_ptr<Aws::GameLift::GameLiftClient> GetGameLiftClient() const = 0;

        //! SetGameLiftClient
        //! Set GameLift client to provided GameLift client
        //! @param gameliftClient Input new GameLift client
        virtual void SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient) = 0;
    };
} // namespace AWSGameLift
