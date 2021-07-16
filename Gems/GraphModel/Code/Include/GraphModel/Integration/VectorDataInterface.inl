/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

// Graph Model
#include <GraphModel/Integration/IntegrationBus.h>
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    template<class Type, int ElementCount>
    class VectorDataInterface
        : public GraphCanvas::VectorDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(VectorDataInterface, AZ::SystemAllocator, 0);

        VectorDataInterface(GraphModel::SlotPtr slot)
            : m_slot(slot)
        {
        }
        ~VectorDataInterface() = default;

        const char* GetLabel(int index) const override
        {
            if (index == 0)
            {
                return "X";
            }
            else if (index == 1)
            {
                return "Y";
            }
            else if (index == 2)
            {
                return "Z";
            }
            else if (index == 3)
            {
                return "W";
            }

            return "???";
        }
        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }
        AZStd::string GetElementStyle(int index) const override
        {
            return AZStd::string::format("vector_%i", index);
        }
        int GetElementCount() const override
        {
            return ElementCount;
        }

        double GetValue(int index) const override
        {
            if (GraphModel::SlotPtr slot = m_slot.lock())
            {
                return slot->GetValue<Type>().GetElement(index);
            }
            else
            {
                return 0.0;
            }
        }
        void SetValue(int index, double value) override
        {
            if (GraphModel::SlotPtr slot = m_slot.lock())
            {
                Type vector = slot->GetValue<Type>();
                if (value != vector.GetElement(index))
                {
                    vector.SetElement(index, aznumeric_cast<float>(value));
                    slot->SetValue(vector);

                    GraphCanvas::GraphId graphCanvasSceneId;
                    IntegrationBus::BroadcastResult(graphCanvasSceneId, &IntegrationBusInterface::GetActiveGraphCanvasSceneId);
                    IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, graphCanvasSceneId);
                }
            }
        }

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
}
