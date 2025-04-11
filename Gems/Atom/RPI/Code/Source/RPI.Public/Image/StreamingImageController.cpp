/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImageController.h>
#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Time/ITime.h>

ATOM_RPI_PUBLIC_API AZ_DECLARE_BUDGET(RPI);

#define ENABLE_STREAMING_DEBUG_TRACE 0

namespace AZ
{
    namespace RPI
    {
#if ENABLE_STREAMING_DEBUG_TRACE
        #define StreamingDebugOutput(window, ...) AZ_TracePrintf(window, __VA_ARGS__);
#else
        #define StreamingDebugOutput(window, ...)
#endif

        AZStd::unique_ptr<StreamingImageController> StreamingImageController::Create(RHI::StreamingImagePool& pool)
        {
            AZStd::unique_ptr<StreamingImageController> controller = AZStd::make_unique<StreamingImageController>();
            controller->m_pool = &pool;
            controller->m_pool->SetLowMemoryCallback(AZStd::bind(&StreamingImageController::ReleaseMemory, controller.get(), AZStd::placeholders::_1));
            return controller;
        }

        void StreamingImageController::AttachImage(StreamingImage* image)
        {
            AZ_PROFILE_FUNCTION(RPI);
            AZ_Assert(image, "Image must not be null");

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_contextAccessMutex);

                StreamingImageContextPtr context = CreateContext();
                AZ_Assert(context, "A valid context must be returned from AttachImageInternal.");

