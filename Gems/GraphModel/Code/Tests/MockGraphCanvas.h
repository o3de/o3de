/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ ...
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Canvas ...
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphCanvasBus.h>

namespace MockGraphCanvasServices
{
    //! This mocks the GraphCanvas::SlotComponent component.
    //! This component is added to a SlotEntity that is created when a Slot is added to a Node.
    class MockSlotComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockSlotComponent, "{030690A4-6D16-4770-89B8-20A2EDF48D87}");
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateCoreSlotEntity();

        MockSlotComponent() = default;
        explicit MockSlotComponent(const GraphCanvas::SlotType& slotType);

        MockSlotComponent(const GraphCanvas::SlotType& slotType, const GraphCanvas::SlotConfiguration& configuration);
        ~MockSlotComponent() override = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

    protected:
        GraphCanvas::SlotType m_slotType;
        GraphCanvas::SlotConfiguration m_slotConfiguration;
    };
    
    //! This mocks the GraphCanvas::DataSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when a DataSlot is added to a Node.
    //! Implements the GraphCanvas::DataSlotRequestBus for tests which involve data slots.
    class MockDataSlotComponent
        : public MockSlotComponent
        , public GraphCanvas::DataSlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockDataSlotComponent, "{0E2E8F38-3B7B-427D-ABD6-38C68FDEFE88}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateDataSlot(const GraphCanvas::DataSlotConfiguration& dataSlotConfiguration);

        MockDataSlotComponent();
        MockDataSlotComponent(const GraphCanvas::DataSlotConfiguration& dataSlotConfiguration);
        ~MockDataSlotComponent() = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::DataSlotRequestBus overrides ...
        bool ConvertToReference() override;
        bool CanConvertToReference() const override;
        bool ConvertToValue() override;
        bool CanConvertToValue() const override;
        bool IsUserSlot() const override;

        GraphCanvas::DataSlotType GetDataSlotType() const override;
        GraphCanvas::DataValueType GetDataValueType() const override;
        AZ::Uuid GetDataTypeId() const override;
        void SetDataTypeId(AZ::Uuid typeId) override;
        const GraphCanvas::Styling::StyleHelper* GetDataColorPalette() const override;
        size_t GetContainedTypesCount() const override;
        AZ::Uuid GetContainedTypeId(size_t index) const override;
        const GraphCanvas::Styling::StyleHelper* GetContainedTypeColorPalette(size_t index) const override;
        void SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds, GraphCanvas::DataValueType valueType) override;

    private:
        MockDataSlotComponent(const MockDataSlotComponent&) = delete;
        MockDataSlotComponent& operator=(const MockDataSlotComponent&) = delete;

        GraphCanvas::DataSlotConfiguration m_dataSlotConfiguration;
    };

    //! This mocks the GraphCanvas::ExecutionSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when an ExecutionSlot is added to a Node.
    class MockExecutionSlotComponent
        : public MockSlotComponent
    {
    public:
        AZ_COMPONENT(MockExecutionSlotComponent, "{3E12451C-65EB-45A6-AC98-437F06021359}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreateExecutionSlot(const AZ::EntityId& nodeId, const GraphCanvas::SlotConfiguration& slotConfiguration);

        MockExecutionSlotComponent();
        explicit MockExecutionSlotComponent(const GraphCanvas::SlotConfiguration& slotConfiguration);
        ~MockExecutionSlotComponent() = default;

    protected:
        MockExecutionSlotComponent(const MockExecutionSlotComponent&) = delete;
        MockExecutionSlotComponent& operator=(const MockExecutionSlotComponent&) = delete;

        GraphCanvas::SlotConfiguration m_executionSlotConfiguration;
    };

    //! This mocks the GraphCanvas::ExtenderSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when an ExtenderSlot is added to a Node.
    //! Implements the GraphCanvas::ExtenderSlotRequestBus for tests which involve extender slots.
    class MockExtenderSlotComponent
        : public MockSlotComponent
        , public GraphCanvas::ExtenderSlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockExtenderSlotComponent, "{0CAE942E-5E4E-42EC-8F63-809A4DE317C0}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreateExtenderSlot(const AZ::EntityId& nodeId, const GraphCanvas::ExtenderSlotConfiguration& slotConfiguration);

        MockExtenderSlotComponent();
        explicit MockExtenderSlotComponent(const GraphCanvas::ExtenderSlotConfiguration& slotConfiguration);
        ~MockExtenderSlotComponent() = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;
        ////

        // ExtenderSlotComponent overrides ...
        void TriggerExtension() override;
        GraphCanvas::Endpoint ExtendForConnectionProposal(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& endpoint) override;

    protected:
        MockExtenderSlotComponent(const MockExtenderSlotComponent&) = delete;
        MockExtenderSlotComponent& operator=(const MockExtenderSlotComponent&) = delete;

        GraphCanvas::ExtenderSlotConfiguration m_extenderSlotConfiguration;
    };

    //! This mocks the GraphCanvas::NodeComponent component.
    //! This component is added to a Node entity when a Node is added to the graph.
    //! Implements the GraphCanvas::NodeRequestBus for tests that invole nodes.
    class MockNodeComponent
        : public AZ::Component
        , public GraphCanvas::NodeRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockNodeComponent, "{886E7216-FD58-442B-AF1E-1AC7174885F8}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateCoreNodeEntity(const GraphCanvas::NodeConfiguration& config = GraphCanvas::NodeConfiguration());

        MockNodeComponent() = default;
        MockNodeComponent(const GraphCanvas::NodeConfiguration& config);
        ~MockNodeComponent() override = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::NodeRequestBus overrides ...
        void SetTooltip(const AZStd::string& tooltip) override;
        void SetTranslationKeyedTooltip(const GraphCanvas::TranslationKeyedString& tooltip) override;
        const AZStd::string GetTooltip() const override;
        void SetShowInOutliner(bool showInOutliner) override;
        bool ShowInOutliner() const override;
        void AddSlot(const AZ::EntityId& slotId) override;
        void RemoveSlot(const AZ::EntityId& slotId) override;
        AZStd::vector<AZ::EntityId> GetSlotIds() const override;
        AZStd::vector<GraphCanvas::SlotId> GetVisibleSlotIds() const override;
        AZStd::vector<GraphCanvas::SlotId> FindVisibleSlotIdsByType(const GraphCanvas::ConnectionType& connectionType, const GraphCanvas::SlotType& slotType) const override;
        bool HasConnections() const override;
        AZStd::any* GetUserData() override;
        bool IsWrapped() const override;
        void SetWrappingNode(const AZ::EntityId& wrappingNode) override;
        AZ::EntityId GetWrappingNode() const override;
        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;
        GraphCanvas::RootGraphicsItemEnabledState UpdateEnabledState() override;
        bool IsHidingUnusedSlots() const override;
        void ShowAllSlots() override;
        void HideUnusedSlots() override;
        bool HasHideableSlots() const override;
        void SignalConnectionMoveBegin(const GraphCanvas::ConnectionId& connectionId) override;
        void SignalNodeAboutToBeDeleted() override;

    protected:
        /// This node's slots
        AZStd::vector<AZ::EntityId> m_slotIds;

        /// Serialized configuration settings
        GraphCanvas::NodeConfiguration m_configuration;

        /// Stores custom user data for this node
        AZStd::any m_userData;
    };

    //! This mocks the GraphCanvas::GraphCanvasSystemComponent component.
    //! This component is created and added to the system entity created in our GraphModelIntegrationTest::TestEnvironment
    //! because this component implements the GraphCanvas::GraphCanvasRequestBus that is
    //! the entry point bus for performing basic GraphCanvas operations such as creating
    //! a new scene, creating nodes, creating slots, etc...
    class MockGraphCanvasSystemComponent
        : public AZ::Component
        , private GraphCanvas::GraphCanvasRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockGraphCanvasSystemComponent, "{03D5474F-5FF3-4D7B-B578-2C3EC132E921}");
        static void Reflect(AZ::ReflectContext* context);

        MockGraphCanvasSystemComponent() = default;
        ~MockGraphCanvasSystemComponent() override = default;

    private:
        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::GraphCanvasRequestBus overrides ...
        AZ::Entity* CreateBookmarkAnchor() const override;
        AZ::Entity* CreateScene() const override;
        AZ::Entity* CreateCoreNode() const override;
        AZ::Entity* CreateGeneralNode(const char* nodeType) const override;
        AZ::Entity* CreateCommentNode() const override;
        AZ::Entity* CreateWrapperNode(const char* nodeType) const override;
        AZ::Entity* CreateNodeGroup() const override;
        AZ::Entity* CreateCollapsedNodeGroup(const GraphCanvas::CollapsedNodeGroupConfiguration& groupedNodeConfiguration) const override;
        AZ::Entity* CreateSlot(const AZ::EntityId& nodeId, const GraphCanvas::SlotConfiguration& slotConfiguration) const override;
        GraphCanvas::NodePropertyDisplay* CreateBooleanNodePropertyDisplay(GraphCanvas::BooleanDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateNumericNodePropertyDisplay(GraphCanvas::NumericDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateComboBoxNodePropertyDisplay(GraphCanvas::ComboBoxDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateEntityIdNodePropertyDisplay(GraphCanvas::EntityIdDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateReadOnlyNodePropertyDisplay(GraphCanvas::ReadOnlyDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateStringNodePropertyDisplay(GraphCanvas::StringDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateVectorNodePropertyDisplay(GraphCanvas::VectorDataInterface* dataInterface) const override;
        GraphCanvas::NodePropertyDisplay* CreateAssetIdNodePropertyDisplay(GraphCanvas::AssetIdDataInterface* dataInterface) const override;
        AZ::Entity* CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const GraphCanvas::SlotConfiguration& slotConfiguration) const override;
    };
}
