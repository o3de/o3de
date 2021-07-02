/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/SwapChain.h>
#include <RHI/Device.h>
#include <RHI/Conversions.h>
#include <RHI/Image.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<SwapChain> SwapChain::Create()
        {
            return aznew SwapChain();
        }

        Device& SwapChain::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        RHI::ResultCode SwapChain::InitImageInternal(const InitImageRequest& request)
        {

            Device& device = GetDevice();

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            DX12::AssertSuccess(m_swapChain->GetBuffer(request.m_imageIndex, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            device.GetImageAllocationInfo(request.m_descriptor, allocationInfo);

            Name name(AZStd::string::format("SwapChainImage_%d", request.m_imageIndex));

            Image& image = static_cast<Image&>(*request.m_image);
            image.m_memoryView = MemoryView(resource.Get(), 0, allocationInfo.SizeInBytes, allocationInfo.Alignment, MemoryViewType::Image);
            image.SetName(name);
            image.GenerateSubresourceLayouts();
            // Overwrite m_initialAttachmentState because Swapchain images are created with D3D12_RESOURCE_STATE_COMMON state
            image.SetAttachmentState(D3D12_RESOURCE_STATE_COMMON);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_reservedInBytes += allocationInfo.SizeInBytes;
            memoryUsage.m_residentInBytes += allocationInfo.SizeInBytes;

            return RHI::ResultCode::Success;
        }

        void SwapChain::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            Image& image = static_cast<Image&>(resourceBase);

            const size_t sizeInBytes = image.GetMemoryView().GetSize();

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_reservedInBytes -= sizeInBytes;
            memoryUsage.m_residentInBytes -= sizeInBytes;

            GetDevice().QueueForRelease(image.m_memoryView);

            image.m_memoryView = {};
        }
    }
}
