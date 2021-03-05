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

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    // Node that represents a Node Group that has been collapsed.
    //
    // Will handle generating remapping the connections that cross over the visual border.
    class CollapsedNodeGroupComponent
        : public GraphCanvasPropertyComponent
        , public SceneMemberNotificationBus::Handler
        , public NodeNotificationBus::Handler
        , public CommentNotificationBus::Handler
        , public GeometryNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public VisualNotificationBus::Handler
        , public CollapsedNodeGroupRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CollapsedNodeGroupComponent, "{FFA874A1-0D14-4BF9-932D-FE1A285506E6}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext*);
        
        static AZ::Entity* CreateCollapsedNodeGroupEntity(const CollapsedNodeGroupConfiguration& config);
        
        CollapsedNodeGroupComponent();
        CollapsedNodeGroupComponent(const CollapsedNodeGroupConfiguration& config);
        ~CollapsedNodeGroupComponent() override;

        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
        
        // NodeNotifications
        void OnAddedToScene(const GraphId& graphId) override;
        void OnRemovedFromScene(const GraphId& graphId) override;
        ////

        // GeometryNotifications
        void OnPositionChanged(const AZ::EntityId& targetEntity, const AZ::Vector2& position) override;
        ////

        // SceneNotifications
        void OnSceneMemberDragBegin() override;
        void OnSceneMemberDragComplete() override;
        ////

        // CommentNotifications
        void OnCommentChanged(const AZStd::string& comment) override;
        void OnBackgroundColorChanged(const AZ::Color& color) override;
        ////

        // VisualNotifications
        bool OnMouseDoubleClick(const QGraphicsSceneMouseEvent* mouseEvent) override;
        ////

        // CollapsedNodeGroupRequests
        void ExpandGroup() override;

        AZ::EntityId GetSourceGroup() const override;
        ////
        
    private:

        void CreateOccluder(const GraphId& graphId, bool isExpanding);

        void ConstructGrouping(const GraphId& graphId);
        void ReverseGrouping(const GraphId& graphId);

        void MoveGroupedElementsBy(const AZ::Vector2& offset);
        void MoveSubGraphBy(const GraphSubGraph& subGraph, const AZ::Vector2& offset);

        void OnAnimationFinished();        

        SlotId CreateSlotRedirection(const GraphId& graphId, const Endpoint& slotId);
        SlotId InitializeRedirectionSlot(const SlotRedirectionConfiguration& configuration);

        AZStd::vector< SlotRedirectionConfiguration > m_redirections;
        SubGraphParsingResult m_containedSubGraphs;
    
        AZ::EntityId m_nodeGroupId;

        bool         m_unhideOnAnimationComplete;
        bool         m_deleteObjects;

        bool         m_positionDirty;
        bool         m_ignorePositionChanges;
        AZ::Vector2  m_previousPosition;

        GraphicsEffectId m_effectId;
        QParallelAnimationGroup m_occluderAnimation;
        QPropertyAnimation* m_sizeAnimation;
        QPropertyAnimation* m_positionAnimation;
        QPropertyAnimation* m_opacityAnimation;
    };
}