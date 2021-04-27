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
            return AZ_CRC("DisableActionGroup", 0xf0c8972d);
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetDisableContextMenuActionGroupId();
        }
    };
}
