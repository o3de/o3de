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
