/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Image/StreamingImageController.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>

#include <Atom/RHI/Factory.h>

#include <AtomCore/Instance/InstanceDatabase.h>

// Enable this define to debug output streaming image initialization and expanding process.
//#define AZ_RPI_STREAMING_IMAGE_DEBUG_LOG

AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
    namespace RPI
    {
        Data::Instance<StreamingImage> StreamingImage::FindOrCreate(const Data::Asset<StreamingImageAsset>& streamingImageAsset)
        {
            return Data::InstanceDatabase<StreamingImage>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(streamingImageAsset.GetId()),
                streamingImageAsset);
        }

        Data::Instance<StreamingImage> StreamingImage::CreateFromCpuData(
            const StreamingImagePool& streamingImagePool,
            RHI::ImageDimension imageDimension,
            RHI::Size imageSize,
            RHI::Format imageFormat,
            const void* imageData,
            size_t imageDataSize,
            Uuid id)
        {
            Data::Instance<StreamingImage> existingImage = Data::InstanceDatabase<StreamingImage>::Instance().Find(Data::InstanceId::CreateFromAssetId(id));
            AZ_Error("StreamingImage", !existingImage, "StreamingImage::CreateFromCpuData found an existing entry in the instance database for the provided id.");

            RHI::ImageDescriptor imageDescriptor;
            imageDescriptor.m_bindFlags = RHI::ImageBindFlags::ShaderRead;
            imageDescriptor.m_dimension = imageDimension;
            imageDescriptor.m_size = imageSize;
            imageDescriptor.m_format = imageFormat;

            const RHI::ImageSubresourceLayout imageSubresourceLayout = RHI::GetImageSubresourceLayout(imageDescriptor, RHI::ImageSubresource{});

            const size_t expectedImageDataSize = imageSubresourceLayout.m_bytesPerImage * imageDescriptor.m_size.m_depth;
            if (expectedImageDataSize != imageDataSize)
            {
                AZ_Error("StreamingImage", false, "StreamingImage::CreateFromCpuData expected '%d' bytes of image data, but got '%d' instead.", expectedImageDataSize, imageDataSize);
                return nullptr;
            }

            Data::Asset<ImageMipChainAsset> mipChainAsset;

            // Construct the mip chain asset.
            {
                ImageMipChainAssetCreator assetCreator;
                assetCreator.Begin(Uuid::CreateRandom(), 1, 1);
                assetCreator.BeginMip(imageSubresourceLayout);
                assetCreator.AddSubImage(imageData, expectedImageDataSize);
                assetCreator.EndMip();
                if (!assetCreator.End(mipChainAsset))
                {
                    AZ_Error("StreamingImage", false, "Failed to initialize mip chain asset");
                    return nullptr;
                }
            }

            Data::Asset<StreamingImageAsset> streamingImageAsset;

            // Construct the streaming image asset.
            {
                StreamingImageAssetCreator assetCreator;
                assetCreator.Begin(id);
                assetCreator.SetImageDescriptor(imageDescriptor);
                assetCreator.AddMipChainAsset(*mipChainAsset.Get());
                assetCreator.SetFlags(StreamingImageFlags::NotStreamable);
                assetCreator.SetPoolAssetId(streamingImagePool.GetAssetId());
                if (!assetCreator.End(streamingImageAsset))
                {
                    AZ_Error("StreamingImage", false, "Failed to initialize streaming image asset");
                    return nullptr;
                }
            }

            return StreamingImage::FindOrCreate(streamingImageAsset);
        }

        Data::Instance<StreamingImage> StreamingImage::CreateInternal(StreamingImageAsset& streamingImageAsset)
        {
            Data::Instance<StreamingImage> streamingImage = aznew StreamingImage();
            const RHI::ResultCode resultCode = streamingImage->Init(streamingImageAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return streamingImage;
            }

            return nullptr;
        }

        RHI::ResultCode StreamingImage::Init(StreamingImageAsset& imageAsset)
        {
            AZ_PROFILE_FUNCTION(RPI);

            Data::Instance<StreamingImagePool> pool;
            if (imageAsset.GetPoolAssetId().IsValid())
            {
                Data::Asset<RPI::StreamingImagePoolAsset> poolAsset(imageAsset.GetPoolAssetId(), AZ::AzTypeInfo<RPI::StreamingImagePoolAsset>::Uuid());
                pool = StreamingImagePool::FindOrCreate(poolAsset);
            }
            else
            {
                pool = ImageSystemInterface::Get()->GetStreamingPool();
            }

            if (!pool)
            {
                AZ_Error("StreamingImage", false, "Failed to acquire the streaming image pool instance.");
                return RHI::ResultCode::Fail;
            }

            // Cache off the RHI streaming image pool instance.
            RHI::StreamingImagePool* rhiPool = pool->GetRHIPool();

            /**
             * NOTE: The tail mip-chain is required to exist as a dependency of this asset. This allows
             * the image to initialize with well-defined content.
             */
            const uint16_t mipChainTailIndex = static_cast<uint16_t>(imageAsset.GetMipChainCount() - 1);

            RHI::ResultCode resultCode = RHI::ResultCode::Success;
            
            const ImageMipChainAsset& mipChainTailAsset = imageAsset.GetTailMipChain();

            {
                RHI::StreamingImageInitRequest initRequest;
                initRequest.m_image = GetRHIImage();
                initRequest.m_descriptor = imageAsset.GetImageDescriptor();
                initRequest.m_tailMipSlices = mipChainTailAsset.GetMipSlices();

                // NOTE: Initialization can fail due to out-of-memory errors. Need to handle it at runtime.
                resultCode = rhiPool->InitImage(initRequest);
            }

            if (resultCode == RHI::ResultCode::Success)
            {
                m_imageView = m_image->GetImageView(imageAsset.GetImageViewDescriptor());                
                if(!m_imageView.get())
                {
                   AZ_Error("Image", false, "Failed to initialize RHI image view. This is not a recoverable error and is likely a bug.");
                   return RHI::ResultCode::Fail;
                }
                
                // Build a local set of mip chain asset handles.
                for (size_t mipChainIndex = 0; mipChainIndex < imageAsset.GetMipChainCount(); ++mipChainIndex)
                {
                    const Data::AssetId& assetId = imageAsset.GetMipChainAsset(mipChainIndex).GetId();

                    // We want to store off the id, not the AssetData instance. This simplifies the fetch / evict logic which
                    // can do more strict assertions.
                    m_mipChains.push_back(Data::Asset<ImageMipChainAsset>(assetId, azrtti_typeid<ImageMipChainAsset>()));
                }

                // Initialize the streaming state to have the tail mip active and ready.
                m_state.m_residencyTarget = mipChainTailIndex;
                m_state.m_streamingTarget = mipChainTailIndex;

                // Setup masks for tail mip chain
                const uint16_t mipChainBit = static_cast<uint16_t>(1 << mipChainTailIndex);
                m_state.m_maskActive |= mipChainBit;
                m_state.m_maskEvictable &= ~mipChainBit;
                m_state.m_maskReady |= mipChainBit;

                // Take references on dependent assets
                m_imageAsset = { &imageAsset, AZ::Data::AssetLoadBehavior::PreLoad };
                m_rhiPool = rhiPool;
                m_pool = pool;
                m_pool->AttachImage(this);

                // Set rhi image name
                m_image->SetName(Name(m_imageAsset.GetHint()));
                        
#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                AZ_TracePrintf("StreamingImage", "Init image [%s]\n", m_image->GetName().data());
#endif
                                
#if defined (AZ_RPI_STREAMING_IMAGE_HOT_RELOADING)
                BusConnect(imageAsset.GetId());
#endif                                
                return RHI::ResultCode::Success;
            }

            AZ_Warning("StreamingImagePool", false, "Failed to initialize RHI::Image on RHI::StreamingImagePool.");
            return resultCode;
        }

        void StreamingImage::Shutdown()
        {
            if (IsInitialized())
            {
#if defined (AZ_RPI_STREAMING_IMAGE_HOT_RELOADING)
                Data::AssetBus::MultiHandler::BusDisconnect(GetAssetId());
#endif

                if(m_pool)
                {
                    m_pool->DetachImage(this);
                    m_pool = nullptr;
                }
                
                m_rhiPool = nullptr;

                GetRHIImage()->Shutdown();

                // Evict all active mip chains
                for (size_t mipChainIndex = 0; mipChainIndex < m_mipChains.size(); ++mipChainIndex)
                {
                    EvictMipChainAsset(mipChainIndex);
                }

                m_mipChains.clear();
                m_state = {};
            }
        }

        StreamingImage::~StreamingImage()
        {
            Shutdown(); 
        }

        void StreamingImage::SetTargetMip(uint16_t targetMipLevel)
        {
            if (m_streamingController)
            {
                m_streamingController->OnSetTargetMip(this, targetMipLevel);
            }
        }
        
        uint16_t StreamingImage::GetResidentMipLevel()
        {
            return static_cast<uint16_t>(m_image->GetResidentMipLevel());
        }

        Color StreamingImage::GetAverageColor() const
        {
            return m_imageAsset->GetAverageColor();
        }

        RHI::ResultCode StreamingImage::TrimToMipChainLevel(size_t mipChainIndex)
        {
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded number of mip chains.");

            const size_t mipChainBegin = m_state.m_streamingTarget;
            const size_t mipChainEnd = mipChainIndex;

            RHI::ResultCode resultCode = RHI::ResultCode::Success;

            // We only evict if the current target is higher detail than our requested target.
            if (mipChainBegin < mipChainEnd)
            {
                const uint32_t mipLevel = static_cast<uint32_t>(m_imageAsset->GetMipLevel(mipChainEnd));
                resultCode = m_rhiPool->TrimImage(*m_image.get(), mipLevel);

                // Start from the most detailed chain, evict all in-flight or loaded assets.
                // Note: this should only happened after TrimImage which all the possible backend asset data referencing were removed
                for (size_t chainIdx = mipChainBegin; chainIdx < mipChainEnd; ++chainIdx)
                {
                    EvictMipChainAsset(chainIdx);
                }
                
                // Reset tracked state to match the new target.
                m_state.m_residencyTarget = static_cast<uint16_t>(mipChainEnd);
                m_state.m_streamingTarget = static_cast<uint16_t>(mipChainEnd);
            }

            return resultCode;
        }

        void StreamingImage::QueueExpandToMipChainLevel(size_t mipChainIndex)
        {
            AZ_Assert(IsStreamable(), "Only streamable StreamingImage's mip chain can be expanded");
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded number of mip chains.");

            // Expand operation - queue streaming of mip chains up to target mip chain index. 
            if (m_state.m_streamingTarget > mipChainIndex)
            {
                // Start on the next-detailed chain from the streaming target.
                const size_t mipChainBegin = m_state.m_streamingTarget - 1;
                const size_t mipChainEnd = mipChainIndex - 1;

                // Iterate through to the end chain and queue loading operations on the mip assets.
                for (size_t i = mipChainBegin; i != mipChainEnd; --i)
                {
                    FetchMipChainAsset(i);
                }

                m_state.m_streamingTarget = static_cast<uint16_t>(mipChainIndex);
            }
        }
        
        void StreamingImage::QueueExpandToNextMipChainLevel()
        {
            // Return if already reach the end
            if (m_state.m_streamingTarget == 0)
            {
                return;
            }

            QueueExpandToMipChainLevel(m_state.m_streamingTarget - 1);
        }

        RHI::ResultCode StreamingImage::ExpandMipChain()
        {
            AZ_Assert(m_state.m_streamingTarget <= m_state.m_residencyTarget, "The target mip chain cannot be less detailed than the resident mip chain.")

            RHI::ResultCode resultCode = RHI::ResultCode::Success;

            if (m_state.m_streamingTarget < m_state.m_residencyTarget)
            {
#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                AZ_TracePrintf("StreamingImage", "Expand image [%s]\n", GetImage()->GetName().data());
#endif

                // Start by assuming we can expand residency to the full target range.
                uint16_t mipChainIndexFound = m_state.m_streamingTarget;

                // Walk the mip chains from most to least detailed, and track the latest instance
                // of an unloaded mip chain. This incrementally shortens the interval.
                for (uint16_t i = m_state.m_streamingTarget; i < m_state.m_residencyTarget; ++i)
                {
                    // Can't expand to this chain, select the next one as the candidate.
                    if (!IsMipChainAssetReady(i))
                    {
                        mipChainIndexFound = i + 1;
                    }
                }

                // If we found a range of loaded mip chains, upload them from the low level mipchain to high level mipchain
                // which the index should be from higher value to lower value
                if (mipChainIndexFound != m_state.m_residencyTarget)
                {
                    for (uint16_t mipChainIndex = m_state.m_residencyTarget-1; 
                        mipChainIndex >= mipChainIndexFound && resultCode == RHI::ResultCode::Success; 
                        mipChainIndex--)
                    {
                        resultCode = UploadMipChain(mipChainIndex);
                        if (mipChainIndex == 0)
                        {
                            break;
                        }
                    }
                    m_state.m_residencyTarget = mipChainIndexFound;
                }
            }

            return resultCode;
        }

        void StreamingImage::EvictMipChainAsset(size_t mipChainIndex)
        {
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded total number of mip chains.");

            const uint16_t mipChainBit = static_cast<uint16_t>(1 << mipChainIndex);

            const bool isMipChainActive = RHI::CheckBitsAll(m_state.m_maskActive, mipChainBit);
            const bool isMipChainEvictable = RHI::CheckBitsAll(m_state.m_maskEvictable, mipChainBit);

            if (isMipChainActive && isMipChainEvictable)
            {
                const uint32_t mipChainMask = ~mipChainBit;
                m_state.m_maskActive &= mipChainMask;
                m_state.m_maskReady  &= mipChainMask;

                Data::Asset<ImageMipChainAsset>& mipChainAsset = m_mipChains[mipChainIndex];
                AZ_Assert(mipChainAsset.GetStatus() != Data::AssetData::AssetStatus::NotLoaded, "Asset marked as active, but mipChainAsset in 'NotLoaded' state.");
                Data::AssetBus::MultiHandler::BusDisconnect(mipChainAsset.GetId());
                mipChainAsset.Release();
            }
        }

        void StreamingImage::FetchMipChainAsset(size_t mipChainIndex)
        {
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded total number of mip chains.");

            const uint16_t mipChainBit = static_cast<uint16_t>(1 << mipChainIndex);

            const bool isMipChainActive = RHI::CheckBitsAll(m_state.m_maskActive, mipChainBit);

            if (!isMipChainActive)
            {
                m_state.m_maskActive |= mipChainBit;

                Data::Asset<ImageMipChainAsset>& mipChainAsset = m_mipChains[mipChainIndex];
                AZ_Assert(mipChainAsset.Get() == nullptr, "Asset marked as inactive, but has a valid reference.");

                // Connect to the AssetBus so we are ready to receive OnAssetReady(), which will call OnMipChainAssetReady().
                // If the asset happens to already be loaded, OnAssetReady() will be called immediately.
                Data::AssetBus::MultiHandler::BusConnect(mipChainAsset.GetId());

                // And we request that the asset be loaded in case it isn't already.
                mipChainAsset.QueueLoad();

#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                AZ_TracePrintf("StreamingImage", "Fetch mip chain asset [%s]\n", mipChainAsset.GetHint().c_str());
#endif
            }
            else
            {
                AZ_Assert(false, "FetchMipChainAsset called for a mip chain that was already active.");
            }
        }

        bool StreamingImage::IsMipChainAssetReady(size_t mipChainIndex) const
        {
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded total number of mip chains.");

            return RHI::CheckBitsAny(m_state.m_maskReady, static_cast<uint16_t>(1 << mipChainIndex));
        }

        void StreamingImage::OnMipChainAssetReady(size_t mipChainIndex)
        {
            AZ_Assert(mipChainIndex < m_mipChains.size(), "Exceeded total number of mip chains.");

            const uint16_t mipChainBit = static_cast<uint16_t>(1 << mipChainIndex);

            AZ_Assert(RHI::CheckBitsAll(m_state.m_maskActive, mipChainBit), "Mip chain should be marked as active.");

            m_state.m_maskReady |= mipChainBit;

            if (m_streamingController)
            {
                m_streamingController->OnMipChainAssetReady(this);
            }
        }
                
        RHI::ResultCode StreamingImage::UploadMipChain(size_t mipChainIndex)
        {
            if (const Data::Asset<ImageMipChainAsset>& mipChainAsset = m_mipChains[mipChainIndex])
            {
                const auto& mipSlices = mipChainAsset->GetMipSlices();

                RHI::StreamingImageExpandRequest request;
                request.m_image = GetRHIImage();
                request.m_mipSlices = mipSlices;

                request.m_completeCallback = [=]()
                {
#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                    AZ_TracePrintf("StreamingImage", "Upload mipchain done [%s]\n", mipChainAsset.GetHint().c_str());
#endif
                    EvictMipChainAsset(mipChainIndex); 
                };

#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                AZ_TracePrintf("StreamingImage", "Start Upload mipchain [%d] [%s] start [%d], resident [%d]\n", mipChainIndex,
                    mipChainAsset.GetHint().c_str(), m_image->GetResidentMipLevel());
#endif

                return m_rhiPool->ExpandImage(request);
            }
            return RHI::ResultCode::InvalidOperation;
        }

        void StreamingImage::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            size_t mipChainIndex = 0;

            const size_t mipChainCount = m_mipChains.size();

            for (; mipChainIndex < mipChainCount; ++mipChainIndex)
            {
                if (m_mipChains[mipChainIndex] == asset)
                {
#ifdef AZ_RPI_STREAMING_IMAGE_DEBUG_LOG
                    AZ_TracePrintf("StreamingImage", "mip chain asset ready [%s]\n", asset.GetHint().c_str());
#endif
                    OnMipChainAssetReady(mipChainIndex);
                    break;
                }
            }
        }

        void StreamingImage::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
