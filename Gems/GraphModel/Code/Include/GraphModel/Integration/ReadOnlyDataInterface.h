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
#include <GraphCanvas/Components/NodePropertyDisplay/ReadOnlyDataInterface.h>

// Graph Model
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    //! Satisfies GraphCanvas API requirements for showing read only property widgets in nodes.
    class ReadOnlyDataInterface
        : public GraphCanvas::ReadOnlyDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(ReadOnlyDataInterface, AZ::SystemAllocator, 0);

        ReadOnlyDataInterface(GraphModel::SlotPtr slot);
        ~ReadOnlyDataInterface() = default;

        // GraphCanvas::ReadOnlyDataInterface overrides ...
        AZStd::string GetString() const override;

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
}
