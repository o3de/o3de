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
