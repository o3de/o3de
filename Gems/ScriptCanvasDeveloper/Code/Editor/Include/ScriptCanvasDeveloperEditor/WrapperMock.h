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

#include <GraphCanvas/Components/SceneBus.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvasDeveloperEditor/Mock.h>
#include <ScriptCanvasDeveloperEditor/MockBus.h>

namespace ScriptCanvasDeveloper
{
    namespace Nodes
    {
        using namespace ScriptCanvas;

        class WrapperMock
            : public Mock
            , public ScriptCanvasEditor::ScriptCanvasWrapperNodeDescriptorRequestBus::Handler
            , public MockDescriptorNotificationBus::MultiHandler
            , public GraphCanvas::SceneNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(WrapperMock, "{14C6CE68-906A-4D2F-89BA-153774CE8015}", Mock);
            static void Reflect(AZ::ReflectContext* context);

            WrapperMock();
            ~WrapperMock() = default;

            // ScriptCanvasWrapperNodeDescriptorRequestBus
            void OnWrapperAction(const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;
            ////

            // MockDescriptorNotificationBus
            void OnGraphCanvasNodeSetup(const GraphCanvas::NodeId& graphCanvasNodeId) override;
            ////

            // GraphCanvas::NodeNotificationBus
            void OnNodeRemoved(const GraphCanvas::NodeId& nodeId) override;
            ////

        protected:

            void OnActionNameChanged();

            void OnClear();
            void OnNodeDisplayed(const GraphCanvas::NodeId& graphCanvasNodeId) override;

        private:

            AZStd::string                 m_actionName;
            AZStd::vector< AZ::EntityId > m_wrappedNodeIds;
            AZStd::unordered_map< GraphCanvas::NodeId, AZ::EntityId > m_graphCanvasMapping;
        };
    }
}