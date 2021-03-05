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

#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RPI
    {
        const char* ResourcePoolAsset::DisplayName = "ResourcePool";
        const char* ResourcePoolAsset::Group = "RenderingPipeline";
        const char* ResourcePoolAsset::Extension = "pool";

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
