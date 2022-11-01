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
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            return slot->GetValue<float>();
        }
        else
        {
            return 0.0;
        }
    }
    void FloatDataInterface::SetNumber(double value)
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            if (static_cast<float>(value) != slot->GetValue<float>())
            {
                const GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
                GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                slot->SetValue(static_cast<float>(value));
                GraphControllerNotificationBus::Event(
                    graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
                GraphControllerNotificationBus::Event(
                    graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
            }
        }
    }

    int FloatDataInterface::GetDecimalPlaces() const
    {
        return 7;
    }
    int FloatDataInterface::GetDisplayDecimalPlaces() const
    {
        return 4;
    }

    double FloatDataInterface::GetMin() const
    {
        return std::numeric_limits<float>::lowest();
    }
    double FloatDataInterface::GetMax() const
    {
        return std::numeric_limits<float>::max();
    }
}
