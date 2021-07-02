/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
