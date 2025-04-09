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
    class CommentContextMenuAction
        : public ContextMenuAction
    {
    protected:
        CommentContextMenuAction(AZStd::string_view actionName, QObject* parent)
            : ContextMenuAction(actionName, parent)
        {
        }

        using ContextMenuAction::RefreshAction;
        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override
        {
            AZ_UNUSED(targetId);

            bool hasSelectedItems = false;
            SceneRequestBus::EventResult(hasSelectedItems, graphId, &SceneRequests::HasSelectedItems);

            setEnabled(hasSelectedItems);
        }
        
    public:
    
        static ActionGroupId GetCommentContextMenuActionGroupId()
        {
            return AZ_CRC_CE("CommentActionGroup");
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetCommentContextMenuActionGroupId();
        }
    };
}
