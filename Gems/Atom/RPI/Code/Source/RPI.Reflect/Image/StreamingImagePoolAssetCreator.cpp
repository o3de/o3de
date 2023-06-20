/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void StreamingImagePoolAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void StreamingImagePoolAssetCreator::SetPoolDescriptor(AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor>&& descriptor)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!descriptor)
            {
                ReportError("You must provide a valid pool descriptor instance.");
                return;
            }

            m_asset->m_poolDescriptor = AZStd::move(descriptor);
        }

        void StreamingImagePoolAssetCreator::SetPoolName(AZStd::string_view poolName)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolName = poolName;
            }
        }

        bool StreamingImagePoolAssetCreator::End(Data::Asset<StreamingImagePoolAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_asset->m_poolDescriptor)
            {
                ReportError("Streaming image pool was not assigned a pool descriptor.");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
    }
}
