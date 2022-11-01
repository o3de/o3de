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

AZ_DECLARE_BUDGET(RPI);

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
            controller->m_pool->SetLowMemoryCallback(AZStd::bind(&StreamingImageController::OnHandleLowMemory, controller.get()));
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

            // Add the image to the streaming image
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
                UpdateImagePriority(image);
                m_streamableImages.insert(image);
            }
        }

        void StreamingImageController::DetachImage(StreamingImage* image)
        {
            AZ_Assert(image, "Image must not be null.");

            // Remove image from the list first before clearing the image streaming context
            // since the compare function uses the context
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
                m_expandingImages.erase(image);
                m_streamableImages.erase(image);
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

        void StreamingImageController::Update()
        {
            AZ_PROFILE_FUNCTION(RPI);

            // Limit the amount of upload image per update to avoid internal queue being too long
            const uint32_t c_jobCount = 30;
            uint32_t jobCount = 0;

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
                                AZStd::lock_guard<AZStd::mutex> imageListAccesslock(m_imageListAccessMutex);
                                m_expandingImages.erase(image);
                                // remove unused mips in case global mip bias changed 
                                EvictUnusedMips(image);
                                UpdateImagePriority(image);
                                m_streamableImages.insert(image);

                                StreamingDebugOutput("StreamingImageController", "Image [%s] expanded mip level to %d\n",
                                    image->GetRHIImage()->GetName().GetCStr(), image->m_imageAsset->GetMipChainIndex(image->m_mipChainState.m_residencyTarget));
                            }
                        }
                        context->m_queuedForMipExpand = false;
                    }
                    m_mipExpandQueue.pop();

                    jobCount++;
                    if (jobCount >= c_jobCount || m_lastLowMemory > 0)
                    {
                        break;
                    }
                }
            }
            
            // Try to expand if the memory usage is dropping or there are enough free memory
            jobCount = 0;
            if (m_lastLowMemory == 0 || m_lastLowMemory > GetPoolMemoryUsage())
            {
                while (jobCount < c_jobCount)
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
                // reset last low memory
                m_lastLowMemory = 0;
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
                AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
                EvictUnusedMips(image);
                if (m_streamableImages.erase(image))
                {
                    UpdateImagePriority(image);
                    m_streamableImages.insert(image);
                }
            }
        }

        void StreamingImageController::UpdateImagePriority(StreamingImage* image)
        {
            StreamingImage::Priority newPriority = CalculateImagePriority(image);
            image->SetStreamingPriority(newPriority);
        }

        bool StreamingImageController::ImagePriorityComparator::operator()(const StreamingImage* lhs, const StreamingImage* rhs) const
        {
            auto lhsPriority = lhs->GetStreamingPriority();
            auto rhsPriority = rhs->GetStreamingPriority();
            auto lhsTimestamp = lhs->m_streamingContext->GetLastAccessTimestamp();
            auto rhsTimestamp = rhs->m_streamingContext->GetLastAccessTimestamp();
            if (lhsPriority == rhsPriority)
            {
                if (lhsTimestamp == rhsTimestamp)
                {
                    return lhs > rhs;
                }
                else
                {
                    return lhsTimestamp > rhsTimestamp;
                }
            }
            return lhsPriority > rhsPriority; 
        }

        StreamingImage::Priority StreamingImageController::CalculateImagePriority(StreamingImage* image) const
        {
            StreamingImage::Priority newPriority = 0; // 0 means lowest priority and the image mip would be evicted first
            size_t residentMip = image->m_imageAsset->GetMipLevel(image->m_mipChainState.m_residencyTarget);
            size_t targetMip = image->m_streamingContext->GetTargetMip();
            if (residentMip >= targetMip)
            {
                newPriority = residentMip - targetMip;
            }

            return newPriority;
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

        uint32_t StreamingImageController::GetExpandingImageCount() const
        {
            return aznumeric_cast<uint32_t>(m_expandingImages.size());
        }

        void StreamingImageController::SetMipBias(int16_t mipBias)
        {
            int16_t prevMipBias = m_globalMipBias;
            m_globalMipBias = mipBias;
            // if the mipBias increased (using smaller size of mips), go throw each streaming image and evict unneeded mips
            if (m_globalMipBias > prevMipBias)
            {
                StreamingDebugOutput("StreamingImageController", "SetMipBias. EvictUnusedMips %u\n");
                EvictUnusedMips();
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
            AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
            if (m_streamableImages.size() == 0)
            {
                return false; 
            }
            auto ritr = m_streamableImages.rbegin();
            StreamingImage* image = *ritr;

            if (image->IsTrimmable())
            {
                RHI::ResultCode success = image->TrimOneMipChain();
                if (success == RHI::ResultCode::Success)
                {
                    StreamingDebugOutput("StreamingImageController", "Image [%s] has one mipchain released; Current resident mip: %d\n",
                        image->GetRHIImage()->GetName().GetCStr(), image->GetRHIImage()->GetResidentMipLevel());
                    // update the image's priority and re-insert the image 
                    m_streamableImages.erase(image);
                    UpdateImagePriority(image);
                    m_streamableImages.insert(image);
                    return true;
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
            AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
            if (m_streamableImages.size() == 0)
            {
                return false; 
            }
            auto itr = m_streamableImages.begin();
            StreamingImage* image = *itr;

            if (NeedExpand(image))
            {
                image->QueueExpandToNextMipChainLevel();
                if (image->IsExpanding())
                {
                    StreamingDebugOutput("StreamingImageController", "Image [%s] is expanding mip level to %d\n",
                        image->GetRHIImage()->GetName().GetCStr(), image->m_imageAsset->GetMipChainIndex(image->m_mipChainState.m_streamingTarget));
                    m_streamableImages.erase(image);
                    m_expandingImages.insert(image);
                }
                return true;
            }
            return false;
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

        void StreamingImageController::EvictUnusedMips()
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_imageListAccessMutex);
            if (m_streamableImages.size() == 0)
            {
                return; 
            }

            AZStd::vector<StreamingImage*> evictedImages;

            auto itr = m_streamableImages.begin();
            while (itr != m_streamableImages.end())
            {
                StreamingImage* image = *itr;
                if (EvictUnusedMips(image))
                {
                    itr = m_streamableImages.erase(itr);
                    evictedImages.push_back(image);
                }
                else
                {
                    itr++;
                }
            }

            for (auto image : evictedImages)
            {
                UpdateImagePriority(image);
                m_streamableImages.insert(image);
            }
        }

        bool StreamingImageController::OnHandleLowMemory()
        {
            StreamingDebugOutput("StreamingImageController", "Handle low memory\n");

            // Evict some mips
            bool evicted = EvictOneMipChain();

            // Save the memory 
            m_lastLowMemory = GetPoolMemoryUsage();

            return evicted;
        }

        size_t StreamingImageController::GetPoolMemoryUsage()
        {
            size_t totalResident = m_pool->GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_totalResidentInBytes.load();
            return totalResident;
        }
    }
}
