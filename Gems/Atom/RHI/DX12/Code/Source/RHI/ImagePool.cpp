/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/ImagePool.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
namespace AZ
{
    namespace DX12
    {
        class ImagePoolResolver
            : public ResourcePoolResolver
        {
        public:
            AZ_CLASS_ALLOCATOR(ImagePoolResolver, AZ::SystemAllocator);
            AZ_RTTI(ImagePoolResolver, "{305EFAFB-9319-4AB7-99DD-0AA361C22CED}", ResourcePoolResolver);

            ImagePoolResolver(Device& device, ImagePool* imagePool)
                : m_device{&device}
                , m_pool{imagePool}
            {}

            ImagePool* m_pool = nullptr;

            RHI::ResultCode UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred)
            {
                AZ_PROFILE_FUNCTION(RHI);

                AZStd::lock_guard<AZStd::mutex> lock(m_imagePacketMutex);

                Image* image = static_cast<Image*>(request.m_image);
                Memory* imageMemory = image->GetMemoryView().GetMemory();

                // Allocate an entry from the Image packets vector. Keep the set unique.
                {
                    size_t imagePacketIndex = 0;
                    for (; imagePacketIndex < m_imagePackets.size(); ++imagePacketIndex)
                    {
                        if (m_imagePackets[imagePacketIndex].m_image == image)
                        {
                            break;
                        }
                    }

                    if (imagePacketIndex == m_imagePackets.size())
                    {
                        m_imagePackets.emplace_back();

                        ImagePacket& imagePacket = m_imagePackets.back();
                        imagePacket.m_image = image;
                        imagePacket.m_imageMemory = image->GetMemoryView().GetMemory();
                    }
                }

                // Build a subresource packet which contains the staging data and target image location to copy into.
                const RHI::ImageDescriptor& imageDescriptor = image->GetDescriptor();
                const RHI::DeviceImageSubresourceLayout& sourceSubresourceLayout = request.m_sourceSubresourceLayout;
                const uint32_t stagingRowPitch = RHI::AlignUp(sourceSubresourceLayout.m_bytesPerRow, DX12_TEXTURE_DATA_PITCH_ALIGNMENT);
                const uint32_t stagingSlicePitch = stagingRowPitch * sourceSubresourceLayout.m_rowCount;

                MemoryView stagingMemory = m_device->AcquireStagingMemory(stagingSlicePitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT stagingFootprint;
                stagingFootprint.Offset = stagingMemory.GetOffset();
                stagingFootprint.Footprint.Width = sourceSubresourceLayout.m_size.m_width;
                stagingFootprint.Footprint.Height = sourceSubresourceLayout.m_size.m_height;
                stagingFootprint.Footprint.Depth = sourceSubresourceLayout.m_size.m_depth;
                stagingFootprint.Footprint.Format = GetBaseFormat(ConvertFormat(imageDescriptor.m_format));
                stagingFootprint.Footprint.RowPitch = stagingRowPitch;

                m_imageSubresourcePackets.emplace_back();

                ImageSubresourcePacket& imageSubresourcePacket = m_imageSubresourcePackets.back();

                // Copy to the requested image subresource with the requested pixel offset.
                imageSubresourcePacket.m_imageLocation = CD3DX12_TEXTURE_COPY_LOCATION(imageMemory, RHI::GetImageSubresourceIndex(request.m_imageSubresource, imageDescriptor.m_mipLevels));
                imageSubresourcePacket.m_imageSubresourcePixelOffset = request.m_imageSubresourcePixelOffset;

                // Copy from the staging data using the allocated staging memory and the computed footprint.
                imageSubresourcePacket.m_stagingLocation = CD3DX12_TEXTURE_COPY_LOCATION(stagingMemory.GetMemory(), stagingFootprint);
                
                // Copy CPU data into the staging memory.
                {
                    CpuVirtualAddress stagingMemoryPtr = stagingMemory.Map(RHI::HostMemoryAccess::Write);

                    D3D12_MEMCPY_DEST destData;
                    destData.pData = stagingMemoryPtr;
                    destData.RowPitch = stagingRowPitch;
                    destData.SlicePitch = stagingSlicePitch;

                    D3D12_SUBRESOURCE_DATA srcData;
                    srcData.pData = request.m_sourceData;
                    srcData.RowPitch = sourceSubresourceLayout.m_bytesPerRow;
                    srcData.SlicePitch = sourceSubresourceLayout.m_bytesPerImage;

                    MemcpySubresource(&destData, &srcData, sourceSubresourceLayout.m_bytesPerRow, sourceSubresourceLayout.m_rowCount, sourceSubresourceLayout.m_size.m_depth);

                    stagingMemory.Unmap(RHI::HostMemoryAccess::Write);
                }

                image->m_pendingResolves++;
                bytesTransferred = stagingMemory.GetSize();
                return RHI::ResultCode::Success;
            }

            void Compile(Scope& scope) override
            {
                AZ_UNUSED(scope);
                m_prologueBarriers.clear();
                m_epilogueBarriers.clear();

                // Compile the resource barriers and set the final resource states.
                for (const ImagePacket& imagePacket : m_imagePackets)
                {
                    Image& image = *imagePacket.m_image;

                    D3D12_RESOURCE_TRANSITION_BARRIER transition;
                    transition.pResource = imagePacket.m_imageMemory;
                    for (const auto& subresourceState : image.GetAttachmentStateByIndex())
                    {
                        transition.StateBefore = subresourceState.m_state;
                        transition.Subresource = subresourceState.m_subresourceIndex;
                        transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                        m_prologueBarriers.push_back(transition);

                        if (!image.IsAttachment())
                        {
                            // Convert back to previous state.
                            transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                            transition.StateAfter = subresourceState.m_state;
                            m_epilogueBarriers.push_back(transition);
                        }
                    }

                    if (image.IsAttachment())
                    {
                        // Leave attachment in copy write state. The graph will take ownership.
                        image.SetAttachmentState(D3D12_RESOURCE_STATE_COPY_DEST);
                    }
                }
            }

            void QueuePrologueTransitionBarriers(CommandList& commandList) override
            {
                for (const auto& barrier : m_prologueBarriers)
                {
                    commandList.QueueTransitionBarrier(barrier);
                }
            }

            void Resolve(CommandList& commandList) const override
            {
                for (const ImageSubresourcePacket& imageSubresourcePacket : m_imageSubresourcePackets)
                {
                    commandList.GetCommandList()->CopyTextureRegion(
                        &imageSubresourcePacket.m_imageLocation,
                        imageSubresourcePacket.m_imageSubresourcePixelOffset.m_left,
                        imageSubresourcePacket.m_imageSubresourcePixelOffset.m_top,
                        imageSubresourcePacket.m_imageSubresourcePixelOffset.m_front,
                        &imageSubresourcePacket.m_stagingLocation,
                        nullptr);
                }
            }

            void QueueEpilogueTransitionBarriers(CommandList& commandList) const
            {
                for (const auto& barrier : m_epilogueBarriers)
                {
                    commandList.QueueTransitionBarrier(barrier);
                }
            }

            void Deactivate() override
            {
                AZStd::for_each(m_imagePackets.begin(), m_imagePackets.end(), [](auto& packet)
                {
                    AZ_Assert(packet.m_image->m_pendingResolves, "There's no pending resolves for image %s", packet.m_image->GetName().GetCStr());
                    packet.m_image->m_pendingResolves--;
                });

                m_imagePackets.clear();
                m_imageSubresourcePackets.clear();
            }

            template<class T, class Predicate>
            void EraseResourceFromList(AZStd::vector<T>& list, const Predicate& predicate)
            {
                list.erase(AZStd::remove_if(list.begin(), list.end(), predicate), list.end());
            }

            void OnResourceShutdown(const RHI::DeviceResource& resource) override
            {
                const Image& image = static_cast<const Image&>(resource);
                if (!image.m_pendingResolves)
                {
                    return;
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_imagePacketMutex);
                auto predicatePackets = [&image](const ImagePacket& packet)
                {
                    return packet.m_image == &image;
                };

                auto predicateSubresourcesPackets = [&image](const ImageSubresourcePacket& packet)
                {
                    return packet.m_imageLocation.pResource == image.GetMemoryView().GetMemory();
                };

                auto predicateBarriers = [&image](const D3D12_RESOURCE_TRANSITION_BARRIER& barrier)
                {
                    return barrier.pResource == image.GetMemoryView().GetMemory();
                };

                EraseResourceFromList(m_imagePackets, predicatePackets);
                EraseResourceFromList(m_imageSubresourcePackets, predicateSubresourcesPackets);
                EraseResourceFromList(m_prologueBarriers, predicateBarriers);
                EraseResourceFromList(m_epilogueBarriers, predicateBarriers);
            }

        private:
            struct ImagePacket
            {
                Image* m_image;
                Memory* m_imageMemory;
            };

            struct ImageSubresourcePacket
            {
                RHI::Origin m_imageSubresourcePixelOffset;
                D3D12_TEXTURE_COPY_LOCATION m_imageLocation;
                D3D12_TEXTURE_COPY_LOCATION m_stagingLocation;
            };

            Device* m_device = nullptr;

            AZStd::mutex m_imagePacketMutex;
            AZStd::vector<ImagePacket> m_imagePackets;
            AZStd::vector<ImageSubresourcePacket> m_imageSubresourcePackets;
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_prologueBarriers;
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_epilogueBarriers;
        };

