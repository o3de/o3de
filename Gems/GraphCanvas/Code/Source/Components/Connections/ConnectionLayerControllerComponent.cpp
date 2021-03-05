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
#include "precompiled.h"

#include <Source/Components/Connections/ConnectionLayerControllerComponent.h>

namespace GraphCanvas
{
    ///////////////////////////////////////
    // ConnectionLayerControllerComponent
    ///////////////////////////////////////

    void ConnectionLayerControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ConnectionLayerControllerComponent, LayerControllerComponent>()
                ->Version(0)
                ;
        }
    }

    ConnectionLayerControllerComponent::ConnectionLayerControllerComponent()
        : LayerControllerComponent("ConnectionLayer")
    {
    }

    void ConnectionLayerControllerComponent::Activate()
    {
        LayerControllerComponent::Activate();

        ConnectionNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ConnectionLayerControllerComponent::OnMoveBegin()
    {
        SetBaseModifier("editing");
    }

    void ConnectionLayerControllerComponent::OnMoveFinalized(bool isValidConnection)
    {
        if (isValidConnection)
        {
            SetBaseModifier("");
        }
    }
}