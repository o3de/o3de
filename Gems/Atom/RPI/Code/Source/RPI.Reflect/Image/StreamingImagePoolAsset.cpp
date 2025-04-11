/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace AZ
{
    namespace RPI
    {
        void StreamingImagePoolAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<StreamingImagePoolAsset, Data::AssetData>()
                    ->Version(1)
                    ->Field("m_poolDescriptor", &StreamingImagePoolAsset::m_poolDescriptor)
                    ->Field("m_poolName", &StreamingImagePoolAsset::m_poolName)
                    ;
            }
        }

        void StreamingImagePoolAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        const RHI::StreamingImagePoolDescriptor& StreamingImagePoolAsset::GetPoolDescriptor() const
        {
            return *m_poolDescriptor;
        }

        AZStd::string_view StreamingImagePoolAsset::GetPoolName() const
        {
            return m_poolName;
        }
    }
}
