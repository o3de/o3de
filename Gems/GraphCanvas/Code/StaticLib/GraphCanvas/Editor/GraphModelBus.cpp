/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

bool GraphCanvas::GraphModelRequests::IsSlotConnected(const Endpoint& endpoint) const
{
    bool connected = false;
    SlotRequestBus::EventResult(connected, endpoint.GetSlotId(), &SlotRequests::HasConnections);
    return connected;
}

DECLARE_EBUS_INSTANTIATION(GraphCanvas::GraphModelRequests);
