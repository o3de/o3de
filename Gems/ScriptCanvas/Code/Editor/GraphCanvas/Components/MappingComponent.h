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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/NodeBus.h>

namespace ScriptCanvasEditor
{
    class SceneMemberMappingComponent
        : public AZ::Component
        , public SceneMemberMappingRequestBus::Handler
        , public SceneMemberMappingConfigurationRequestBus::Handler
        , public GraphCanvas::NodeNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SceneMemberMappingComponent, "{145667DE-EBD6-4EC5-B630-7C9B1A5ACFF0}");
        static void Reflect(AZ::ReflectContext* context);

        SceneMemberMappingComponent() = default;
        SceneMemberMappingComponent(const AZ::EntityId& sourceId);
        ~SceneMemberMappingComponent() = default;

        void Activate();
        void Deactivate();

        // SceneMemberMappingConfigurationRequestBus
        void ConfigureMapping(const AZ::EntityId& scriptCanvasMemberId) override;
        ////

        // SceneMemberMappingRequestBus
        AZ::EntityId GetGraphCanvasEntityId() const override;
        ////

        // NodeNotificationBus
        void OnBatchedConnectionManipulationBegin() override;
        void OnBatchedConnectionManipulationEnd() override;
        ////

    private:

        AZ::EntityId m_sourceId;
    };

    class SlotMappingComponent
        : public AZ::Component
        , public GraphCanvas::NodeNotificationBus::Handler
        , public SlotMappingRequestBus::MultiHandler
        , public ScriptCanvas::NodeNotificationsBus::Handler
        , public SceneMemberMappingConfigurationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SlotMappingComponent, "{94DBC04C-964D-46A0-AD66-6A779FE4DC61}");
        
        static void Reflect(AZ::ReflectContext* context);
        
        SlotMappingComponent() = default;
        SlotMappingComponent(const AZ::EntityId& sourceId);
        ~SlotMappingComponent() = default;
        
        void Activate();
        void Deactivate();
        
        // GraphCanvas::NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId&) override;
        
        void OnSlotAddedToNode(const AZ::EntityId& slotId) override;
        void OnSlotRemovedFromNode(const AZ::EntityId& slotId) override;
        ////
        
        // ScriptCanvasSlotRemappingBus
        AZ::EntityId MapToGraphCanvasId(const ScriptCanvas::SlotId& slotId) override;
        ////

        // ScriptCanvas::NodeNotificationBus::Handler
        void OnSlotRenamed(const ScriptCanvas::SlotId& slotId, AZStd::string_view newName) override;
        void OnSlotDisplayTypeChanged(const ScriptCanvas::SlotId& slotId, const ScriptCanvas::Data::Type& slotType) override;
        ////

        // SceneMemberMappingConfigurationRequestBus
        void ConfigureMapping(const AZ::EntityId& scriptCanvasMemberId) override;
        ////
        
    private:

        AZ::EntityId m_sourceId;
        AZStd::unordered_map< ScriptCanvas::SlotId, AZ::EntityId > m_slotMapping;

        AZStd::unordered_set< ScriptCanvas::SlotId > m_ignoreRenameSlots;
    };    
}
