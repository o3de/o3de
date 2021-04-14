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

#include <Source/Components/LocalPredictionPlayerInputComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Multiplayer
{
    void LocalPredictionPlayerInputComponent::LocalPredictionPlayerInputComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LocalPredictionPlayerInputComponent, LocalPredictionPlayerInputComponentBase>()
                ->Version(1);
        }
        LocalPredictionPlayerInputComponentBase::Reflect(context);
    }

    void LocalPredictionPlayerInputComponentController::HandleSendClientInput
    (
        [[maybe_unused]] const Multiplayer::NetworkInputVector& inputArray,
        [[maybe_unused]] const uint32_t& stateHash,
        [[maybe_unused]] const AzNetworking::PacketEncodingBuffer& clientState
    )
    {
        ;
    }

    void LocalPredictionPlayerInputComponentController::HandleSendMigrateClientInput
    (
        [[maybe_unused]] const Multiplayer::MigrateNetworkInputVector& inputArray
    )
    {
        ;
    }

    void LocalPredictionPlayerInputComponentController::HandleSendClientInputCorrection
    (
        [[maybe_unused]] const Multiplayer::NetworkInputId& inputId,
        [[maybe_unused]] const AzNetworking::PacketEncodingBuffer& correction
    )
    {
        ;
    }
}
