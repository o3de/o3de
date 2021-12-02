/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuAction.h>

namespace GraphCanvas
{
    class ManageUnusedSlotsMenuAction
        : public NodeContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ManageUnusedSlotsMenuAction, AZ::SystemAllocator, 0);
        
        ManageUnusedSlotsMenuAction(QObject* parent, bool hideSlots);
        virtual ~ManageUnusedSlotsMenuAction() = default;

        using NodeContextMenuAction::RefreshAction;
        void RefreshAction(const GraphId& grpahId, const AZ::EntityId& targetId) override;

        using NodeContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2&) override;
        
    private:
        bool         m_hideSlots = true;
        AZ::EntityId m_targetId;
    };    
}
