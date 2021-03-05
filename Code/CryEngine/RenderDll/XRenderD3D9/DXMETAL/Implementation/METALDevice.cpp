/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Implementation of the type CDevice and the functions to
//               initialize Metal contexts and detect hardware capabilities.

#include "RenderDll_precompiled.h"
#include "MetalDevice.hpp"
#include "GLResource.hpp"
#include <DriverD3D.h>

////////////////////////////////////////////////////////////////////////////////
@implementation MetalView

////////////////////////////////////////////////////////////////////////////////
@synthesize metalLayer = _metalLayer;

////////////////////////////////////////////////////////////////////////////////
+ (id)layerClass
{
    return [CAMetalLayer class];
}

////////////////////////////////////////////////////////////////////////////////
- (id)initWithFrame: (CGRect)frame
              scale: (CGFloat)scale
             device: (id<MTLDevice>)device
{
    if ((self = [super initWithFrame: frame]))
    {

#if defined(AZ_PLATFORM_MAC)
        self.wantsLayer = YES;
        self.layer = _metalLayer = [CAMetalLayer layer];
        self.autoresizingMask = (NSViewWidthSizable | NSViewHeightSizable);
#else
        _metalLayer = (CAMetalLayer*)self.layer;
        self.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
        if ([self respondsToSelector : @selector(contentScaleFactor)])
        {
            self.contentScaleFactor = scale;
        }
#endif
        _metalLayer.device = device;
        _metalLayer.framebufferOnly = TRUE;
        _metalLayer.drawsAsynchronously = TRUE;
        _metalLayer.presentsWithTransaction = FALSE;
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        CGFloat components[] = { 0.0, 0.0, 0.0, 1 };
        _metalLayer.backgroundColor = CGColorCreate(CGColorSpaceCreateDeviceRGB(), components);

        self.autoresizesSubviews = TRUE;

        
#if defined(AZ_PLATFORM_IOS)
        self.multipleTouchEnabled = TRUE;
#endif // defined(AZ_PLATFORM_IOS)

    }

    return self;
}

- (void)setFrameSize: (CGSize) size
{
#if defined(AZ_PLATFORM_MAC)
    // iOS UiView may not respond to this message according to the compile error...
    [super setFrameSize: size];
#endif
    [_metalLayer setDrawableSize: size];
}
@end // MetalView Implementation

////////////////////////////////////////////////////////////////////////////////
@implementation MetalViewController

////////////////////////////////////////////////////////////////////////////////
- (BOOL)prefersStatusBarHidden
{
    return TRUE;
}

#if defined(AZ_PLATFORM_IOS)
- (void)viewWillTransitionToSize: (CGSize)size withTransitionCoordinator: (id<UIViewControllerTransitionCoordinator>) coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
    CGFloat screenScale = [nativeScreen scale];
    int width = static_cast<int>(size.width * screenScale);
    int height = static_cast<int>(size.height * screenScale);
    
    ICVar* widthCVar = gEnv->pConsole->GetCVar("r_width");
    ICVar* heightCVar = gEnv->pConsole->GetCVar("r_height");
    
    // We need to wait for the render thread to finish before we set the new dimensions.
    if (!gRenDev->m_pRT->IsRenderThread(true))
    {
        gEnv->pRenderer->GetRenderThread()->WaitFlushFinishedCond();
    }
    
    gcpRendD3D->GetClampedWindowSize(width, height);
    
    widthCVar->Set(width);
    heightCVar->Set(height);
    gcpRendD3D->SetWidth(widthCVar->GetIVal());
    gcpRendD3D->SetHeight(heightCVar->GetIVal());
}
#endif

#if defined(AZ_PLATFORM_MAC)
////////////////////////////////////////////////////////////////////////////////
- (void)keyDown: (NSEvent*)nsEvent
{
    // Override and do nothing to suppress beeping sound
}
#endif // defined(AZ_PLATFORM_MAC)

@end // MetalViewController Implementation

