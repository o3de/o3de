/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/MockBus.h>

namespace ScriptCanvasDeveloper
{
    namespace Nodes
    {
        using namespace ScriptCanvas;


        enum class SlotConfigState : AZ::u8
        {
            New,
            Unmodified
        };

        class Mock;
        struct SlotConfig
        {
            AZ_TYPE_INFO(SlotConfig, "{10B5DF2F-1105-4BDD-B8C0-24684AA4ECB3}");
            AZ_CLASS_ALLOCATOR(SlotConfig, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            AZ::Crc32 OnSlotNameChanged();
            AZ::Crc32 OnSlotToolTipChanged();
            void OnSlotDataTypeChanged(const Data::Type& dataType);
            AZStd::vector<AZStd::pair<Data::Type, AZStd::string>> GetCreatableTypes();

            SlotConfig();
            ~SlotConfig();

            AZStd::string m_name;
            AZStd::string m_toolTip;
            Data::Type m_type;
            SlotId m_slotId;
            Mock* m_owner{};
            SlotConfigState m_state = SlotConfigState::New;
        };

        struct MockNodeConfig
            : public AZ::ComponentConfig
        {
            AZ_RTTI(MockNodeConfig, "{6FA19B08-186D-4308-BAE3-530367D902E0}");
            AZ_CLASS_ALLOCATOR(MockNodeConfig, AZ::SystemAllocator, 0);

            static AZ::TypeId GetComponentTypeId()
            {
                return AZ::TypeId("{91B3D89A-4A4B-49FF-AD91-CAAC49E141A8}");
            }

            AZ::EntityId m_uiEntityId;
        };

        class Mock
            : public Node
            , protected ScriptCanvasEditor::EditorNodeNotificationBus::Handler
            , protected GraphCanvas::NodeNotificationBus::Handler
            , protected GraphCanvas::SceneMemberNotificationBus::Handler
            , protected MockDescriptorRequestBus::Handler
        {
        public:
            AZ_COMPONENT(Mock, MockNodeConfig::GetComponentTypeId(), Node);
            static void Reflect(AZ::ReflectContext* context);

            Mock();
            ~Mock();

            friend struct SlotConfig;

            AZ::Crc32 OnNodeTitleChanged();
            AZ::Crc32 OnNodeSubTitleChanged();
            AZ::Crc32 OnNodeColorPaletteChanged();
            AZ::Crc32 OnDataInSlotArrayChanged();
            AZ::Crc32 OnDataOutSlotArrayChanged();
            AZ::Crc32 OnExecutionInSlotArrayChanged();
            AZ::Crc32 OnExecutionOutSlotArrayChanged();

            //// GraphCanvas::NodeNotificationBus
            void OnSlotAddedToNode(const AZ::EntityId& slotId) override;
            void OnSlotRemovedFromNode(const AZ::EntityId& slotId) override;
            void OnAddedToScene(const AZ::EntityId& sceneId) override;
            ////

            // SceneMemberNotifications
            void OnSceneMemberDeserialized(const GraphCanvas::GraphId& grpahId, const GraphCanvas::GraphSerialization& serialization) override;
            ////

            // MockDescriptorRequestBus
            AZ::EntityId GetGraphCanvasNodeId() const override { return m_graphCanvasNodeId; }
            ////

        protected:
            void OnInit() override;

            void SetUiEntityId(AZ::EntityId uiEntityId);

            void Clear();
            
            AZStd::vector<AZStd::string> GetColorPaletteList() const;
            
            virtual void OnClear();
            virtual void OnNodeDisplayed(const GraphCanvas::NodeId& graphCanvasEntityId);

        private:
            //// ScriptCanvasEditor::EditorGraphNotificationBus
            void OnGraphCanvasNodeDisplayed(AZ::EntityId graphCanvasEntityId) override;
            ////

            AZStd::string m_nodeTitle;
            AZStd::string m_nodeSubTitle;
            AZStd::string m_nodeColorPaletteOverride;

            AZStd::list<SlotConfig> m_dataInConfigArray;
            AZStd::list<SlotConfig> m_dataOutConfigArray;
            AZStd::list<SlotConfig> m_executionInConfigArray;
            AZStd::list<SlotConfig> m_executionOutConfigArray;
            
            AZ::EntityId m_graphCanvasNodeId;
            AZStd::vector<AZ::EntityId> m_graphCanvasSlotIds;

            ///< Contains slotIds from deleted SlotConfigs that needs the slots removed from the Mock
            AZStd::vector<SlotId> m_pendingConfigRemovals; 
        };
    }
}
