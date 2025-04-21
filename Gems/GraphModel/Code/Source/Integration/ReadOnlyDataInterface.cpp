/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Graph Model
#include <GraphModel/Integration/ReadOnlyDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    ReadOnlyDataInterface::ReadOnlyDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    AZStd::string ReadOnlyDataInterface::GetString() const
    {
        GraphModel::SlotPtr slot = m_slot.lock();
        return slot ? slot->GetValue<AZStd::string>() : AZStd::string();
    }
} // namespace GraphModelIntegration
