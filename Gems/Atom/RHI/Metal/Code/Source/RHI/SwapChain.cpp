/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/SwapChain.h>

namespace Platform
{
    CGRect GetScreenBounds(NativeWindowType* nativeWindow);
    CGFloat GetScreenScale();
    void AttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController, RHIMetalView* metalView);
    void UnAttachViewController(NativeWindowType* nativeWindow, NativeViewControllerType* viewController);
    void PresentInternal(id <MTLCommandBuffer> mtlCommandBuffer, id<CAMetalDrawable> drawable, float syncInterval, float refreshRate);
    void ResizeInternal(RHIMetalView* metalView, CGSize viewSize);
    float GetRefreshRate();
    RHIMetalView* GetMetalView(NativeWindowType* nativeWindow);
}

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<SwapChain> SwapChain::Create()
        {
            return aznew SwapChain();
        }
        
        Device& SwapChain::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        RHI::ResultCode SwapChain::InitInternal(RHI::Device& deviceBase, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions)
        {
            auto& device = static_cast<Device&>(deviceBase);
            m_mtlDevice = device.GetMtlDevice();
            m_nativeWindow = reinterpret_cast<NativeWindowType*>(descriptor.m_window.GetIndex());
            AZ_Assert(m_nativeWindow != nullptr, "No window created");
            
            const bool isDisplayAWindow = ([(id)descriptor.m_window.GetIndex() isKindOfClass:[NativeWindowType class]]) == YES;
            if (isDisplayAWindow)
            {
                              
                // Create the MetalView
                CGRect screenBounds = Platform::GetScreenBounds(m_nativeWindow);
                CGFloat screenScale = Platform::GetScreenScale();
                m_metalView = [[RHIMetalView alloc] initWithFrame: screenBounds
                                                           scale: screenScale
                                                          device: m_mtlDevice];
                
                [m_metalView retain];
                
                // Create the MetalViewController
                RHIMetalViewController* metalViewController = [RHIMetalViewController alloc];
                m_viewController = [metalViewController init];
                [m_viewController setView : m_metalView];
                [m_viewController retain];
                        
                Platform::AttachViewController(m_nativeWindow, m_viewController, m_metalView);
                
                m_metalView.metalLayer.drawableSize = CGSizeMake(descriptor.m_dimensions.m_imageWidth, descriptor.m_dimensions.m_imageHeight);
            }
            else
            {
                AddSubView();
            }

            m_drawables.resize(descriptor.m_dimensions.m_imageCount);

            if (nativeDimensions)
            {
                *nativeDimensions = descriptor.m_dimensions;
            }

            AzFramework::WindowRequestBus::EventResult(
                m_refreshRate, m_nativeWindow, &AzFramework::WindowRequestBus::Events::GetDisplayRefreshRate);

            return RHI::ResultCode::Success;
        }

        void SwapChain::AddSubView()
        {
            NativeViewType* superView = reinterpret_cast<NativeViewType*>(m_nativeWindow);
            
            CGFloat screenScale = Platform::GetScreenScale();
            CGRect screenBounds = [superView bounds];
            m_metalView = [[RHIMetalView alloc] initWithFrame: screenBounds
                                                       scale: screenScale
                                                      device: m_mtlDevice];
            
            [m_metalView retain];
            [superView addSubview: m_metalView];
        }
    
        void SwapChain::ShutdownInternal()
        {
            if (m_viewController)
            {
                NativeWindowType* nativeWindow = static_cast<NativeWindowType*>(m_metalView.window);
                Platform::UnAttachViewController(nativeWindow, m_viewController);
                [m_viewController release];
                m_viewController = nullptr;
            }
            
            // Destroy the RHIMetalView
            if (m_metalView)
            {
                [m_metalView removeFromSuperview];
                [m_metalView release];
                m_metalView = nullptr;
            }
        }

        RHI::ResultCode SwapChain::InitImageInternal(const InitImageRequest& request)
        {
            Name name(AZStd::string::format("SwapChainImage_%d", request.m_imageIndex));
            Image& image = static_cast<Image&>(*request.m_image);
            
            //For metal we can only request swapchain texture right before writing into it which is handled by the appropriate scope itself.
            RHI::ImageDescriptor imgDescriptor = image.GetDescriptor();
            image.SetDescriptor(imgDescriptor);
            image.SetName(name);
            image.m_isSwapChainImage = true;

            return RHI::ResultCode::Success;
        }

        void SwapChain::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            Image& image = static_cast<Image&>(resourceBase);

            const size_t sizeInBytes = image.GetMemoryView().GetSize();

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_reservedInBytes -= sizeInBytes;
            memoryUsage.m_residentInBytes -= sizeInBytes;

            image.m_memoryView = {};
        }

        uint32_t SwapChain::PresentInternal()
        {
            const uint32_t currentImageIndex = GetCurrentImageIndex();
            
            //Preset the drawable
            Platform::PresentInternal(
                m_mtlCommandBuffer,
                m_drawables[currentImageIndex], GetDescriptor().m_verticalSyncInterval,
                m_refreshRate);
            
            [m_drawables[currentImageIndex] release];
            m_drawables[currentImageIndex] = nil;
            
            return (GetCurrentImageIndex() + 1) % GetImageCount();
        }

        RHI::ResultCode SwapChain::ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions)
        {
            if(m_metalView)
            {
                Platform::ResizeInternal(m_metalView, CGSizeMake(dimensions.m_imageWidth, dimensions.m_imageHeight));
            }
            else
            {
                if ([(id)m_nativeWindow isKindOfClass:[NativeWindowType class]])
                {
                    // Cache the window's view in order to get drawables
                    m_metalView = Platform::GetMetalView(m_nativeWindow);
                }
                else
                {
                    AddSubView();
                }
            }
            return RHI::ResultCode::Success;
        }
        
        void SwapChain::SetCommandBuffer(id <MTLCommandBuffer> mtlCommandBuffer)
        {
            m_mtlCommandBuffer = mtlCommandBuffer;
        }
    
        id<MTLTexture> SwapChain::RequestDrawable(bool isFrameCaptureEnabled)
        {
            AZ_PROFILE_SCOPE(RHI, "SwapChain::RequestDrawable");
            m_metalView.metalLayer.framebufferOnly = !isFrameCaptureEnabled;
            const uint32_t currentImageIndex = GetCurrentImageIndex();
            if(m_drawables[currentImageIndex])
            {
                //We already have a drawable for this frame. Lets return that
                //This can happen if a pass comes after Swapchain and wants to write to the swapchain texture
                return m_drawables[currentImageIndex].texture;
            }
            else
            {
                m_drawables[currentImageIndex] = [m_metalView.metalLayer nextDrawable];
                AZ_Assert(m_drawables[currentImageIndex], "Drawable can not be null");
                
                //Need this to make sure the drawable is alive for Present call
                [m_drawables[currentImageIndex] retain];
                
                id<MTLTexture> mtlDrawableTexture =  m_drawables[currentImageIndex].texture;
                if(isFrameCaptureEnabled)
                {
                    //If the swapchainimage's m_memoryView does not exist create one and if it already exists override the
                    //native texture pointer with the one received from the driver (i.e nextDrawable call).
                    Image* swapChainImage = static_cast<Image*>(GetCurrentImage());
                    if( swapChainImage->GetMemoryView().GetMemory())
                    {
                        swapChainImage->GetMemoryView().GetMemory()->OverrideResource(mtlDrawableTexture);
                    }
                    else
                    {
                        RHI::ImageDescriptor imgDescriptor = swapChainImage->GetDescriptor();
                        imgDescriptor.m_size.m_width = mtlDrawableTexture.width;
                        imgDescriptor.m_size.m_height = mtlDrawableTexture.height;
                        swapChainImage->SetDescriptor(imgDescriptor);
                        
                        RHI::Ptr<MetalResource> resc = MetalResource::Create(MetalResourceDescriptor{mtlDrawableTexture, ResourceType::MtlTextureType, swapChainImage->m_isSwapChainImage});
                        swapChainImage->m_memoryView = MemoryView(resc, 0, mtlDrawableTexture.allocatedSize, 0);
                    }
                }
                return mtlDrawableTexture;
            }
        }
    }
}
