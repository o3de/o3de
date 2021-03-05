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

#include <AzFramework/Asset/Benchmark/BenchmarkAsset.h>

namespace AzFramework
{
    void BenchmarkAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BenchmarkAsset, AZ::Data::AssetData>()
                ->Version(0)
                ->Field("BufferSize", &BenchmarkAsset::m_bufferSize)
                ->Field("Buffer", &BenchmarkAsset::m_buffer)
                ->Field("Dependencies", &BenchmarkAsset::m_assetReferences)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<BenchmarkAsset>(
                    "Benchmark Asset", "Generated benchmark asset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkAsset::m_bufferSize, "Buffer Size", "Size of the buffer in bytes")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkAsset::m_buffer, "Buffer", "Aribtrary data buffer")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BenchmarkAsset::m_assetReferences, "Dependencies", "Asset dependencies to load with this asset")
                    ;
            }
        }
    }
} // namespace AzFramework
