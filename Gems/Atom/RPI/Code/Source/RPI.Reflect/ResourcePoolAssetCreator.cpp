/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
