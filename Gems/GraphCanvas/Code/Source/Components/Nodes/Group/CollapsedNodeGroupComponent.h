/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <GraphCanvas/Utils/StateControllers/StackStateController.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    class RedirectedSlotWatcher
        : public NodeNotificationBus::MultiHandler
    {
    public:
        RedirectedSlotWatcher() = default;
        ~RedirectedSlotWatcher();

        void ConfigureWatcher(const AZ::EntityId& collapsedGroupId);

        void RegisterEndpoint(const Endpoint& sourceEndpoint, const Endpoint& remappedEndpoint);

        // NodeNotificationBus
        void OnNodeAboutToBeDeleted() override;

        void OnSlotRemovedFromNode(const AZ::EntityId& slotId) override;
        ////

    private:

        void OnEndpointRemoved(Endpoint endpoint);

        AZStd::unordered_map<Endpoint, Endpoint> m_endpointMapping;

        AZ::EntityId m_collapsedGroupId;
    };

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
        , public GroupableSceneMemberNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
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

        // SystemTickBus
        void OnSystemTick() override;
        ////
        
        // NodeNotifications
        void OnAddedToScene(const GraphId& graphId) override;
        void OnRemovedFromScene(const GraphId& graphId) override;
        ////

        // GeometryNotifications
        void OnBoundsChanged() override;
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

        AZStd::vector< Endpoint > GetRedirectedEndpoints() const override;
        void ForceEndpointRedirection(const AZStd::vector< Endpoint >& endpoints) override;
        ////

        // GroupableSceneMemberNotifications
        void OnGroupChanged() override;
        ////

        // SceneMemberNotifications
        void OnSceneMemberHidden() override;
        void OnSceneMemberShown() override;
        ////
        
    private:

        void SetupGroupPosition(const GraphId& graphId);

        void CreateOccluder(const GraphId& graphId, const AZ::EntityId& initialElement);

        void TriggerExpandAnimation();
        void TriggerCollapseAnimation();

        void AnimateOccluder(bool isExpanding);

        void ConstructGrouping(const GraphId& graphId);
        void ReverseGrouping(const GraphId& graphId);

        void MoveGroupedElementsBy(const AZ::Vector2& offset);
        void MoveSubGraphBy(const GraphSubGraph& subGraph, const AZ::Vector2& offset);

        void OnAnimationFinished();

        SlotId CreateSlotRedirection(const GraphId& graphId, const Endpoint& slotId);
        SlotId InitializeRedirectionSlot(const SlotRedirectionConfiguration& configuration);

        void UpdateSystemTickBus();

        AZStd::unordered_set< Endpoint > m_forcedRedirections;
        AZStd::vector< SlotRedirectionConfiguration > m_redirections;
        SubGraphParsingResult m_containedSubGraphs;
    
        AZ::EntityId m_nodeGroupId;

        // Counter for how many frames to delay before beginning animations
        int m_animationDelayCounter;

        // Indicates if we are expanding in our animation or not.
        bool m_isExpandingOccluderAnimation;

        // Counter for how many frames to delay before destroying the occluder after the expansion animation.
        int m_occluderDestructionCounter;

        bool         m_unhideOnAnimationComplete;
        bool         m_deleteObjects;

        bool         m_positionDirty;

        RedirectedSlotWatcher m_redirectedSlotWatcher;

        StackStateController<bool> m_ignorePositionChanges;
        StateSetter<bool> m_memberHiddenStateSetter;
        StateSetter<bool> m_memberDraggedStateSetter;

        AZ::Vector2  m_previousPosition;

        GraphicsEffectId m_effectId;
        QParallelAnimationGroup m_occluderAnimation;
        QPropertyAnimation* m_sizeAnimation;
        QPropertyAnimation* m_positionAnimation;
        QPropertyAnimation* m_opacityAnimation;
    };
}
