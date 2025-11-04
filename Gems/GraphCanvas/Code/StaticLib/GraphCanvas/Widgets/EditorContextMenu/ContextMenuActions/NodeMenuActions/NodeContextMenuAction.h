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
    class NodeContextMenuAction
        : public ContextMenuAction
    {
    protected:
        NodeContextMenuAction(AZStd::string_view actionName, QObject* parent)
            : ContextMenuAction(actionName, parent)
        {
        }
        
    public:
    
        static ActionGroupId GetNodeContextMenuActionGroupId()
        {
            return AZ_CRC_CE("NodeActionGroup");
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetNodeContextMenuActionGroupId();
        }
    };
}