                m_contexts.push_back(*context);
                context->m_streamingImage = image;
                image->m_streamingController = this;
                image->m_streamingContext = AZStd::move(context);
            }
            
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
                m_streamableImages.insert(image);
            }

            ReinsertImageToLists(image);
        }

        void StreamingImageController::DetachImage(StreamingImage* image)
        {
            AZ_Assert(image, "Image must not be null.");

            // Remove image from the list first before clearing the image streaming context
            // since the compare functions may use the image's StreamingImageContext
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
                m_streamableImages.erase(image);
                m_expandingImages.erase(image);
                m_expandableImages.erase(image);
                m_evictableImages.erase(image);
            }

            const StreamingImageContextPtr& context = image->m_streamingContext;
            AZ_Assert(context, "Image streaming context must not be null.");

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_contextAccessMutex);
                m_contexts.erase(*context);
            }

            context->m_queuedForMipExpand = false;
            context->m_streamingImage = nullptr;
            image->m_streamingController = nullptr;
            image->m_streamingContext = nullptr;
        }

        void StreamingImageController::ReinsertImageToLists(StreamingImage* image)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
            m_expandableImages.erase(image);
            m_evictableImages.erase(image);

            if (!image->IsExpanding())
            {
                image->m_streamingContext->UpdateMipStats();

                if (NeedExpand(image))
                {
                    m_expandableImages.insert(image);
                }
                if (image->IsTrimmable())
                {
                    m_evictableImages.insert(image);
                }
            }
        }

        void StreamingImageController::EndExpandImage(StreamingImage* image)
        {
            // remove unused mips in case global mip bias was changed during expanding
            EvictUnusedMips(image);

            image->m_streamingContext->m_queuedForMipExpand = false;

            ReinsertImageToLists(image);
        }

        void StreamingImageController::Update()
        {
            AZ_PROFILE_FUNCTION(RPI);

            // Limit the amount of upload image per update to avoid internal queue being too long
            const uint32_t c_jobCount = 30;
            uint32_t jobCount = 0;

            // if the memory was low, cancel all expanding images
            if (m_lastLowMemory)
            {
                // clear the gpu expand queue
                {
                    AZStd::lock_guard<AZStd::mutex> mipExpandlock(m_mipExpandMutex);
                    m_mipExpandQueue = AZStd::queue<StreamingImageContextPtr>();
                }

                // clear the expanding images
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> imageListAccesslock(m_imageListAccessMutex);
                    for (auto image:m_expandingImages)
                    {
                        image->CancelExpanding();
                        EndExpandImage(image);
                    }
                    m_expandingImages.clear();
                }
            }

            // Finalize the mip expansion events generated from the controller. This is done once per update. Anytime
            // a new mip chain asset is ready, the streaming image will notify the controller, which will then queue
            // the request.
            {
                AZStd::lock_guard<AZStd::mutex> mipExpandlock(m_mipExpandMutex);
                while (m_mipExpandQueue.size() > 0)
                {
                    StreamingImageContextPtr context = m_mipExpandQueue.front();
                    if (context->m_queuedForMipExpand)
                    {
                    
                        if (StreamingImage* image = context->TryGetImage())
                        {
                            StreamingDebugOutput("StreamingImageController", "ExpandMipChain for [%s]\n", image->GetRHIImage()->GetName().GetCStr());
                            [[maybe_unused]] const RHI::ResultCode resultCode = image->ExpandMipChain();
                            AZ_Warning("StreamingImageController", resultCode == RHI::ResultCode::Success, "Failed to expand mip chain for streaming image.");

                            if (!image->IsExpanding())
                            {
                                EndExpandImage(image);
                                m_expandingImages.erase(image);

                                StreamingDebugOutput("StreamingImageController", "Image [%s] expanded mip level to %d\n",
                                    image->GetRHIImage()->GetName().GetCStr(), image->m_imageAsset->GetMipChainIndex(image->m_mipChainState.m_residencyTarget));
                            }
                        }
                    }
                    m_mipExpandQueue.pop();

                    jobCount++;
                    if (jobCount >= c_jobCount || m_lastLowMemory > 0)
                    {
                        break;
                    }
                }
            }

            // reset low memory state if the memory is dropping since last low memory state
            if (m_lastLowMemory > GetPoolMemoryUsage())
            {
                m_lastLowMemory = 0;
            }
            
            // Try to expand if it's not in low memory state
            jobCount = 0;
            while (jobCount < c_jobCount && m_lastLowMemory == 0)
            {
                if (ExpandOneMipChain())
                {
                    jobCount++;
                }
                else
                {
                    break;
                }
            }

            ++m_timestamp;
        }

        size_t StreamingImageController::GetTimestamp() const
        {
            return m_timestamp;
        }

        void StreamingImageController::OnSetTargetMip(StreamingImage* image, uint16_t mipLevelTarget)
        {
            StreamingImageContext* context = image->m_streamingContext.get();

            context->m_mipLevelTarget = mipLevelTarget;
            context->m_lastAccessTimestamp = m_timestamp;

            // update image priority and re-insert the image
            if (!context->m_queuedForMipExpand)
            {
                EvictUnusedMips(image);
            }

            // reinsert the image since the priority might be changed after mip target changed
            ReinsertImageToLists(image);
        }

        bool StreamingImageController::ExpandPriorityComparator::operator()(const StreamingImage* lhs, const StreamingImage* rhs) const
        {
            // use the resident mip size and missing mip count to decide the expand priority
            auto lhsMipSize = lhs->m_streamingContext->m_residentMipSize;
            auto rhsMipSize = rhs->m_streamingContext->m_residentMipSize;

            if (lhsMipSize == rhsMipSize)
            {
                auto lhsMissingMips = lhs->m_streamingContext->m_missingMips;
                auto rhsMissingMips = rhs->m_streamingContext->m_missingMips;
                if (lhsMissingMips == rhsMissingMips)
                {
                    auto lhsTimestamp = lhs->m_streamingContext->GetLastAccessTimestamp();
                    auto rhsTimestamp = rhs->m_streamingContext->GetLastAccessTimestamp();
                    if (lhsTimestamp == rhsTimestamp)
                    {
                        // we need this to avoid same key in the AZStd::set
                        return lhs < rhs;
                    }
                    // latest accessed image has higher priority
                    return lhsTimestamp > rhsTimestamp;
                }
                else
                {
                    // image with more missing mips has higher priority
                    return lhsMissingMips > rhsMissingMips; 
                }
            }

            // image has smaller resolution has higher priority
            return lhsMipSize < rhsMipSize;
        }
        
        bool StreamingImageController::EvictPriorityComparator::operator()(const StreamingImage* lhs, const StreamingImage* rhs) const
        {
            auto lhsEvictableMips = lhs->m_streamingContext->m_evictableMips;
            auto rhsEvictableMips = rhs->m_streamingContext->m_evictableMips;

            if (lhsEvictableMips == rhsEvictableMips)
            {
                auto lhsTimestamp = lhs->m_streamingContext->GetLastAccessTimestamp();
                auto rhsTimestamp = rhs->m_streamingContext->GetLastAccessTimestamp();
                if (lhsTimestamp == rhsTimestamp)
                {
                    // we need this to avoid same key in the AZStd::set
                    return lhs < rhs;
                }

                // Last visited image will be evicted later
                return lhsTimestamp < rhsTimestamp;
            }

            // images with higher evictable mip count will be evict first
            return lhsEvictableMips > rhsEvictableMips; 
        }

        void StreamingImageController::OnMipChainAssetReady(StreamingImage* image)
        {
            StreamingImageContext* context = image->m_streamingContext.get();

            const bool queuedForMipExpand = context->m_queuedForMipExpand.exchange(true);

            // If the image was already queued for expand, it will take care of all mip chain assets that are ready. 
            // So it's unnecessary to queue again.
            if (!queuedForMipExpand)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mipExpandMutex);
                m_mipExpandQueue.push(context);
            }
        }
        
        uint32_t StreamingImageController::GetStreamableImageCount() const
        {
            return aznumeric_cast<uint32_t>(m_streamableImages.size());
        }

        uint32_t StreamingImageController::GetExpandingImageCount() const
        {
            return aznumeric_cast<uint32_t>(m_expandingImages.size());
        }

        void StreamingImageController::SetMipBias(int16_t mipBias)
        {
            if (m_globalMipBias == mipBias)
            {
                return;
            }

            m_globalMipBias = mipBias;

            // we need go through all the streamable image to update their streaming context and regenerate the lists
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
            m_expandableImages.clear();
            m_evictableImages.clear();

            for (auto image : m_streamableImages)
            {
                EvictUnusedMips(image);
                image->m_streamingContext->UpdateMipStats();
                if (!image->IsExpanding())
                {
                    if (NeedExpand(image))
                    {
                        m_expandableImages.insert(image);
                    }
                    if (image->IsTrimmable())
                    {
                        m_evictableImages.insert(image);
                    }
                }
            }
        }

        int16_t StreamingImageController::GetMipBias() const
        {
            return m_globalMipBias;
        }

        StreamingImageContextPtr StreamingImageController::CreateContext()
        {
            return aznew StreamingImageContext();
        }

        void StreamingImageController::ResetLowMemoryState()
        {
            m_lastLowMemory = 0;
        }

        bool StreamingImageController::EvictOneMipChain()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
            for (auto itr = m_evictableImages.begin(); itr != m_evictableImages.end(); itr++)
            {
                StreamingImage* image = *itr;

                RHI::ResultCode success = image->TrimOneMipChain();

                if (success == RHI::ResultCode::Success)
                {
                    // update the image's priority and re-insert the image
                    ReinsertImageToLists(image);

                    StreamingDebugOutput(
                        "StreamingImageController",
                        "Image [%s] has one mipchain released; Current resident mip: %d\n",
                        image->GetRHIImage()->GetName().GetCStr(),
                        image->GetRHIImage()->GetResidentMipLevel());
                    return true;
                }
                else
                {
                    AZ_Assert(false, "failed to evict an evictable image!");
                }
            }

            return false;
        }

        bool StreamingImageController::NeedExpand(const StreamingImage* image) const
        {
            uint16_t targetMip = GetImageTargetMip(image);
            // only need expand if the current expanding target is smaller than final target
            return image->m_mipChainState.m_streamingTarget > image->m_imageAsset->GetMipChainIndex(targetMip);
        }

        bool StreamingImageController::ExpandOneMipChain()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_imageListAccessMutex);
            if (m_expandableImages.size() == 0)
            {
                return false; 
            }
            auto itr = m_expandableImages.begin();
            StreamingImage* image = *itr;
            image->QueueExpandToNextMipChainLevel();
            if (image->IsExpanding())
            {
                StreamingDebugOutput("StreamingImageController", "Image [%s] is expanding mip level to %d\n",
                    image->GetRHIImage()->GetName().GetCStr(), image->m_imageAsset->GetMipChainIndex(image->m_mipChainState.m_streamingTarget));
                m_expandingImages.insert(image);
                ReinsertImageToLists(image);
            }
            return true;
        }

        uint16_t StreamingImageController::GetImageTargetMip(const StreamingImage* image) const
        {
            int16_t targetMip = image->m_streamingContext->GetTargetMip() + m_globalMipBias;
            targetMip = AZStd::clamp(targetMip, (int16_t)0,  aznumeric_cast<int16_t>(image->GetRHIImage()->GetDescriptor().m_mipLevels-1));
            return aznumeric_cast<uint16_t>(targetMip);
        }

        bool StreamingImageController::IsMemoryLow() const
        {
            return m_lastLowMemory != 0;
        }

        bool StreamingImageController::EvictUnusedMips(StreamingImage* image)
        {
            uint16_t targetMip = GetImageTargetMip(image);
            size_t targetMipChain = image->m_imageAsset->GetMipChainIndex(targetMip);
            if (image->m_mipChainState.m_streamingTarget < targetMipChain)
            {
                RHI::ResultCode success = image->TrimToMipChainLevel(targetMipChain);
            
                StreamingDebugOutput("StreamingImageController", "Image [%s] mip level was evicted to %d\n",
                    image->GetRHIImage()->GetName().GetCStr(), image->GetResidentMipLevel());

                return success == RHI::ResultCode::Success;
            }
            return true;
        }

        bool StreamingImageController::ReleaseMemory(size_t targetMemoryUsage)
        {
            StreamingDebugOutput("StreamingImageController", "Handle low memory\n");

            size_t currentResident = GetPoolMemoryUsage();

            while (currentResident > targetMemoryUsage)
            {
                // Evict some mips
                bool evicted = EvictOneMipChain();
                if (!evicted)
                {
                    // nothing to be evicted anymore
                    m_lastLowMemory = currentResident;
                    return false;
                }
                currentResident = GetPoolMemoryUsage();
            }

            m_lastLowMemory = currentResident;
            return true;
        }

        size_t StreamingImageController::GetPoolMemoryUsage()
        {
            size_t totalResident = m_pool->GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_usedResidentInBytes.load();
            return totalResident;
        }
    }
}
