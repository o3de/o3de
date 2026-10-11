/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/SwapChain_Windows.h>

#include <Atom/RHI.Interface/DX12/SwapChainProxy.h>
#include <RHI/Device.h>
#include <RHI/Conversions.h>
#include <RHI/Image.h>
#include <RHI/NsightAftermath.h>

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
            return static_cast<Device&>(RHI::DeviceSwapChain::GetDevice());
        }

        RHI::ResultCode SwapChain::InitInternal(RHI::Device& deviceBase, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions)
        {
            // Check whether tearing support is available for full screen borderless windowed mode.
            Device& device = static_cast<Device&>(deviceBase);
            BOOL allowTearing = FALSE;
            DX12Ptr<IDXGIFactoryX> dxgiFactory;
            device.AssertSuccess(CreateDXGIFactory2(0, IID_GRAPHICS_PPV_ARGS(dxgiFactory.GetAddressOf())));
            m_isTearingSupported = SUCCEEDED(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))) && allowTearing;

            if (nativeDimensions)
            {
                *nativeDimensions = descriptor.m_dimensions;
            }
            const uint32_t SwapBufferCount = AZStd::max(RHI::Limits::Device::MinSwapChainImages, RHI::Limits::Device::FrameCountMax);

            DXGI_SWAP_CHAIN_DESCX swapChainDesc = {};
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.BufferCount = SwapBufferCount;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.Width = descriptor.m_dimensions.m_imageWidth;
            swapChainDesc.Height = descriptor.m_dimensions.m_imageHeight;
            swapChainDesc.Format = ConvertFormat(descriptor.m_dimensions.m_imageFormat);
            swapChainDesc.Scaling = ConvertScaling(descriptor.m_scalingMode);
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            if (m_isTearingSupported)
            {
                // It is recommended to always use the tearing flag when it is available.
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            IUnknown* window = reinterpret_cast<IUnknown*>(descriptor.m_window.GetIndex());
            RHI::ResultCode result = device.CreateSwapChain(reinterpret_cast<IUnknown*>(descriptor.m_window.GetIndex()), swapChainDesc, m_swapChain);
            if (result == RHI::ResultCode::Success)
            {
                TryReplaceWithProxy(device);

                ConfigureDisplayMode(*nativeDimensions);

                // According to various docs (and the D3D12Fulscreen sample), when tearing is supported
                // a borderless full screen window is always preferred over exclusive full screen mode.
                //
                // - https://devblogs.microsoft.com/directx/demystifying-full-screen-optimizations/
                // - https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
                //
                // So we have modelled our full screen support on the D3D12Fulscreen sample by choosing
                // the best full screen mode to use based on whether tearing is supported by the device.
                //
                // It would be possible to allow a choice between these different full screen modes,
                // but we have chosen not to given that guidance for DX12 appears to be discouraging
                // the use of exclusive full screen mode, and because no other platforms support it.
                if (m_isTearingSupported)
                {
                    // To use tearing in full screen Win32 apps the application should present to a fullscreen borderless window and disable automatic
                    // ALT+ENTER fullscreen switching using IDXGIFactory::MakeWindowAssociation (see also implementation of SwapChain::PresentInternal).
                    // You must call the MakeWindowAssociation method after the creation of the swap chain, and on the factory object associated with the
                    // target HWND swap chain, which you can guarantee by calling the IDXGIObject::GetParent method on the swap chain to locate the factory.
                    IDXGIFactoryX* parentFactory = nullptr;
                    m_swapChain->GetParent(__uuidof(IDXGIFactoryX), (void **)&parentFactory);
                    device.AssertSuccess(parentFactory->MakeWindowAssociation(reinterpret_cast<HWND>(window), DXGI_MWA_NO_ALT_ENTER));
                }
            }
            return result;
        }

        void SwapChain::ShutdownInternal()
        {
            // Let a registered proxy (frame-generation swapchain) flush outstanding presents and
            // drop its handle before we release ours -- it may still own GPU work on this chain.
            if (m_replacedByProxy)
            {
                if (auto* proxy = SwapChainProxyInterface::Get())
                {
                    proxy->OnSwapChainDestroyed(m_swapChain.get());
                }
                m_replacedByProxy = false;
            }

            // We must exit exclusive full screen mode before shutting down.
            // Safe to call even if not in the exclusive full screen state.
            m_swapChain->SetFullscreenState(0, nullptr);
            m_swapChain = nullptr;
        }

        void SwapChain::TryReplaceWithProxy(Device& device)
        {
            auto* proxy = SwapChainProxyInterface::Get();
            if (!proxy || !m_swapChain)
            {
                return;
            }

            ID3D12CommandQueue* presentQueue =
                device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetPlatformQueue();
            if (!presentQueue)
            {
                return;
            }

            // Hand the proxy exactly one reference, per the ISwapChainProxy contract, and take one
            // back. The local raw pointer holds that transferred reference across the call so the
            // chain cannot be destroyed underneath us while m_swapChain is momentarily empty.
            IDXGISwapChainX* transferred = m_swapChain.get();
            transferred->AddRef();
            m_swapChain = nullptr;

            IDXGISwapChain4* replacement = transferred;
            const bool replaced = proxy->ReplaceSwapChain(presentQueue, replacement);
            if (!replaced || !replacement)
            {
                // Contract says a declining proxy leaves the pointer and its reference untouched.
                replacement = transferred;
            }

            m_swapChain = replacement;  // intrusive_ptr adds its own reference...
            replacement->Release();     // ...so give up the transferred one.

            m_replacedByProxy = replaced && (replacement != transferred);
            AZ_TracePrintf(
                "DX12",
                "SwapChain: proxy %s the DXGI swapchain.",
                m_replacedByProxy ? "replaced" : "declined to replace");
        }

        uint32_t SwapChain::PresentInternal()
        {
            if (m_swapChain)
            {
                // This is a flip model swapchain (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL). Present(0, 0) on a
                // flip model chain does NOT run unlocked - DXGI still waits to flip at vblank, it just
                // discards the frames queued in between. The frame rate stays pinned to the display's
                // refresh rate no matter what the vsync interval is set to. Passing
                // DXGI_PRESENT_ALLOW_TEARING is the only way to actually present unlocked, and the
                // swapchain is already created with DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING to permit it
                // (see InitInternal).
                //
                // The flag is only legal with a sync interval of 0 - passing it alongside a non-zero
                // interval fails the Present with DXGI_ERROR_INVALID_CALL. It also cannot be used while
                // in full screen exclusive mode, entered via SetFullscreenState(TRUE); full screen Win32
                // apps wanting tearing should use a borderless window with automatic ALT+ENTER switching
                // disabled through IDXGIFactory::MakeWindowAssociation (see InitInternal).
                const uint32_t verticalSyncInterval = GetDescriptor().m_verticalSyncInterval;
                const UINT presentFlags =
                    (verticalSyncInterval == 0 && m_isTearingSupported && !m_isInFullScreenExclusiveState)
                    ? DXGI_PRESENT_ALLOW_TEARING
                    : 0;

                HRESULT hresult = m_swapChain->Present(verticalSyncInterval, presentFlags);

                if (!GetDevice().AssertSuccess(hresult))
                {
                    // A failed Present (e.g. DXGI_ERROR_DEVICE_REMOVED) may have already run
                    // Device::OnDeviceRemoved()'s cleanup by this point, which can invalidate swapchain
                    // image state. Don't touch GetImageCount()/advance the index on top of that -- matches
                    // ResizeInternal()'s handling of the same AssertSuccess() failure case below.
                    return GetCurrentImageIndex();
                }

                return (GetCurrentImageIndex() + 1) % GetImageCount();
            }

            return GetCurrentImageIndex();
        }

        RHI::ResultCode SwapChain::InitImageInternal(const InitImageRequest& request)
        {
            Device& device = GetDevice();

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            GetDevice().AssertSuccess(m_swapChain->GetBuffer(request.m_imageIndex, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf())));

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
            memoryUsage.m_totalResidentInBytes += allocationInfo.SizeInBytes;
            memoryUsage.m_usedResidentInBytes += allocationInfo.SizeInBytes;

            return RHI::ResultCode::Success;
        }

        void SwapChain::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            Image& image = static_cast<Image&>(resourceBase);

            const size_t sizeInBytes = image.GetMemoryView().GetSize();

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_totalResidentInBytes -= sizeInBytes;
            memoryUsage.m_usedResidentInBytes -= sizeInBytes;

            GetDevice().QueueForRelease(image.m_memoryView);

            image.m_memoryView = {};
        }

        RHI::ResultCode SwapChain::ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions)
        {
            GetDevice().WaitForIdle();

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
             m_swapChain->GetDesc(&swapChainDesc);
            if (AssertSuccess(m_swapChain->ResizeBuffers(
                    dimensions.m_imageCount,
                    dimensions.m_imageWidth,
                    dimensions.m_imageHeight,
                    swapChainDesc.BufferDesc.Format,
                    swapChainDesc.Flags)))
            {
                if (nativeDimensions)
                {
                    *nativeDimensions = dimensions;
                }
                ConfigureDisplayMode(dimensions);

                // Check whether SetFullscreenState was used to enter full screen exclusive state.
                BOOL fullscreenState;
                m_isInFullScreenExclusiveState = SUCCEEDED(m_swapChain->GetFullscreenState(&fullscreenState, nullptr)) && fullscreenState;

                return RHI::ResultCode::Success;
            }

            return RHI::ResultCode::Fail;
        }

        bool SwapChain::IsExclusiveFullScreenPreferred() const
        {
            return !m_isTearingSupported;
        }

        bool SwapChain::GetExclusiveFullScreenState() const
        {
            return m_isInFullScreenExclusiveState;
        }

        bool SwapChain::SetExclusiveFullScreenState(bool fullScreenState)
        {
            if (m_swapChain)
            {
                m_swapChain->SetFullscreenState(fullScreenState, nullptr);
            }

            // The above call to SetFullscreenState will ultimately result in
            // SwapChain:ResizeInternal being called above where we set this.
            return (fullScreenState == m_isInFullScreenExclusiveState);
        }

        void SwapChain::ConfigureDisplayMode(const RHI::SwapChainDimensions& dimensions)
        {
            DXGI_COLOR_SPACE_TYPE colorSpace = static_cast<DXGI_COLOR_SPACE_TYPE>(InvalidColorSpace);
            bool hdrEnabled = false;

            if (dimensions.m_imageFormat == RHI::Format::R8G8B8A8_UNORM)
            {
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
            }
            else if (dimensions.m_imageFormat == RHI::Format::R10G10B10A2_UNORM)
            {
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                hdrEnabled = true;
            }
            else
            {
                AZ_Assert(false, "Unhandled swapchain buffer format.");
            }

            if (m_colorSpace != colorSpace)
            {
                EnsureColorSpace(colorSpace);
                if (hdrEnabled)
                {
                    // [GFX TODO][ATOM-2587] How to specify and determine the limits of the display and scene?
                    float maxOutputNits = 1000.f;
                    float minOutputNits = 0.001f;
                    float maxContentLightLevelNits = 2000.f;
                    float maxFrameAverageLightLevelNits = 500.f;
                    SetHDRMetaData(maxOutputNits, minOutputNits, maxContentLightLevelNits, maxFrameAverageLightLevelNits);
                }
                else
                {
                    DisableHdr();
                }
            }
        }

        void SwapChain::EnsureColorSpace(const DXGI_COLOR_SPACE_TYPE& colorSpace)
        {
            AZ_Assert(colorSpace != DXGI_COLOR_SPACE_CUSTOM, "Invalid color space type for swapchain.");

            UINT colorSpaceSupport = 0;
            HRESULT res = m_swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);
            if (res == S_OK)
            {
                if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
                {
                    [[maybe_unused]] HRESULT hr = m_swapChain->SetColorSpace1(colorSpace);
                    AZ_Assert(S_OK == hr, "Failed to set swap chain color space.");
                    m_colorSpace = colorSpace;
                }
            }
        }

        void SwapChain::DisableHdr()
        {
            // Reset the HDR metadata.
            [[maybe_unused]] HRESULT hr = m_swapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);
            AZ_Assert(S_OK == hr, "Failed to reset HDR metadata.");
        }

        void SwapChain::SetHDRMetaData(float maxOutputNits, float minOutputNits, float maxContentLightLevelNits, float maxFrameAverageLightLevelNits)
        {
            struct DisplayChromacities
            {
                float RedX;
                float RedY;
                float GreenX;
                float GreenY;
                float BlueX;
                float BlueY;
                float WhiteX;
                float WhiteY;
            };
            static const DisplayChromacities DisplayChromacityList[] =
            {
                { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 
                { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
            };

            // Select the chromaticity based on HDR format of the DWM.
            int selectedChroma = 0;
            if (m_colorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709)
            {
                selectedChroma = 0;
            }
            else if (m_colorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
            {
                selectedChroma = 1;
            }
            else
            {
                AZ_Assert(false, "Unhandled color space for swapchain.");
            }

            // These are scaling factors that the api expects values to be normalized to.
            const float ChromaticityScalingFactor = 50000.0f;
            const float LuminanceScalingFactor = 10000.0f;

            const DisplayChromacities& Chroma = DisplayChromacityList[selectedChroma];
            DXGI_HDR_METADATA_HDR10 HDR10MetaData = {};
            HDR10MetaData.RedPrimary[0] = static_cast<UINT16>(Chroma.RedX * ChromaticityScalingFactor);
            HDR10MetaData.RedPrimary[1] = static_cast<UINT16>(Chroma.RedY * ChromaticityScalingFactor);
            HDR10MetaData.GreenPrimary[0] = static_cast<UINT16>(Chroma.GreenX * ChromaticityScalingFactor);
            HDR10MetaData.GreenPrimary[1] = static_cast<UINT16>(Chroma.GreenY * ChromaticityScalingFactor);
            HDR10MetaData.BluePrimary[0] = static_cast<UINT16>(Chroma.BlueX * ChromaticityScalingFactor);
            HDR10MetaData.BluePrimary[1] = static_cast<UINT16>(Chroma.BlueY * ChromaticityScalingFactor);
            HDR10MetaData.WhitePoint[0] = static_cast<UINT16>(Chroma.WhiteX * ChromaticityScalingFactor);
            HDR10MetaData.WhitePoint[1] = static_cast<UINT16>(Chroma.WhiteY * ChromaticityScalingFactor);
            HDR10MetaData.MaxMasteringLuminance = static_cast<UINT>(maxOutputNits * LuminanceScalingFactor);
            HDR10MetaData.MinMasteringLuminance = static_cast<UINT>(minOutputNits * LuminanceScalingFactor);
            HDR10MetaData.MaxContentLightLevel = static_cast<UINT16>(maxContentLightLevelNits);
            HDR10MetaData.MaxFrameAverageLightLevel = static_cast<UINT16>(maxFrameAverageLightLevelNits);
            [[maybe_unused]] HRESULT hr = m_swapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &HDR10MetaData);
            AZ_Assert(S_OK == hr, "Failed to set HDR meta data.");
        }
    }
}
