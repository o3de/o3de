/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Canvas
#include <GraphCanvas/GraphCanvasBus.h>

// Graph Model
#include <GraphModel/Integration/GraphControllerManager.h>

namespace GraphModelIntegration
{
    GraphControllerManager::GraphControllerManager()
    {
        GraphManagerRequestBus::Handler::BusConnect();
    }

    GraphControllerManager::~GraphControllerManager()
    {
        GraphManagerRequestBus::Handler::BusDisconnect();
    }

    AZ::Entity* GraphControllerManager::CreateScene(GraphModel::GraphPtr graph, const GraphCanvas::EditorId editorId)
    {
        AZ::Entity* scene = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(scene, &GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);

        // Set the EditorId for the new scene
        const AZ::EntityId& sceneId = scene->GetId();
        GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::SetEditorId, editorId);

        // Create a graph controller for the new scene
        CreateGraphController(sceneId, graph);

        return scene;
    }

    void GraphControllerManager::RemoveScene(const GraphCanvas::GraphId& sceneId)
    {
        DeleteGraphController(sceneId);
    }

    void GraphControllerManager::CreateGraphController(const GraphCanvas::GraphId& sceneId, GraphModel::GraphPtr graph)
    {
        m_graphControllers[sceneId] = AZStd::make_shared<GraphController>(graph, sceneId);
    }

    void GraphControllerManager::DeleteGraphController(const GraphCanvas::GraphId& sceneId)
    {
        m_graphControllers.erase(sceneId);
    }

    GraphModel::GraphPtr GraphControllerManager::GetGraph(const GraphCanvas::GraphId& sceneId)
    {
        const auto it = m_graphControllers.find(sceneId);
        return it != m_graphControllers.end() ? it->second->GetGraph() : nullptr;
    }

    const GraphModelSerialization& GraphControllerManager::GetSerializedMappings()
    {
        return m_serialization;
    }

    void GraphControllerManager::SetSerializedMappings(const GraphModelSerialization& serialization)
    {
        m_serialization = serialization;
    }
} // namespace GraphModelIntegration
