/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

namespace GraphCanvas
{
    class Connection;

    class SlotComponent
        : public AZ::Component
        , public SlotRequestBus::Handler
        , public SceneMemberRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SlotComponent, "{EACFC8FB-C75B-4ABA-988D-89C964B9A4E4}");
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static void Reflect(AZ::ReflectContext* context);
        static AZ::Entity* CreateCoreSlotEntity();

        SlotComponent();
        explicit SlotComponent(const SlotType& slotType);
        SlotComponent(const SlotType& slotType, const SlotConfiguration& slotConfiguration);
        ~SlotComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(kSlotServiceProviderId);
            provided.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& oldSceneId) override;
        void SignalMemberSetupComplete() override;

        AZ::EntityId GetScene() const override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // SlotRequestBus
        const AZ::EntityId& GetNode() const override;
        void SetNode(const AZ::EntityId&) override;

        Endpoint GetEndpoint() const override;

        const AZStd::string GetName() const  override { return m_slotConfiguration.m_name.GetDisplayString(); }
        void SetName(const AZStd::string& name) override;

        TranslationKeyedString GetTranslationKeyedName() const override { return m_slotConfiguration.m_name; }
        void SetTranslationKeyedName(const TranslationKeyedString&) override;

        const AZStd::string GetTooltip() const override { return m_slotConfiguration.m_tooltip.GetDisplayString(); }
        void SetTooltip(const AZStd::string& tooltip)  override;

        TranslationKeyedString GetTranslationKeyedTooltip() const override { return m_slotConfiguration.m_tooltip; }
        void SetTranslationKeyedTooltip(const TranslationKeyedString&) override;

        void DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        void AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        ConnectionType GetConnectionType() const override { return m_slotConfiguration.m_connectionType; }
        SlotGroup GetSlotGroup() const override { return m_slotConfiguration.m_slotGroup; }
        SlotType GetSlotType() const override { return m_slotType; }

        void SetDisplayOrdering(int ordering) override;
        int GetDisplayOrdering() const override;

        bool IsConnectedTo(const Endpoint& endpoint) const override;

        void FindConnectionsForEndpoints(const AZStd::unordered_set<GraphCanvas::Endpoint>& searchEndpoints, AZStd::unordered_set<ConnectionId>& connections) override;

        bool CanDisplayConnectionTo(const Endpoint& endpoint) const override;
        bool CanCreateConnectionTo(const Endpoint& endpoint) const override;

        AZ::EntityId CreateConnectionWithEndpoint(const Endpoint& endpoint) override;

        AZ::EntityId DisplayConnection() override;
        AZ::EntityId DisplayConnectionWithEndpoint(const Endpoint& endpoint) override;

        AZStd::any* GetUserData() override;

        bool HasConnections() const override;

        AZ::EntityId GetLastConnection() const override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;

        void SetConnectionDisplayState(RootGraphicsItemDisplayState displayState) override;
        void ReleaseConnectionDisplayState() override;
        void ClearConnections() override;

        const SlotConfiguration& GetSlotConfiguration() const override;
        SlotConfiguration* CloneSlotConfiguration() const override;

        void RemapSlotForModel(const Endpoint& endpoint) override;

        bool HasModelRemapping() const override;

        AZStd::vector< Endpoint > GetRemappedModelEndpoints() const override;

        int GetLayoutPriority() const override;
        void SetLayoutPriority(int priority) override;

        void Show() { m_visible = true; }
        void Hide() { m_visible = false; }
        ////

        bool IsVisible() const { return m_visible; }

    protected:

        void PopulateSlotConfiguration(SlotConfiguration& slotConfiguration) const;

        AZ::EntityId CreateConnectionHelper(const Endpoint& otherEndpoint, bool createConnection);

        SlotComponent(const SlotComponent&) = delete;
        const SlotComponent& operator=(const SlotComponent&) = delete;
        virtual AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection);

        void FinalizeDisplay();
        virtual void OnFinalizeDisplay();

        AZStd::vector< Endpoint > m_modelRedirections;

        //! The Node this Slot belongs to.
        AZ::EntityId m_nodeId;

        SlotType          m_slotType;
        SlotConfiguration m_slotConfiguration;

        // The actual display ordering this slot is in
        int               m_displayOrdering;

        // The priority with which to layout this slot
        int               m_layoutPriority;

        //! Keeps track of connections to this slot
        AZStd::vector<AZ::EntityId> m_connections;

        StateSetter<RootGraphicsItemDisplayState> m_connectionDisplayStateStateSetter;

        //! Stores custom user data for this slot
        AZStd::any m_userData;

        bool m_visible;
    };
}
