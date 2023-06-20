/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Integration/FloatDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    FloatDataInterface::FloatDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    double FloatDataInterface::GetNumber() const
    {
        GraphModel::SlotPtr slot = m_slot.lock();
        return slot ? slot->GetValue<float>() : 0.0;
    }

    void FloatDataInterface::SetNumber(double value)
    {
        GraphModel::SlotPtr slot = m_slot.lock();
        if (slot && slot->GetValue<float>() != static_cast<float>(value))
        {
            const GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
            GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

            slot->SetValue(static_cast<float>(value));
            GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
            GraphControllerNotificationBus::Event(
                graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
        }
    }

    double FloatDataInterface::GetMin() const
    {
        return std::numeric_limits<float>::lowest();
    }

    double FloatDataInterface::GetMax() const
    {
        return std::numeric_limits<float>::max();
    }
} // namespace GraphModelIntegration
