/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/Benchmark/BenchmarkAsset.h>
#include <AzCore/Asset/AssetSerializer.h>

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
