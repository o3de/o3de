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

        void StreamingImagePoolAssetCreator::SetControllerAsset(const Data::Asset<StreamingImageControllerAsset>& controllerAsset)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!controllerAsset.GetId().IsValid())
            {
                ReportError("You must provide a valid controller asset reference.");
                return;
            }

            m_asset->m_controllerAsset = controllerAsset;
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

            if (!m_asset->m_controllerAsset.GetId().IsValid())
            {
                ReportError("Streaming image pool was not assigned a controller asset.");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
    }
}
