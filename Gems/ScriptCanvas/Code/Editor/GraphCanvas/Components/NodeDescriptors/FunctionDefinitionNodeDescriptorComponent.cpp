/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FunctionDefinitionNodeDescriptorComponent.h"

#include <QInputDialog>
#include <QMainWindow>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/NodelingBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{

    FunctionDefinitionNodeDescriptorComponent::FunctionDefinitionNodeDescriptorComponent()
        : NodelingDescriptorComponent(NodeDescriptorType::FunctionDefinitionNode)
    {
    }

    void FunctionDefinitionNodeDescriptorComponent::Activate()
    {
        NodelingDescriptorComponent::Activate();

        GraphCanvas::VisualNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void FunctionDefinitionNodeDescriptorComponent::Deactivate()
    {
        NodelingDescriptorComponent::Deactivate();
        GraphCanvas::VisualNotificationBus::Handler::BusDisconnect();
    }

    void FunctionDefinitionNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<FunctionDefinitionNodeDescriptorComponent, NodelingDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    bool FunctionDefinitionNodeDescriptorComponent::RenameDialog(FunctionDefinitionNodeDescriptorComponent* descriptor)
    {
        if (!descriptor)
        {
            return false;
        }

        AZStd::string inBoxText{};
        AZStd::string defaultName{};

        GraphCanvas::NodeId nodeId = descriptor->GetEntityId();

        GraphCanvas::NodeTitleRequestBus::EventResult(defaultName, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);

        QMainWindow* editorWindow = nullptr;
        UIRequestBus::BroadcastResult(editorWindow, &UIRequests::GetMainWindow);

        bool accepted = false;
        QString name = QInputDialog::getText(qobject_cast<QWidget*>(editorWindow->parent()), "Name", inBoxText.c_str(), QLineEdit::Normal, defaultName.c_str(), &accepted);

        const AZStd::string newName = name.toUtf8().data();

        if (!name.isEmpty() && newName.compare(defaultName) != 0)
        {
            GraphCanvas::NodeTitleRequestBus::Event(nodeId, &GraphCanvas::NodeTitleRequests::SetTitle, newName);


            AZ::EntityId graphCanvasGraphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, descriptor->GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

            ScriptCanvas::GraphScopedNodeId scopedNodeId = ScriptCanvas::GraphScopedNodeId(scriptCanvasId, descriptor->FindScriptCanvasNodeId());

            ScriptCanvas::NodelingRequestBus::Event(scopedNodeId, &ScriptCanvas::NodelingRequests::SetDisplayName, newName);

            return true;
        }

        return false;
    }

    bool FunctionDefinitionNodeDescriptorComponent::OnMouseDoubleClick(const QGraphicsSceneMouseEvent*)
    {
        bool rename = RenameDialog(this);

        ScriptCanvas::ScriptCanvasId activeScriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeScriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, activeScriptCanvasId);

        return rename;
    }
}