////////////////////////////////////////////////////////////////////////////////
bool UIDeviceIsTablet()
{
#if defined(AZ_PLATFORM_IOS)
    if([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad)
    {
        return true;
    }
#endif
    return false;
}

////////////////////////////////////////////////////////////////////////////////
bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels)
{

    NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
#if defined(AZ_PLATFORM_MAC)
    CGRect screenBounds = [nativeScreen frame];
    CGFloat screenScale = 1.0f;
#else
    CGRect screenBounds = [nativeScreen bounds];
    CGFloat screenScale = [nativeScreen scale];
#endif
    o_widthPixels = static_cast<int>(screenBounds.size.width * screenScale);
    o_heightPixels = static_cast<int>(screenBounds.size.height * screenScale);

#if defined(AZ_PLATFORM_IOS)
    const bool isScreenLandscape = o_widthPixels > o_heightPixels;

    UIInterfaceOrientation uiOrientation = UIInterfaceOrientationUnknown;
#if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
    if(@available(iOS 13.0, tvOS 13.0, *))
    {
        UIWindow* foundWindow = nil;
        
        //Find the key window
        NSArray* windows = [[UIApplication sharedApplication] windows];
        for (UIWindow* window in windows)
        {
            if (window.isKeyWindow)
            {
                foundWindow = window;
                break;
            }
        }
        
        //Check if the key window is found
        if(foundWindow)
        {
            uiOrientation = foundWindow.windowScene.interfaceOrientation;
        }
        else
        {
            //If no key window is found create a temporary window in order to extract the orientation
            //This can happen as this function gets called before the renderer is initialized
            CGRect screenBounds = [[NativeScreenType mainScreen] bounds];
            UIWindow* tempWindow = [[NativeWindowType alloc] initWithFrame: screenBounds];
            uiOrientation = tempWindow.windowScene.interfaceOrientation;
            [tempWindow release];
        }
    }
#else
    uiOrientation = UIApplication.sharedApplication.statusBarOrientation;
#endif
    
    const bool isInterfaceLandscape = UIInterfaceOrientationIsLandscape(uiOrientation);
    if (isScreenLandscape != isInterfaceLandscape)
    {
        const int width = o_widthPixels;
        o_widthPixels = o_heightPixels;
        o_heightPixels = width;
    }
#endif // defined(AZ_PLATFORM_IOS)

    return true;
}

////////////////////////////////////////////////////////////////////////////////
namespace NCryMetal
{
    extern const DXGI_FORMAT DXGI_FORMAT_INVALID = DXGI_FORMAT_FORCE_UINT;

    ////////////////////////////////////////////////////////////////////////////
    CDevice::CDevice()
        : m_currentView(nullptr)
        , m_viewController(nullptr)
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    CDevice::~CDevice()
    {
        Shutdown();
    }