        RHI::Ptr<ImagePool> ImagePool::Create()
        {
            return aznew ImagePool();
        }

        Device& ImagePool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        ImagePoolResolver* ImagePool::GetResolver()
        {
            return static_cast<ImagePoolResolver*>(Base::GetResolver());
        }

        RHI::ResultCode ImagePool::InitInternal(RHI::Device& device, const RHI::ImagePoolDescriptor&)
        {
            SetResolver(AZStd::make_unique<ImagePoolResolver>(static_cast<Device&>(device), this));
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode ImagePool::InitImageInternal(const RHI::DeviceImageInitRequest& request)
        {
            Device& device = GetDevice();

            Image* image = static_cast<Image*>(request.m_image);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            device.GetImageAllocationInfo(request.m_descriptor, allocationInfo);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            if (!memoryUsage.CanAllocate(allocationInfo.SizeInBytes))
            {
                return RHI::ResultCode::OutOfMemory;
            }

            /**
             * Super simple implementation. Just creates a committed resource for the image. No
             * real pooling happening at the moment. Other approaches might involve creating dedicated
             * heaps and then placing resources onto those heaps. This will allow us to control residency
             * at the heap level.
             */

            MemoryView memoryView = device.CreateImageCommitted(
                request.m_descriptor, request.m_optimizedClearValue, image->GetInitialResourceState(), D3D12_HEAP_TYPE_DEFAULT);

            if (memoryView.IsValid())
            {
                image->m_residentSizeInBytes = memoryView.GetSize();
                image->m_memoryView = AZStd::move(memoryView);
                image->GenerateSubresourceLayouts();
                image->m_memoryView.SetName(image->GetName().GetStringView());
                image->m_streamedMipLevel = image->GetResidentMipLevel();

                memoryUsage.m_totalResidentInBytes += allocationInfo.SizeInBytes;
                memoryUsage.m_usedResidentInBytes += allocationInfo.SizeInBytes;
                return RHI::ResultCode::Success;
            }
            else
            {
                return RHI::ResultCode::OutOfMemory;
            }
        }

        RHI::ResultCode ImagePool::UpdateImageContentsInternal(const RHI::DeviceImageUpdateRequest& request)
        {
            size_t bytesTransferred = 0;
            RHI::ResultCode resultCode = GetResolver()->UpdateImage(request, bytesTransferred);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_memoryUsage.m_transferPull.m_bytesPerFrame += bytesTransferred;
            }
            return resultCode;
        }

        void ImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }

            Image& image = static_cast<Image&>(resourceBase);
            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_totalResidentInBytes -= image.m_residentSizeInBytes;
            memoryUsage.m_usedResidentInBytes -= image.m_residentSizeInBytes;

            GetDevice().QueueForRelease(image.m_memoryView);
            image.m_memoryView = {};
            image.m_pendingResolves = 0;
        }
    }
}
