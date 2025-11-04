/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RPI
    {
        void ResourcePoolAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ResourcePoolAsset>()
                    ->Field("PoolName", &ResourcePoolAsset::m_poolName)
                    ->Field("PoolDescriptor", &ResourcePoolAsset::m_poolDescriptor)
                    ;
            }
        }

        ResourcePoolAsset::ResourcePoolAsset(const AZ::Data::AssetId& assetId)
            : AZ::Data::AssetData(assetId)
        {}


        AZStd::string_view ResourcePoolAsset::GetPoolName() const
        {
            return m_poolName;
        }

        const AZStd::shared_ptr<RHI::ResourcePoolDescriptor>& ResourcePoolAsset::GetPoolDescriptor() const
        {
            return m_poolDescriptor;
        }

        void ResourcePoolAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

     } // namespace RPI
} // namespace AZ
