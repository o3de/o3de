/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/GraphCanvas/Components/DynamicSlotComponent.h>

namespace ScriptCanvasEditor
{
    // Should be used when the order of slots might be re-arranged at edit time.
    // Will handle synchronizing and updating the various slots.
    //
    // Will do extra work, so should not be used all the time
    class DynamicOrderingDynamicSlotComponent
        : public DynamicSlotComponent
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(DynamicOrderingDynamicSlotComponent, "{90205620-E77B-4F09-8891-A0B1AE5E83EA}", DynamicSlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        DynamicOrderingDynamicSlotComponent();
        DynamicOrderingDynamicSlotComponent(GraphCanvas::SlotGroup slotGroup);
        ~DynamicOrderingDynamicSlotComponent() override = default;

        // AZ::SystemTickBus
        void OnSystemTick() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;        
        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& serializationTarget) override;
        ////

        // NodeNotifications
        void OnSlotsReordered() override;
        ////

        // DynamicRequestBus
        void StopQueueSlotUpdates() override;
        ////

    protected:

        void ConfigureGraphCanvasSlot(const ScriptCanvas::Slot* slot, const GraphCanvas::SlotId& graphCanvasSlotId) override;

    private:

        bool m_deserialized;
    };
}
