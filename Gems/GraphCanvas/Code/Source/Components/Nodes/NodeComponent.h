/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/string/string_view.h>


#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/ComponentSaveDataInterface.h>

namespace GraphCanvas
{
    class NodeComponent
        : public GraphCanvasPropertyComponent
        , public NodeRequestBus::Handler
        , public SceneMemberRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public AZ::EntityBus::Handler
        , public SlotNotificationBus::MultiHandler
        , public ComponentSaveDataInterface<NodeSaveData>
        , public ConnectionNotificationBus::Handler
        , public GroupableSceneMemberRequestBus::Handler
    {
        friend class NodeSerializer;
    public:
        AZ_COMPONENT(NodeComponent, "{7385AAC3-18F0-4BCE-BD9B-C17798C899EC}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext*);

        static AZ::Entity* CreateCoreNodeEntity(const NodeConfiguration& config = NodeConfiguration());

        NodeComponent();
        NodeComponent(const NodeConfiguration& config);
        ~NodeComponent() override;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            provided.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_GeometryService", 0x80981600));
            required.push_back(StyledGraphicItemServiceCrc);
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SlotNotificationBus
        void OnConnectedTo(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void OnDisconnectedFrom(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        ////

        // AZ::EntityBus
        void OnEntityExists(const AZ::EntityId&) override;
        void OnEntityActivated(const AZ::EntityId&) override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& oldSceneId) override;
        void SignalMemberSetupComplete() override;

        AZ::EntityId GetScene() const override;
        ////        

        // SceneMemberNotificationsBus
        void OnSceneReady() override;
        ////

        // SceneNotificationsBus
        void OnStylesChanged() override;
        void OnGraphLoadComplete() override;

        void OnPasteEnd() override;
        ////

        // NodeRequestBus
        void SetTooltip(const AZStd::string& tooltip) override;
        void SetTranslationKeyedTooltip(const TranslationKeyedString& tooltip) override;
        const AZStd::string GetTooltip() const override { return m_configuration.GetTooltip(); }

        void SetShowInOutliner(bool showInOutliner) override { m_configuration.SetShowInOutliner(showInOutliner); }
        bool ShowInOutliner() const override { return m_configuration.GetShowInOutliner(); }

        void AddSlot(const AZ::EntityId& slotId) override;
        void RemoveSlot(const AZ::EntityId& slotId) override;

        AZStd::vector<AZ::EntityId> GetSlotIds() const override;
        AZStd::vector<SlotId> GetVisibleSlotIds() const override;
        AZStd::vector<SlotId> FindVisibleSlotIdsByType(const ConnectionType& connectionType, const SlotType& slotType) const override;

        bool HasConnections() const override;

        AZStd::any* GetUserData() override;

        bool IsWrapped() const override;
        void SetWrappingNode(const AZ::EntityId& wrappingNode) override;
        AZ::EntityId GetWrappingNode() const override;

        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;

        void SignalConnectionMoveBegin(const ConnectionId& connectionId) override;

        RootGraphicsItemEnabledState UpdateEnabledState() override;

        bool HasHideableSlots() const override;
        bool IsHidingUnusedSlots() const override;
        void ShowAllSlots() override;
        void HideUnusedSlots() override;

        void SignalNodeAboutToBeDeleted() override;
        ////

        // ConnectionNotificationBus
        void OnMoveFinalized(bool isValidConnection) override;
        ////

        // GroupableSceneMemberRequests
        bool IsGrouped() const override;
        const AZ::EntityId& GetGroupId() const override;

        void RegisterToGroup(const AZ::EntityId& groupId) override;
        void UnregisterFromGroup(const AZ::EntityId& groupId) override;
        void RemoveFromGroup() override;
        ////

    protected:

        void HideUnusedSlotsImpl();
        void UpdateDisabledStateVisuals();

        //! The ID of the scene this node belongs to.
        AZ::EntityId m_sceneId;

        //! This node's slots
        AZStd::vector<AZ::Entity*> m_slots;

        //! Serialized configuration settings
        NodeConfiguration m_configuration;

        AZ::EntityId m_wrappingNode;
        AZ::EntityId m_owningGroupId;

        //! Stores custom user data for this node
        AZStd::any m_userData;

        bool m_updateSlotState = false;
    };
}
