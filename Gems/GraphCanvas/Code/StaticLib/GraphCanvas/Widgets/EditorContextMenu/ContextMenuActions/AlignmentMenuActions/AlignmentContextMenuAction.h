/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    class AlignmentContextMenuAction
        : public ContextMenuAction
    {
    protected:
        AlignmentContextMenuAction(AZStd::string_view actionName, QObject* parent)
            : ContextMenuAction(actionName, parent)
        {
        }

        using ContextMenuAction::RefreshAction;
        void RefreshAction() override
        {
            const AZ::EntityId& graphId = GetGraphId();
            const AZ::EntityId& targetId = GetTargetId();

            bool canAlignSelection = false;
            SceneRequestBus::EventResult(canAlignSelection, graphId, &SceneRequests::HasMultipleSelection);

            if (!canAlignSelection && targetId.IsValid())
            {
                canAlignSelection = GraphUtils::IsNodeGroup(targetId);
            }

            setEnabled(canAlignSelection);
        }
        
    public:
    
        static ActionGroupId GetAlignmentContextMenuActionGroupId()
        {
            return AZ_CRC("AlignmentActionGroup", 0xd31bdeab);
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetAlignmentContextMenuActionGroupId();
        }
    };
}
