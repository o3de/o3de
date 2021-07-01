/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/DefaultStreamingImageController.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<DefaultStreamingImageController> DefaultStreamingImageController::FindOrCreate(const Data::Asset<DefaultStreamingImageControllerAsset>& asset)
        {
            return azrtti_cast<DefaultStreamingImageController*>(
                Data::InstanceDatabase<StreamingImageController>::Instance().FindOrCreate(
                    Data::InstanceId::CreateFromAssetId(asset.GetId()),
                    asset));
        }

        AZ::Data::Instance<DefaultStreamingImageController> DefaultStreamingImageController::CreateInternal(Data::AssetData* assetData)
        {
            DefaultStreamingImageControllerAsset* specificAsset = azrtti_cast<DefaultStreamingImageControllerAsset*>(assetData);
            if (!specificAsset)
            {
                AZ_Error("DefaultStreamingImageController", false, "DefaultStreamingImageController instance requires a DefaultStreamingImageControllerAsset.");
                return nullptr;
            }

            Data::Instance<DefaultStreamingImageController> instance = aznew DefaultStreamingImageController();

            const RHI::ResultCode resultCode = instance->Init(*specificAsset);
            if (resultCode == RHI::ResultCode::Success)
            {
                return instance;
            }

            return nullptr;
        }

        RHI::ResultCode DefaultStreamingImageController::Init([[maybe_unused]] DefaultStreamingImageControllerAsset& imageControllerAsset)
        {
            return RHI::ResultCode::Success;
        }

        StreamingImageContextPtr DefaultStreamingImageController::CreateContextInternal()
        {
            StreamingImageContextPtr context = aznew StreamingImageContext();
            m_recentlyAttachedContexts.emplace_back(context);
            return context;
        }

        void DefaultStreamingImageController::UpdateInternal(size_t timestamp, const StreamingImageContextList& contexts)
        {
            AZ_UNUSED(timestamp);
            AZ_UNUSED(contexts);

            const uint32_t maxExpandsCount = 20;
            uint32_t mipsExpandsPerUpdate = 0;

            // [GFX TODO] [ATOM-551] Streaming image control of DefaultStreamingImageController
            // For now we always stream in all the mips. Of course we need to make this smarter at some point.
            if (!m_recentlyAttachedContexts.empty())
            {
                for (auto& context : m_recentlyAttachedContexts)
                {
                    if (StreamingImage* image = context->TryGetImage())
                    {
                        QueueExpandToMipChainLevel(image, 0);
                        mipsExpandsPerUpdate++;
                        if (mipsExpandsPerUpdate >= maxExpandsCount)
                        {
                            break;
                        }
                    }
                }
                m_recentlyAttachedContexts.erase(m_recentlyAttachedContexts.begin(), m_recentlyAttachedContexts.begin() + mipsExpandsPerUpdate);
            }
        }
    }
}
