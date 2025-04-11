/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Components/SceneBus.h>

DECLARE_EBUS_INSTANTIATION(GraphCanvas::SceneRequests);

namespace GraphCanvas
{
    void SceneRequests::HandleProposalDaisyChain(const NodeId& startNode, SlotType slotType, ConnectionType connectionType, const QPoint& screenPoint, const QPointF& focusPoint)
    {
        HandleProposalDaisyChainWithGroup(startNode, slotType, connectionType, screenPoint, focusPoint, AZ::EntityId());
    }
}
