/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Integration/BooleanDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    BooleanDataInterface::BooleanDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    bool BooleanDataInterface::GetBool() const
    {
        GraphModel::SlotPtr slot = m_slot.lock();
        return slot ? slot->GetValue<bool>() : false;
    }

    void BooleanDataInterface::SetBool(bool enabled)
    {
        GraphModel::SlotPtr slot = m_slot.lock();
        if (slot && slot->GetValue<bool>() != enabled)
        {
            const GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
            GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

            slot->SetValue(enabled);
            GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
            GraphControllerNotificationBus::Event(
                graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
        }
    }
} // namespace GraphModelIntegration
