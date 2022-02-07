/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include "CreateNodeMimeEvent.h"

#include "ScriptCanvas/Bus/RequestBus.h"

namespace ScriptCanvasEditor
{
    ////////////////////////
    // CreateNodeMimeEvent
    ////////////////////////

    void CreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeMimeEvent, GraphCanvas::CreateSplicingNodeMimeEvent>()
                ->Version(0)
                ;
        }
    }

    const ScriptCanvasEditor::NodeIdPair& CreateNodeMimeEvent::GetCreatedPair() const
    {
        return m_nodeIdPair;
    }
        
    bool CreateNodeMimeEvent::ExecuteEvent(const AZ::Vector2&, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        if (!scriptCanvasId.IsValid() || !graphCanvasGraphId.IsValid())
        {
            return false;
        }

        m_nodeIdPair = CreateNode(scriptCanvasId);

        if (m_nodeIdPair.m_graphCanvasId.IsValid() && m_nodeIdPair.m_scriptCanvasId.IsValid())
        {
            m_createdNodeId = m_nodeIdPair.m_graphCanvasId;

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, m_nodeIdPair.m_graphCanvasId, sceneDropPosition, false);
            GraphCanvas::SceneMemberUIRequestBus::Event(m_nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, m_nodeIdPair.m_graphCanvasId);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ_Assert(gridId.IsValid(), "Grid must be valid, graphCanvasGraphId is likely incorrect");

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
            return true;
        }
        else
        {
            if (m_nodeIdPair.m_graphCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_graphCanvasId);
            }

            if (m_nodeIdPair.m_scriptCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_scriptCanvasId);
            }
        }

        return false;
    }

    AZ::EntityId CreateNodeMimeEvent::CreateSplicingNode(const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair idPair = CreateNode(scriptCanvasId);

        if (idPair.m_graphCanvasId.IsValid() && idPair.m_scriptCanvasId.IsValid())
        {
            return idPair.m_graphCanvasId;
        }

        return AZ::EntityId();
    }

    ///////////////////////////////////
    // SpecializedCreateNodeMimeEvent
    ///////////////////////////////////

    void SpecializedCreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SpecializedCreateNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    /////////////////////////////
    // MultiCreateNodeMimeEvent
    /////////////////////////////

    void MultiCreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<MultiCreateNodeMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ;
        }
    }
}
