/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/BreakpointMenuActions/BreakpointContextMenuAction.h>

namespace GraphCanvas
{
    class AddBreakpointMenuAction : public BreakpointContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddBreakpointMenuAction, AZ::SystemAllocator);

        AddBreakpointMenuAction(QObject* parent);
        virtual ~AddBreakpointMenuAction() = default;

        using BreakpointContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;
    };
} // namespace GraphCanvas