    ////////////////////////////////////////////////////////////////////////////
    bool CDevice::CreateMetalWindow(HWND* handle,
                                    uint32 width,
                                    uint32 height,
                                    bool fullScreen)
    {
#if defined(AZ_PLATFORM_MAC)
        CGRect screenBounds = CGRectMake(0, 0, width, height);
        //Make the window closeable and minimizeable.
        NSUInteger styleMask = NSWindowStyleMaskClosable|NSWindowStyleMaskTitled|NSWindowStyleMaskMiniaturizable;
        NativeWindowType* nativeWindow = [[NativeWindowType alloc] initWithContentRect: screenBounds styleMask: styleMask backing: NSBackingStoreBuffered defer:false];

        [nativeWindow makeKeyAndOrderFront:nil];
        
        if(fullScreen)
        {
            [nativeWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            [nativeWindow toggleFullScreen:nil];
        }
#else
        CGRect screenBounds = [[NativeScreenType mainScreen] bounds];
        NativeWindowType* nativeWindow = [[NativeWindowType alloc] initWithFrame: screenBounds];
        [nativeWindow makeKeyAndVisible];
#endif
    
        *handle = nativeWindow;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    void CDevice::DestroyMetalWindow(HWND handle)
    {
        NativeWindowType* nativeWindow = reinterpret_cast<NativeWindowType*>(handle);
        [nativeWindow release];
    }

    ////////////////////////////////////////////////////////////////////////////
    CContext* CDevice::CreateContext()
    {
        CContext* pContext(new CContext(this));
        if (!pContext->Initialize())
        {
            delete pContext;
            pContext = nullptr;
        }

        return pContext;
    }

    ////////////////////////////////////////////////////////////////////////////
    void CDevice::FreeContext(CContext* pContext)
    {
        delete pContext;
    }

    ////////////////////////////////////////////////////////////////////////////
    bool CDevice::Initialize(const TWindowContext& defaultNativeDisplay)
    {
        m_metalDevice = MTLCreateSystemDefaultDevice();
        m_commandQueue = [m_metalDevice newCommandQueue];

        const bool isDisplayAWindow = ([(id)defaultNativeDisplay isKindOfClass:[NativeWindowType class]]) == YES;
        if (isDisplayAWindow)
        {
            NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
            NativeWindowType* nativeWindow = reinterpret_cast<NativeWindowType*>(defaultNativeDisplay);
            assert(nativeWindow != nullptr);
            
            // Create the MetalView
#if defined(AZ_PLATFORM_MAC)
            bool isFullScreen = (([nativeWindow styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
            CGRect screenBounds = [nativeScreen visibleFrame];
            if(isFullScreen)
            {
                [nativeWindow setFrame:screenBounds display: true animate: true];
            }
            else
            {
                CGRect screenFrame = [[nativeWindow screen] frame];
                CGRect windowFrame = [nativeWindow frame];
                
                CGFloat xPos = NSWidth(screenFrame)/2 - NSWidth(windowFrame)/2;
                CGFloat yPos = NSHeight(screenFrame)/2 - NSHeight(windowFrame)/2;
                
                //Put the window in the centre of the screen
                [nativeWindow setFrame:NSMakeRect(xPos, yPos, NSWidth(windowFrame), NSHeight(windowFrame)) display:true];
                screenBounds = [nativeWindow frame];
            }
            
            CGFloat screenScale = 1.0f;
#else
            CGRect screenBounds = [nativeScreen bounds];
            CGFloat screenScale = [nativeScreen scale];
#endif
            m_currentView = [[MetalView alloc] initWithFrame: screenBounds
                                                       scale: screenScale
                                                      device: m_metalDevice];

            [m_currentView retain];
            
            // Create the MetalViewController
            MetalViewController* metalViewController = [MetalViewController alloc];
            m_viewController = [metalViewController init];
            [m_viewController setView : m_currentView];
            [m_viewController retain];
            
            // Add the MetalView and assign the MetalViewController to the UIWindow        
#if defined(AZ_PLATFORM_MAC)
            // Setting the contentViewController implicitly sets the contentView
            nativeWindow.contentViewController = m_viewController;
            [nativeWindow makeFirstResponder: m_currentView];
#else
            nativeWindow.rootViewController = m_viewController;
#endif
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    void CDevice::Shutdown()
    {
        // Destroy the MetalViewController
        if (m_viewController)
        {
            NativeWindowType* nativeWindow = static_cast<NativeWindowType*>(m_currentView.superview);
#if defined(AZ_PLATFORM_MAC)
            if (nativeWindow.contentViewController == m_viewController)
            {
                nativeWindow.contentViewController = nil;
            }
#else
            if (nativeWindow.rootViewController == m_viewController)
            {
                nativeWindow.rootViewController = nil;
            }
            [m_viewController setView : nil];
#endif
            
            [m_viewController release];
            m_viewController = nullptr;
        }

        // Destroy the MetalView
        if (m_currentView)
        {
            [m_currentView removeFromSuperview];
            [m_currentView release];
            m_currentView = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    bool CDevice::Present()
    {
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    uint32 DetectGIFormatSupport(EGIFormat eGIFormat)
    {
        const SGIFormatInfo* formatInfo = GetGIFormatInfo(eGIFormat);
        if (!formatInfo)
        {
            return 0;
        }

        uint32 support = formatInfo->m_uDefaultSupport;

        if (formatInfo->m_pTexture)
        {
            DXGL_TODO("Use an alternative way to detect texture format support such as proxy textures");
            support |= D3D11_FORMAT_SUPPORT_TEXTURE1D |
                       D3D11_FORMAT_SUPPORT_TEXTURE2D |
                       D3D11_FORMAT_SUPPORT_TEXTURE3D |
                       D3D11_FORMAT_SUPPORT_TEXTURECUBE |
                       D3D11_FORMAT_SUPPORT_MIP;
        }
        else
        {
            support &= ~(D3D11_FORMAT_SUPPORT_TEXTURE1D |
                         D3D11_FORMAT_SUPPORT_TEXTURE2D |
                         D3D11_FORMAT_SUPPORT_TEXTURE3D |
                         D3D11_FORMAT_SUPPORT_TEXTURECUBE |
                         D3D11_FORMAT_SUPPORT_MIP);
        }

        if (formatInfo->m_pUncompressed && formatInfo->m_pTexture)
        {
            DXGL_TODO("Use an alternative way to detect format renderability such as per-platform tables in GLFormat.cpp");
            support |= D3D11_FORMAT_SUPPORT_RENDER_TARGET |
                       D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET |
                       D3D11_FORMAT_SUPPORT_BLENDABLE |
                       D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
        }
        else
        {
            support &= ~(D3D11_FORMAT_SUPPORT_RENDER_TARGET |
                         D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET |
                         D3D11_FORMAT_SUPPORT_BLENDABLE |
                         D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
        }

        return support;
    }

    ////////////////////////////////////////////////////////////////////////////
    bool DetectAdapters(std::vector<SAdapterPtr>& kAdapters)
    {
        SAdapterPtr spAdapter(new SAdapter);
        spAdapter->m_description = "Metal Renderer iOS";
        spAdapter->m_maxSamples = 4;
        spAdapter->m_vramBytes = 0; // Not yet implemented
        spAdapter->m_features.Set(eF_ComputeShader, DXGL_SUPPORT_COMPUTE ? true : false);

        for (uint32 giFormat = 0; giFormat < NCryMetal::eGIF_NUM; ++giFormat)
        {
            spAdapter->m_giFormatSupport[giFormat] = DetectGIFormatSupport((EGIFormat)giFormat);
        }

        kAdapters.push_back(spAdapter);

        return true;
    }
}
