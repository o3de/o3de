/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/GraphCanvas/DynamicSlotBus.h>

namespace ScriptCanvasEditor
{
    class DynamicSlotComponent
        : public AZ::Component
        , public GraphCanvas::SceneMemberNotificationBus::Handler
        , public ScriptCanvas::NodeNotificationsBus::Handler
        , public DynamicSlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(DynamicSlotComponent, "{977152B6-1A7D-49A4-8E70-644AFAD1586A}");
        static void Reflect(AZ::ReflectContext* serializeContext);
         
        DynamicSlotComponent();
        DynamicSlotComponent(GraphCanvas::SlotGroup slotGroup);
        ~DynamicSlotComponent() override = default;
        
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // GraphCanvas::SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////
        
        // ScriptCanvas::EditorNodeNotificationBus
        void OnSlotAdded(const ScriptCanvas::SlotId& slotId) override;
        void OnSlotRemoved(const ScriptCanvas::SlotId& slotId) override;
        ////

        // DynamicSlotRequestBus
        void OnUserDataChanged() override;

        void StartQueueSlotUpdates() override;
        void StopQueueSlotUpdates() override;
        ////

    protected:

        virtual void ConfigureGraphCanvasSlot(const ScriptCanvas::Slot* slot, const GraphCanvas::SlotId& graphCanvasSlotId);

        const AZ::EntityId& GetScriptCanvasNodeId() const { return m_scriptCanvasNodeId; }

    private:

        void HandleSlotAdded(const ScriptCanvas::Endpoint& endpoint);

        GraphCanvas::SlotGroup m_slotGroup;
        AZ::EntityId           m_scriptCanvasNodeId;

        bool m_queueUpdates;
        AZStd::unordered_set<ScriptCanvas::Endpoint> m_queuedEndpoints;
    };
}
