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

#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    namespace RPI
    {
        void ResourcePoolAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void ResourcePoolAssetCreator::SetPoolDescriptor(AZStd::unique_ptr<RHI::ResourcePoolDescriptor> poolDescriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolDescriptor = AZStd::move(poolDescriptor);
            }
        }
      
        void ResourcePoolAssetCreator::SetPoolName(AZStd::string_view poolName)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolName = poolName;
            }
        }

        bool ResourcePoolAssetCreator::End(Data::Asset<ResourcePoolAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_asset->m_poolDescriptor)
            {
                ReportError("The asset doesn't have valid pool descriptor");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }

    } // namespace RPI
} // namespace AZ
