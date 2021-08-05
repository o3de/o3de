/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <typename TYPE>
    bool DeltaSerializerCreate::CreateDelta(TYPE& base, TYPE& current)
    {
        // Gather value records from the base object
        m_gatheringRecords = true;
        if (!base.Serialize(*this))
        {
            return false;
        }

        m_objectCounter = 0;
        // Compile deltas from the new object
        m_gatheringRecords = false;
        if (!current.Serialize(*this))
        {
            return false;
        }

        // Update the delta buffer size based on how much data was serialized
        m_delta.SetBufferSize(m_dataSerializer.GetSize());
        return true;
    }

    template <typename TYPE>
    bool DeltaSerializerApply::ApplyDelta(TYPE& output)
    {
        return output.Serialize(*this);
    }
}
