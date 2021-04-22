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

#include <Source/AutoGen/LocalPredictionPlayerInputComponent.AutoComponent.h>

namespace Multiplayer
{
    class LocalPredictionPlayerInputComponent
        : public LocalPredictionPlayerInputComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::LocalPredictionPlayerInputComponent, s_localPredictionPlayerInputComponentConcreteUuid, Multiplayer::LocalPredictionPlayerInputComponentBase);

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context);

        void OnInit() override {}
        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}


    };

    class LocalPredictionPlayerInputComponentController
        : public LocalPredictionPlayerInputComponentControllerBase
    {
    public:
        LocalPredictionPlayerInputComponentController(LocalPredictionPlayerInputComponent& parent) : LocalPredictionPlayerInputComponentControllerBase(parent) {}

        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override {}

        void HandleSendClientInput(const Multiplayer::NetworkInputVector& inputArray, const uint32_t& stateHash, const AzNetworking::PacketEncodingBuffer& clientState) override;
        void HandleSendMigrateClientInput(const Multiplayer::MigrateNetworkInputVector& inputArray) override;
        void HandleSendClientInputCorrection(const Multiplayer::ClientInputId& inputId, const AzNetworking::PacketEncodingBuffer& correction) override;
    };
}
