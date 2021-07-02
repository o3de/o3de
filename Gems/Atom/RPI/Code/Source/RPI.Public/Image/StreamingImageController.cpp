/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImageController.h>
#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Jobs/Job.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<StreamingImageController> StreamingImageController::Create(const Data::Asset<StreamingImageControllerAsset>& asset, RHI::StreamingImagePool& pool)
        {
            Data::Instance<StreamingImageController> controller = 
                Data::InstanceDatabase<StreamingImageController>::Instance().FindOrCreate(Data::InstanceId::CreateRandom(), asset);

            if (controller)
            {
                controller->m_pool = &pool;
            }

            return controller;
        }

        void StreamingImageController::AttachImage(StreamingImage* image)
        {
            AZ_TRACE_METHOD();

            AZ_Assert(image, "Image must not be null");

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            StreamingImageContextPtr context = CreateContextInternal();
            AZ_Assert(context, "A valid context must be returned from AttachImageInternal.");

            m_contexts.push_back(*context);
            context->m_streamingImage = image;
            image->m_streamingController = this;
            image->m_streamingContext = AZStd::move(context);
        }

        void StreamingImageController::DetachImage(StreamingImage* image)
        {
            AZ_Assert(image, "Image must not be null.");

            const StreamingImageContextPtr& context = image->m_streamingContext;
            AZ_Assert(context, "Image streaming context must not be null.");

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_contexts.erase(*context);
            }

            context->m_queuedForMipExpand = false;
            context->m_streamingImage = nullptr;
            image->m_streamingController = nullptr;
            image->m_streamingContext = nullptr;
        }

        void StreamingImageController::Update()
        {
            AZ_TRACE_METHOD();

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            UpdateInternal(m_timestamp, m_contexts);

            // Limit the amount of upload image per update to avoid internal queue being too long
            const uint32_t c_jobCount = 10;

            // Finalize the mip expansion events generated from the controller. This is done once per update. Anytime
            // a new mip chain asset is ready, the streaming image will notify the controller, which will then queue
            // the request.
            m_mipExpandMutex.lock();
            uint32_t jobCount = 0;
            for (const StreamingImageContextPtr& context : m_mipExpandQueue )
            {
                if (context->m_queuedForMipExpand)
                {
                    if (StreamingImage* image = context->TryGetImage())
                    {
                        const RHI::ResultCode resultCode = image->ExpandMipChain();
                        AZ_Warning("StreamingImageController", resultCode == RHI::ResultCode::Success, "Failed to expand mip chain for streaming image.");
                    }
                    context->m_queuedForMipExpand = false;
                }
                jobCount++;
                if (jobCount >= c_jobCount)
                {
                    break;
                }
            }
            if (jobCount > 0)
            {
                m_mipExpandQueue.erase(m_mipExpandQueue.begin(), m_mipExpandQueue.begin() + jobCount);
            }
            m_mipExpandMutex.unlock();

            // Mip level targets are reset every cycle in order to build the minimum mip level each time.
            m_mipTargetResetMutex.lock();
            for (const StreamingImageContextPtr& context : m_mipTargetResetQueue)
            {
                context->m_queuedForMipTargetReset = false;
                context->m_mipLevelTarget = RHI::Limits::Image::MipCountMax;
            }
            m_mipTargetResetQueue.clear();
            m_mipTargetResetMutex.unlock();

            ++m_timestamp;
        }

        size_t StreamingImageController::GetTimestamp() const
        {
            return m_timestamp;
        }

        void StreamingImageController::OnSetTargetMip(StreamingImage* image, uint16_t mipLevelTarget)
        {
            StreamingImageContext* context = image->m_streamingContext.get();

            // Atomic min operation on target mip level. OnSetTargetMip can be called many times per frame and
            // we want to keep the minimum (most detailed) mip level.
            uint16_t mipLevelPrev = context->m_mipLevelTarget;
            while (mipLevelPrev > mipLevelTarget && !context->m_mipLevelTarget.compare_exchange_weak(mipLevelPrev, mipLevelTarget));

            context->m_lastAccessTimestamp = m_timestamp;

            // Any time a lower mip value is requested, we need to make sure that image is queued for a reset between frames

            const bool queuedForMipTargetReset = context->m_queuedForMipTargetReset.exchange(true);

            if (!queuedForMipTargetReset)
            {
                m_mipTargetResetMutex.lock();
                m_mipTargetResetQueue.emplace_back(context);
                m_mipTargetResetMutex.unlock();
            }
        }

        void StreamingImageController::OnMipChainAssetReady(StreamingImage* image)
        {
            StreamingImageContext* context = image->m_streamingContext.get();

            const bool queuedForMipExpand = context->m_queuedForMipExpand.exchange(true);

            // If the image was already queued for expand, it will take care of all mip chain assets that are ready. 
            // So it's unnecessary to queue again.
            if (!queuedForMipExpand)
            {
                m_mipExpandMutex.lock();
                m_mipExpandQueue.emplace_back(context);
                m_mipExpandMutex.unlock();
            }
        }

        void StreamingImageController::QueueExpandToMipChainLevel(StreamingImage* image, size_t mipChainIndex)
        {
            image->QueueExpandToMipChainLevel(mipChainIndex);
        }

        void StreamingImageController::TrimToMipChainLevel(StreamingImage* image, size_t mipChainIndex)
        {
            image->TrimToMipChainLevel(mipChainIndex);
        }

        StreamingImageContextPtr StreamingImageController::CreateContextInternal()
        {
            return aznew StreamingImageContext();
        }
    }
}
