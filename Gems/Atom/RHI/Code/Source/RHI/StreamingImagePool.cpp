/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/StreamingImagePool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::RHI
{
    bool StreamingImagePool::ValidateInitRequest(const StreamingImageInitRequest& initRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (initRequest.m_tailMipSlices.empty())
            {
                AZ_Error(
                    "StreamingImagePool",
                    false,
                    "No tail mip slices were provided. You must provide at least one tail mip slice.");
                return false;
            }

            if (initRequest.m_tailMipSlices.size() > initRequest.m_descriptor.m_mipLevels)
            {
                AZ_Error("StreamingImagePool", false, "Tail mip array exceeds the number of mip levels in the image.");
                return false;
            }

            // Streaming images are only allowed to update via the CPU.
            if (CheckBitsAny(
                    initRequest.m_descriptor.m_bindFlags,
                    ImageBindFlags::Color | ImageBindFlags::DepthStencil | ImageBindFlags::ShaderWrite))
            {
                AZ_Error("StreamingImagePool", false, "Streaming images may only contain read-only bind flags.");
                return false;
            }
        }

        AZ_UNUSED(initRequest);
        return true;
    }

    bool StreamingImagePool::ValidateExpandRequest(const StreamingImageExpandRequest& expandRequest) const
    {
        if (Validation::IsEnabled())
        {
            if (!ValidateIsRegistered(expandRequest.m_image.get()))
            {
                return false;
            }
        }

        AZ_UNUSED(expandRequest);
        return true;
    }

    ResultCode StreamingImagePool::Init(const StreamingImagePoolDescriptor& descriptor)
    {
        AZ_PROFILE_FUNCTION(RHI);

        return ResourcePool::Init(
            descriptor.m_deviceMask,
            [this, &descriptor]()
            {
                // Assign the descriptor prior to initialization. Technically, the descriptor is undefined
                // for uninitialized pools, so it's okay if initialization fails. Doing this removes the
                // possibility that users will get garbage values from GetDescriptor().
                m_descriptor = descriptor;

                ResultCode result = ResultCode::Success;

                IterateDevices(
                    [this, &descriptor, &result](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateStreamingImagePool();

                        result = GetDeviceStreamingImagePool(deviceIndex)->Init(*device, descriptor);

                        return result == ResultCode::Success;
                    });

                if (result != ResultCode::Success)
                {
                    // Reset already initialized device-specific DeviceStreamingImagePool and set deviceMask to 0
                    m_deviceObjects.clear();
                    MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
                }

                return ResultCode::Success;
            });
    }

    ResultCode StreamingImagePool::InitImage(const StreamingImageInitRequest& initRequest)
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateInitRequest(initRequest))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = ImagePoolBase::InitImage(
            initRequest.m_image,
            initRequest.m_descriptor,
            [this, &initRequest]()
            {
                initRequest.m_image->Init(GetDeviceMask() & initRequest.m_deviceMask);

                ResultCode result = IterateObjects<DeviceStreamingImagePool>(
                    [&initRequest](auto deviceIndex, auto deviceStreamingImagePool)
                    {
                        if (CheckBit(initRequest.m_image->GetDeviceMask(), deviceIndex))
                        {
                            if (!initRequest.m_image->m_deviceObjects.contains(deviceIndex))
                            {
                                initRequest.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();
                            }

                            DeviceStreamingImageInitRequest streamingImageInitRequest(
                                initRequest.m_image->GetDeviceImage(deviceIndex), initRequest.m_descriptor, initRequest.m_tailMipSlices);
                            return deviceStreamingImagePool->InitImage(streamingImageInitRequest);
                        }
                        else
                        {
                            initRequest.m_image->m_deviceObjects.erase(deviceIndex);
                        }

                        return ResultCode::Success;
                    });

                if (result != ResultCode::Success)
                {
                    // Reset already initialized device-specific StreamingImagePools
                    m_deviceObjects.clear();
                }

                return result;
            });

        AZ_Warning("StreamingImagePool", resultCode == ResultCode::Success, "Failed to initialize image.");
        return resultCode;
    }

    ResultCode StreamingImagePool::UpdateImageDeviceMask(const StreamingImageDeviceMaskRequest& request)
    {
        return IterateObjects<DeviceStreamingImagePool>(
            [&request](auto deviceIndex, auto deviceStreamingImagePool)
            {
                if (CheckBit(request.m_deviceMask, deviceIndex))
                {
                    if (!request.m_image->m_deviceObjects.contains(deviceIndex))
                    {
                        request.m_image->m_deviceObjects[deviceIndex] = Factory::Get().CreateImage();

                        DeviceStreamingImageInitRequest imageInitRequest(
                            request.m_image->GetDeviceImage(deviceIndex), request.m_image->m_descriptor, request.m_tailMipSlices);
                        auto result = deviceStreamingImagePool->InitImage(imageInitRequest);

                        if (result == ResultCode::Success)
                        {
                            request.m_image->Init(SetBit(request.m_image->GetDeviceMask(), deviceIndex));
                        }

                        return result;
                    }
                }
                else
                {
                    request.m_image->Init(ResetBit(request.m_image->GetDeviceMask(), deviceIndex));
                    request.m_image->m_deviceObjects.erase(deviceIndex);
                }

                return ResultCode::Success;
            });
    }

    ResultCode StreamingImagePool::ExpandImage(const StreamingImageExpandRequest& request)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateExpandRequest(request))
        {
            return ResultCode::InvalidArgument;
        }

        auto expandCount = AZStd::make_shared<AZStd::atomic_int>(static_cast<int>(m_deviceObjects.size()));

        auto completeCallback = [expandCount, request]()
        {
            if (--*expandCount == 0)
            {
                request.m_completeCallback();
            }
        };

        return IterateObjects<DeviceStreamingImagePool>([&request, &completeCallback](auto deviceIndex, auto deviceStreamingImagePool)
        {
            DeviceStreamingImageExpandRequest expandRequest;

            expandRequest.m_image = request.m_image->GetDeviceImage(deviceIndex);
            expandRequest.m_mipSlices = request.m_mipSlices;
            expandRequest.m_waitForUpload = request.m_waitForUpload;
            expandRequest.m_completeCallback = completeCallback;

            return deviceStreamingImagePool->ExpandImage(expandRequest);
        });
    }

    ResultCode StreamingImagePool::TrimImage(Image& image, uint32_t targetMipLevel)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsRegistered(&image))
        {
            return ResultCode::InvalidArgument;
        }

        ResultCode resultCode = IterateObjects<DeviceStreamingImagePool>([&image, targetMipLevel](auto deviceIndex, auto deviceStreamingImagePool)
        {
            return deviceStreamingImagePool->TrimImage(*image.GetDeviceImage(deviceIndex), targetMipLevel);
        });

        if (resultCode == ResultCode::Success)
        {
            // If initialization succeeded, assign the new resident mip level. Invalidate resource views
            // so that they no longer reference trimmed mip levels.
            image.InvalidateViews();
        }

        return resultCode;
    }

    const StreamingImagePoolDescriptor& StreamingImagePool::GetDescriptor() const
    {
        return m_descriptor;
    }

    bool StreamingImagePool::SetMemoryBudget(size_t newBudget)
    {
        bool success{true};
        IterateObjects<DeviceStreamingImagePool>([&success, newBudget]([[maybe_unused]] auto deviceIndex, auto deviceStreamingImagePool)
        {
            success &= deviceStreamingImagePool->SetMemoryBudget(newBudget);
        });
        return success;
    }

    const HeapMemoryUsage& StreamingImagePool::GetHeapMemoryUsage(HeapMemoryLevel heapMemoryLevel) const
    {
        auto maxUsageIndex{RHI::MultiDevice::DefaultDeviceIndex};
        auto maxBudget{0ull};
        IterateObjects<DeviceStreamingImagePool>([&maxUsageIndex, &maxBudget, heapMemoryLevel](auto deviceIndex, auto deviceStreamingImagePool)
        {
            const auto& deviceHeapMemoryUsage{deviceStreamingImagePool->GetHeapMemoryUsage(heapMemoryLevel)};
            if(deviceHeapMemoryUsage.m_budgetInBytes > maxBudget)
            {
                maxBudget = deviceHeapMemoryUsage.m_budgetInBytes;
                maxUsageIndex = deviceIndex;
            }
        });

        return GetDeviceStreamingImagePool(maxUsageIndex)->GetHeapMemoryUsage(heapMemoryLevel);
    }

    void StreamingImagePool::SetLowMemoryCallback(LowMemoryCallback callback)
    {
        IterateObjects<DeviceStreamingImagePool>([&callback]([[maybe_unused]] auto deviceIndex, auto deviceStreamingImagePool)
        {
            deviceStreamingImagePool->SetLowMemoryCallback(callback);
        });
    }

    bool StreamingImagePool::SupportTiledImage() const
    {
        bool supportsTiledImage{true};
        IterateObjects<DeviceStreamingImagePool>([&supportsTiledImage]([[maybe_unused]] auto deviceIndex, auto deviceStreamingImagePool)
        {
            supportsTiledImage &= deviceStreamingImagePool->SupportTiledImage();
        });
        return supportsTiledImage;
    }

    void StreamingImagePool::Shutdown()
    {
        ResourcePool::Shutdown();
    }
} // namespace AZ::RHI
