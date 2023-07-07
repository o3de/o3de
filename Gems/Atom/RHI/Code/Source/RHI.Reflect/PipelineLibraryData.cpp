/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/PipelineLibraryData.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void PipelineLibraryData::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PipelineLibraryData>()
                ->Version(1)
                ->Field("m_data", &PipelineLibraryData::m_data);
        }
    }

    ConstPtr<PipelineLibraryData> PipelineLibraryData::Create(AZStd::vector<uint8_t>&& data)
    {
        return aznew PipelineLibraryData(AZStd::move(data));
    }

    PipelineLibraryData::PipelineLibraryData(AZStd::vector<uint8_t>&& data)
        : m_data{AZStd::move(data)}
    {}

    AZStd::span<const uint8_t> PipelineLibraryData::GetData() const
    {
        return m_data;
    }
}
