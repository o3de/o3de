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

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Canvas
#include <GraphCanvas/GraphCanvasBus.h>

// Graph Model
#include <GraphModel/Integration/GraphControllerManager.h>

namespace GraphModelIntegration
{
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
        auto it = m_graphControllers.find(sceneId);
        if (it != m_graphControllers.end())
        {
            return it->second->GetGraph();
        }

        return nullptr;
    }

    const GraphModelSerialization& GraphControllerManager::GetSerializedMappings()
    {
        return m_serialization;
    }

    void GraphControllerManager::SetSerializedMappings(const GraphModelSerialization& serialization)
    {
        m_serialization = serialization;
    }

    void GraphControllerManager::Activate()
    {
        GraphManagerRequestBus::Handler::BusConnect();
    }

    void GraphControllerManager::Deactivate()
    {
        GraphManagerRequestBus::Handler::BusDisconnect();
    }
} // namespace GraphModelIntegration
