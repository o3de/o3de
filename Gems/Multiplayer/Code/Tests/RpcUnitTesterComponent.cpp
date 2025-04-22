/*
* Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#include <Tests/RpcUnitTesterComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace MultiplayerTest
{
    void RpcUnitTesterComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RpcUnitTesterComponent, RpcUnitTesterComponentBase>()
                ->Version(1);
        }
        RpcUnitTesterComponentBase::Reflect(context);
    }

    void RpcUnitTesterComponent::OnInit()
    {
    }

    void RpcUnitTesterComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void RpcUnitTesterComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    RpcUnitTesterComponentController* RpcUnitTesterComponent::GetTestController()
    {
        return static_cast<RpcUnitTesterComponentController*>(GetController());
    }

    RpcUnitTesterComponentController::RpcUnitTesterComponentController(RpcUnitTesterComponent& parent)
        : RpcUnitTesterComponentControllerBase(parent)
    {
    }

    void RpcUnitTesterComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void RpcUnitTesterComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }
}
