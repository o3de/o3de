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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/GeneralMenuActions/GeneralMenuActions.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &SlotRequests::GetName);

        AZStd::string tooltip;
        SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }
}
