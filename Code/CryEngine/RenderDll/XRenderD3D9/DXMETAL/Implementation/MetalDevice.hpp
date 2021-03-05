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

// Description : Declaration of the type CDevice and the functions to
//               initialize Metal contexts and detect hardware capabilities.

#pragma once

#include "GLCommon.hpp"
#include "METALContext.hpp"

#if defined(AZ_PLATFORM_MAC)
#include <AppKit/AppKit.h>
using NativeViewType = NSView;
using NativeViewControllerType = NSViewController;
using NativeScreenType = NSScreen;
using NativeApplicationType = NSApplication;
using NativeWindowType = NSWindow;
#else
#include <UIKit/UIKit.h>
using NativeViewType = UIView;
using NativeViewControllerType = UIViewController;
using NativeScreenType = UIScreen;
using NativeApplicationType = UIApplication;
using NativeWindowType = UIWindow;
#endif


#include     <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

////////////////////////////////////////////////////////////////////////////////
namespace NCryMetal
{
    ////////////////////////////////////////////////////////////////////////////
    enum EFeature
    {
        eF_ComputeShader,
        eF_NUM
    };

    ////////////////////////////////////////////////////////////////////////////
    DXGL_DECLARE_REF_COUNTED(class, CDevice)
    {
    public:
        CDevice();
        ~CDevice();

        static bool CreateMetalWindow(HWND* handle, uint32 width, uint32 height, bool fullScreen);
        static void DestroyMetalWindow(HWND handle);

        CContext* CreateContext();
        void FreeContext(CContext* pContext);

        bool Initialize(const TWindowContext& defaultNativeDisplay);
        void Shutdown();
        bool Present();

        const id<MTLDevice> GetMetalDevice() const { return m_metalDevice; }
        const id<MTLCommandQueue> GetMetalCommandQueue() const { return m_commandQueue; }

    protected:
        NativeViewType* m_currentView;
        NativeViewControllerType* m_viewController;

        id<MTLDevice> m_metalDevice;
        id<MTLCommandQueue> m_commandQueue;
    };

    ////////////////////////////////////////////////////////////////////////////
    DXGL_DECLARE_REF_COUNTED(struct, SAdapter)
    {
        string m_description;

        // The supported usage for each GI format (union of D3D11_FORMAT_SUPPORT flags)
        uint32 m_giFormatSupport[eGIF_NUM];
        SBitMask<eF_NUM, SUnsafeBitMaskWord>::Type m_features;

        GLint m_maxSamples;
        size_t m_vramBytes;
    };

    ////////////////////////////////////////////////////////////////////////////
    bool DetectAdapters(std::vector<SAdapterPtr>& kAdapters);
}

////////////////////////////////////////////////////////////////////////////////
@interface MetalView : NativeViewType {}
+ (id)layerClass;
- (id)initWithFrame: (CGRect)frame scale: (CGFloat)scale device: (id<MTLDevice>)device;
- (void)setFrameSize: (CGSize) size;

@property  (nonatomic, assign )CAMetalLayer *metalLayer;

@end    // MetalView Interface

////////////////////////////////////////////////////////////////////////////////
@interface MetalViewController : NativeViewControllerType {}
- (BOOL)prefersStatusBarHidden;
#if defined(AZ_PLATFORM_IOS)
- (void)viewWillTransitionToSize: (CGSize)size withTransitionCoordinator: (id<UIViewControllerTransitionCoordinator>) coordinator;
#endif
@end    // MetalViewController Interface
