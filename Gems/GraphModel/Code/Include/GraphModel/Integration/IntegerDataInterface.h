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

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>

// Graph Model
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    //! Satisfies GraphCanvas API requirements for showing int property widgets in nodes.
    class IntegerDataInterface
        : public GraphCanvas::NumericDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(IntegerDataInterface, AZ::SystemAllocator, 0);

        IntegerDataInterface(GraphModel::SlotPtr slot);
        ~IntegerDataInterface() = default;
        
        double GetNumber() const override;
        void SetNumber(double value) override;

        int GetDecimalPlaces() const override;
        int GetDisplayDecimalPlaces() const override;
        double GetMin() const override;
        double GetMax() const override;

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
}
