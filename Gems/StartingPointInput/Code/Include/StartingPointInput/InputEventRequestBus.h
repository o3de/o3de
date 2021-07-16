/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/User/LocalUserId.h>

namespace StartingPointInput
{
    class InputConfigurationComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual ~InputConfigurationComponentRequests() = default;

        //! Set the local user id that the input component should process input from
        virtual void SetLocalUserId(AzFramework::LocalUserId localUserId) = 0;
    };
    using InputConfigurationComponentRequestBus = AZ::EBus<InputConfigurationComponentRequests>;
} // namespace StartingPointInput
