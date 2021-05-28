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
        : LayerControllerComponent("ConnectionLayer", ConnectionOffset)
        , m_sourceLayerController(nullptr)
        , m_targetLayerController(nullptr)
    {
    }

    void ConnectionLayerControllerComponent::Activate()
    {
        LayerControllerComponent::Activate();

        ConnectionNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ConnectionLayerControllerComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        LayerControllerComponent::OnSceneSet(sceneId);
        UpdateEndpoints();
    }

    void ConnectionLayerControllerComponent::OnMoveBegin()
    {
        SetBaseModifier("editing");
        UpdateEndpoints();
    }

    void ConnectionLayerControllerComponent::OnMoveFinalized(bool isValidConnection)
    {
        if (isValidConnection)
        {
            SetBaseModifier("");
            UpdateEndpoints();
        }
    }

    void ConnectionLayerControllerComponent::OnSourceSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&)
    {
        UpdateEndpoints();
    }

    void ConnectionLayerControllerComponent::OnTargetSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&)
    {
        UpdateEndpoints();
    }

    void ConnectionLayerControllerComponent::OnOffsetsChanged(int selectionOffset, int groupOffset)
    {
        selectionOffset = 0;
        groupOffset = 0;

        if (m_sourceLayerController)
        {
            groupOffset = m_sourceLayerController->GetGroupLayerOffset();
            selectionOffset = m_sourceLayerController->GetGroupLayerOffset();
        }

        if (m_targetLayerController)
        {
            // Need to use min offset here, otherwise connections can be above nodes. Which causes some wierd UX issues
            groupOffset = AZStd::min(m_targetLayerController->GetGroupLayerOffset(), groupOffset);
            selectionOffset = AZStd::min(m_targetLayerController->GetGroupLayerOffset(), selectionOffset);
        }

        SetGroupLayerOffset(groupOffset);
        SetSelectionLayerOffset(selectionOffset);
    }

    void ConnectionLayerControllerComponent::UpdateEndpoints()
    {
        LayerControllerNotificationBus::MultiHandler::BusDisconnect();

        Endpoint sourceEndpoint;
        ConnectionRequestBus::EventResult(sourceEndpoint, GetEntityId(), &ConnectionRequests::GetSourceEndpoint);

        m_sourceLayerController = LayerControllerRequestBus::FindFirstHandler(sourceEndpoint.GetNodeId());

        if (m_sourceLayerController)
        {
            LayerControllerNotificationBus::MultiHandler::BusConnect(sourceEndpoint.GetNodeId());
        }

        Endpoint targetEndpoint;
        ConnectionRequestBus::EventResult(targetEndpoint, GetEntityId(), &ConnectionRequests::GetTargetEndpoint);

        m_targetLayerController = LayerControllerRequestBus::FindFirstHandler(targetEndpoint.GetNodeId());

        if (m_targetLayerController)
        {
            LayerControllerNotificationBus::MultiHandler::BusConnect(targetEndpoint.GetNodeId());
        }

        OnOffsetsChanged(0, 0);
    }
}
