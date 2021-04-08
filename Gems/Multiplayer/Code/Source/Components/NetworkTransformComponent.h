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

#include <Source/AutoGen/NetworkTransformComponent.AutoComponent.h>

namespace Multiplayer
{
    class NetworkTransformComponent
        : public NetworkTransformComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkTransformComponent, s_networkTransformComponentConcreteUuid, Multiplayer::NetworkTransformComponentBase);

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context);

        void OnInit() override {}
        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
    };

    class NetworkTransformComponentController
        : public NetworkTransformComponentControllerBase
    {
    public:
        NetworkTransformComponentController(NetworkTransformComponent& parent) : NetworkTransformComponentControllerBase(parent) {}

        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
    };
}
