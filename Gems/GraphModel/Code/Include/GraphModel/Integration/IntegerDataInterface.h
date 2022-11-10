/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>

// Graph Model
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Integration/IntegrationBus.h>
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    //! Satisfies GraphCanvas API requirements for showing int property widgets in nodes.
    template<typename T>
    class IntegerDataInterface : public GraphCanvas::NumericDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(IntegerDataInterface, AZ::SystemAllocator, 0);

        IntegerDataInterface(GraphModel::SlotPtr slot)
            : m_slot(slot)
        {
        }

        ~IntegerDataInterface() = default;

        double GetNumber() const
        {
            GraphModel::SlotPtr slot = m_slot.lock();
            return static_cast<double>(slot ? slot->GetValue<T>() : 0.0);
        }

        void SetNumber(double value)
        {
            if (GraphModel::SlotPtr slot = m_slot.lock())
            {
                if (static_cast<T>(value) != slot->GetValue<T>())
                {
                    const GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
                    GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                    slot->SetValue(static_cast<T>(value));
                    GraphControllerNotificationBus::Event(
                        graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
                    GraphControllerNotificationBus::Event(
                        graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
                }
            }
        }

        int GetDecimalPlaces() const
        {
            return 0;
        }

        int GetDisplayDecimalPlaces() const
        {
            return 0;
        }

        double GetMin() const
        {
            return static_cast<double>(AZStd::numeric_limits<T>::min());
        }

        double GetMax() const
        {
            return static_cast<double>(AZStd::numeric_limits<T>::max());
        }

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
} // namespace GraphModelIntegration
