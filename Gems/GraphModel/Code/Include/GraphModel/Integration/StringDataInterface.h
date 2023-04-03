/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>

// Graph Model
#include <GraphModel/Model/Slot.h>

namespace GraphModelIntegration
{
    //! Satisfies GraphCanvas API requirements for showing string property widgets in nodes.
    class StringDataInterface : public GraphCanvas::StringDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(StringDataInterface, AZ::SystemAllocator);

        StringDataInterface(GraphModel::SlotPtr slot);
        ~StringDataInterface() = default;

        AZStd::string GetString() const;
        void SetString(const AZStd::string& value);

    private:
        AZStd::weak_ptr<GraphModel::Slot> m_slot;
    };
} // namespace GraphModelIntegration
