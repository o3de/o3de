/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace GraphCanvas
{
    class DisableContextMenuAction
        : public ContextMenuAction
    {
    protected:
        DisableContextMenuAction(AZStd::string_view actionName, QObject* parent)
            : ContextMenuAction(actionName, parent)
        {
        }
        
    public:

        static ActionGroupId GetDisableContextMenuActionGroupId()
        {
            return AZ_CRC_CE("DisableActionGroup");
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetDisableContextMenuActionGroupId();
        }
    };
}
