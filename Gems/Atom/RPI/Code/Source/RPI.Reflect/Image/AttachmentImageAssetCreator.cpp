/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void AttachmentImageAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void AttachmentImageAssetCreator::SetImageDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetImageViewDescriptor(const RHI::ImageViewDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageViewDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetPoolAsset(const Data::Asset<ResourcePoolAsset>& poolAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolAsset = poolAsset;
            }
        }

        void AttachmentImageAssetCreator::SetOptimizedClearValue(const RHI::ClearValue& clearValue)
        {
            if (ValidateIsReady())
            {
                m_asset->m_optimizedClearValue = AZStd::make_shared<RHI::ClearValue>(clearValue);
            }
        }

        bool AttachmentImageAssetCreator::End(Data::Asset<AttachmentImageAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            // If the pool wasn't provided, use the system default attachment pool
            if (!m_asset->m_poolAsset.GetId().IsValid())
            {
                Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
                m_asset->m_poolAsset = {pool->GetAssetId(), azrtti_typeid<ResourcePoolAsset>()};
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
        
        void AttachmentImageAssetCreator::SetAssetHint(AZStd::string_view hint)
        {
            m_asset.SetHint(hint);
        }

        void AttachmentImageAssetCreator::SetName(const AZ::Name& uniqueName, bool isUniqueName)
        {
            if (uniqueName.IsEmpty())
            {
                m_asset->m_isUniqueName = false;
                AZ_Warning("RPI", false, "Can't set empty string as unique name");
            }
            else
            {
                m_asset->m_isUniqueName = isUniqueName;
            }
            m_asset->m_name = uniqueName;

        }
    }
}