#if defined (AZ_RPI_STREAMING_IMAGE_HOT_RELOADING)
            if (asset.GetId() == GetAssetId())
            {
                StreamingImageAsset* imageAsset = azrtti_cast<StreamingImageAsset*>(asset.GetData());

                // Release the loaded mipchain assets from both current asset and new asset since they are coming from old asset
                // This is due to we have to use PreLoad dependecy load behavior for streaming image asset
                // before we can switch load behavior at runtime.
                // [GFX TODO] [ATOM-14467] Remove unnecessary code in StreamingImage::OnAssetReloaded when runtime switching dependency load behavior is supported
                m_imageAsset->ReleaseMipChainAssets();
                imageAsset->ReleaseMipChainAssets();

                // Re-initialize the image.
                Shutdown();
                [[maybe_unused]] RHI::ResultCode resultCode = Init(*imageAsset);

                AZ_Assert(resultCode == RHI::ResultCode::Success, "Failed to re-initialize streaming image");
            }
            else
            {
                AZ_Assert(false, "The mip chain asset auto-reload was disabled. If you are sure you want to reload mip chain manually you can remove this assert");
            }
#endif
        }

        const Data::Instance<StreamingImagePool>& StreamingImage::GetPool() const
        {
            return m_pool;
        }
        
        bool StreamingImage::IsStreamable() const
        {
            return (RHI::CheckBitsAny(m_imageAsset->GetFlags(), StreamingImageFlags::NotStreamable) == false);
        }

    }
}
