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