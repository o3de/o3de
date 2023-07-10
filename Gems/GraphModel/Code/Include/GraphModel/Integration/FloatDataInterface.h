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
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    //! Satisfies GraphCanvas API requirements for showing float property widgets in nodes.
    class FloatDataInterface : public GraphCanvas::NumericDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(FloatDataInterface, AZ::SystemAllocator);

        FloatDataInterface(GraphModel::SlotPtr slot);
        ~FloatDataInterface() = default;

        double GetNumber() const override;
        void SetNumber(double value) override;

        double GetMin() const override;
        double GetMax() const override;

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
} // namespace GraphModelIntegration
