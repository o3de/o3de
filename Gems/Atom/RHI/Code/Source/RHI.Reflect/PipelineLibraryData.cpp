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

#include <Atom/RHI.Reflect/PipelineLibraryData.h>

namespace AZ
{
    namespace RHI
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

        AZStd::array_view<uint8_t> PipelineLibraryData::GetData() const
        {
            return m_data;
        }
    }
}