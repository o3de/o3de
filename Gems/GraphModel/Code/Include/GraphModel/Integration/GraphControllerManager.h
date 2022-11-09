/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/base.h>

// Graph Model
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Integration/GraphController.h>

namespace GraphModelIntegration
{
    //! This is the main class for managing the Graph Controllers for Graph Canvas scenes
    class GraphControllerManager
        : private GraphManagerRequestBus::Handler
    {
    public:
        AZ_RTTI(GraphControllerManager, "{DA358B3E-46EF-411B-B84B-0397F5CD3539}");

        GraphControllerManager();
        ~GraphControllerManager();

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphModelIntegration::GraphManagerRequestBus overrides
        AZ::Entity* CreateScene(GraphModel::GraphPtr graph, const GraphCanvas::EditorId editorId) override;
        void RemoveScene(const GraphCanvas::GraphId& sceneId) override;
        void CreateGraphController(const GraphCanvas::GraphId& sceneId, GraphModel::GraphPtr graph) override;
        void DeleteGraphController(const GraphCanvas::GraphId& sceneId) override;
        GraphModel::GraphPtr GetGraph(const GraphCanvas::GraphId& sceneId) override;
        const GraphModelSerialization& GetSerializedMappings() override;
        void SetSerializedMappings(const GraphModelSerialization& serialization) override;
        ////////////////////////////////////////////////////////////////////////////////////

    private:
        AZ_DISABLE_COPY_MOVE(GraphControllerManager);

        AZStd::unordered_map<GraphCanvas::GraphId, AZStd::shared_ptr<GraphController>> m_graphControllers;

        GraphModelSerialization m_serialization;
    };
}
