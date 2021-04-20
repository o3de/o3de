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
//               initialize OpenGL contexts and detect hardware
//               capabilities

#include "RenderDll_precompiled.h"

#include "GLDevice.hpp"
#include "GLResource.hpp"
#include <Common/RenderCapabilities.h>
#include <SFunctor.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Module/DynamicModuleHandle.h>

#if defined(ANDROID)
#include <AzCore/Android/Utils.h>
#include <android/native_window.h>
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define GLDEVICE_CPP_SECTION_1 1
#define GLDEVICE_CPP_SECTION_2 2
#define GLDEVICE_CPP_SECTION_3 3
#define GLDEVICE_CPP_SECTION_4 4
#define GLDEVICE_CPP_SECTION_5 5
#define GLDEVICE_CPP_SECTION_6 6
#define GLDEVICE_CPP_SECTION_7 7
#define GLDEVICE_CPP_SECTION_8 8
#define GLDEVICE_CPP_SECTION_9 9
#define GLDEVICE_CPP_SECTION_10 10
#endif

#if defined(DEBUG) && !defined(MAC)
#define DXGL_DEBUG_CONTEXT 1
#else
#define DXGL_DEBUG_CONTEXT 0
#endif

#if defined(RELEASE)
#define DXGL_DEBUG_OUTPUT_VERBOSITY 0
#elif DXGL_SUPPORT_DEBUG_OUTPUT
#define DXGL_DEBUG_OUTPUT_VERBOSITY 1
#if defined(DEBUG)
#define DXGL_DEBUG_OUTPUT_SYNCHRONOUS 1
#endif //defined(DEBUG)
#endif

// This is the minimum number of uniform buffers required by the engine in order to run.
// Used when checking the capabilities of an adapter.
static const int MIN_UNIFORM_BUFFERS_REQUIRED = 8;

namespace NCryOpenGL
{
    extern const DXGI_FORMAT DXGI_FORMAT_INVALID = DXGI_FORMAT_FORCE_UINT;

    CDevice::WindowSizeList CDevice::m_windowSizes;

#if defined(AZ_PLATFORM_LINUX)
    // Unfortunately this cannot be a member variable because it gets initialized in the static function CreateWindow,
    // which only intends to return the handle of the created window.  That function is called before CDevice is ever created,
    // so we can't simply initialize the display in that class.
    Display* s_defaultDisplay = nullptr;
#endif

#if DXGL_DEBUG_OUTPUT_VERBOSITY
    #if defined(WIN32)
        #define DXGL_DEBUG_CALLBACK_CONVENTION APIENTRY
    #else
        #define DXGL_DEBUG_CALLBACK_CONVENTION
    #endif
    void DXGL_DEBUG_CALLBACK_CONVENTION DebugCallback(GLenum eSource, GLenum eType, GLuint uId, GLenum eSeverity, GLsizei uLength, const GLchar* szMessage, void* pUserParam);
#endif //DXGL_DEBUG_OUTPUT_VERBOSITY

    SVersion GetRequiredGLVersion()
    {
#if defined(OPENGL_ES)
        uint32 version = DXGLES_REQUIRED_VERSION;
#else
        uint32 version = DXGL_REQUIRED_VERSION;
#endif
        return SVersion(version);
    }

#if defined(DXGL_USE_EGL)
    SDisplayConnection::SDisplayConnection()
        : m_display(EGL_NO_DISPLAY)
        , m_surface(EGL_NO_SURFACE)
        , m_config(nullptr)
        , m_window(EGL_NULL_VALUE)
        , m_dirtyFlag(false)
    {}

    SDisplayConnection::~SDisplayConnection()
    {
        if (m_display != EGL_NO_DISPLAY)
        {
            if (m_surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(m_display, m_surface);
            }
            eglTerminate(m_display);
        }
    }

    SDisplayConnection* SDisplayConnection::Create(const SPixelFormatSpec& pixelFormatSpec, const TNativeDisplay& defaultNativeDisplay)
    {
        SDisplayConnection* displayConnection = new SDisplayConnection();
        if (!displayConnection->Init(pixelFormatSpec, defaultNativeDisplay))
        {
            delete displayConnection;
            return nullptr;
        }

        return displayConnection;
    }

    bool SDisplayConnection::Init(const SPixelFormatSpec& kPixelFormatSpec, const TNativeDisplay& kDefaultNativeDisplay)
    {
#   if defined(OPENGL_ES)
        EGLenum api = EGL_OPENGL_ES_API;
        EGLint renderableType = EGL_OPENGL_ES3_BIT;
#   else
        EGLenum api = EGL_OPENGL_API;
        EGLint renderableType = EGL_OPENGL_BIT;
#   endif
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        
        m_window = kDefaultNativeDisplay->second;

        // Desktop egl platforms only currently support OpenGL ES while mobile egl platforms support OpenGL and OpenGL ES.
        // The api selection function is unavailable on desktop egl platforms
        if (eglBindAPI != NULL)
        {
            if (eglBindAPI(api) == EGL_FALSE)
            {
                DXGL_ERROR("eglBindAPI failed");
                return false;
            }
        }

#if !defined(AZ_PLATFORM_LINUX)
        m_display = eglGetDisplay(kDefaultNativeDisplay->first);
#else
        // If we use the EGL_DEFAULT_DISPLAY for the remaining code in this function when doing some of these operations
        // many operations fail and we end up failing to initialize anything.  Always use the default egl dislpay created
        // from the XOpenDisplay results instead of whatever the kDefaultNativeDisplay has
        m_display = eglGetDisplay(s_defaultDisplay);
#endif

        if (m_display == EGL_NO_DISPLAY)
        {
            DXGL_ERROR("eglGetDisplay failed");
            return false;
        }

        if (eglInitialize(m_display, NULL, NULL) != EGL_TRUE)
        {
            DXGL_ERROR("eglInitialize failed");
            return false;
        }

        bool usePbuffer = m_window == EGL_NULL_VALUE;
        EGLint aiAttributes[] =
        {
            EGL_RENDERABLE_TYPE,          renderableType,
            EGL_SURFACE_TYPE,             usePbuffer ? EGL_PBUFFER_BIT : EGL_WINDOW_BIT,
            EGL_RED_SIZE,                 static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uRedBits),
            EGL_GREEN_SIZE,               static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uGreenBits),
            EGL_BLUE_SIZE,                static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uBlueBits),
            EGL_ALPHA_SIZE,               static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uAlphaBits),
            EGL_BUFFER_SIZE,              static_cast<EGLint>(kPixelFormatSpec.m_pLayout->GetColorBits()),
            EGL_DEPTH_SIZE,               static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uDepthBits),
            EGL_STENCIL_SIZE,             static_cast<EGLint>(kPixelFormatSpec.m_pLayout->m_uStencilBits),
            EGL_SAMPLE_BUFFERS,           static_cast<EGLint>(kPixelFormatSpec.m_uNumSamples > 1 ? 1 : 0),
            EGL_SAMPLES,                  static_cast<EGLint>(kPixelFormatSpec.m_uNumSamples > 1 ? kPixelFormatSpec.m_uNumSamples : 0),
            EGL_NONE
        };

        EGLint iFoundConfigs;
        if (eglChooseConfig(m_display, aiAttributes, &m_config, 1, &iFoundConfigs) != EGL_TRUE || iFoundConfigs < 1)
        {
            DXGL_ERROR("eglChooseConfig failed");
            return false;
        }

#if !defined(AZ_PLATFORM_LINUX)
        CreateSurface();
#else
        // If we want to run in headless mode and not create a window, for optimal setup when rendering video from the server,
        // then do not create an X11 window and only create an offscreen pixel buffer surface.
        // r_GetScreenShot can then be triggered to capture a screenshot to user/screenshots/
        ICVar* skipWindowCreationCvar = gEnv->pConsole->GetCVar("r_linuxSkipWindowCreation");
        if (skipWindowCreationCvar && (skipWindowCreationCvar->GetIVal() > 0))
        {
            usePbuffer = true;
        }

        if (usePbuffer)
        {
            CreateSurface();
        }
        else
        {
            bool result = CreateX11Window();
            if (!result)
            {
                DXGL_ERROR("Failed to create X11 window");
                return false;
            }
        }
#endif

        if (m_surface == EGL_NO_SURFACE)
        {
            DXGL_ERROR("Failed to create EGL surface");
            return false;
        }

        return true;
    }

    bool SDisplayConnection::CreateSurface()
    {
        if (m_display == EGL_NO_DISPLAY || m_config == nullptr)
        {
            return false;
        }

        if (m_window)
        {
            m_surface = eglCreateWindowSurface(m_display, m_config, m_window, NULL);
        }
        else
        {
            const EGLint surfaceAttributes[] = {
                EGL_WIDTH, 1,
                EGL_HEIGHT, 1,
                EGL_NONE
            };
            m_surface = eglCreatePbufferSurface(m_display, m_config, surfaceAttributes);
        }

        return m_surface != EGL_NO_SURFACE;
    }

    bool SDisplayConnection::DestroySurface()
    {
        if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE)
        {
            return false;
        }

        EGLBoolean result = eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
        return result == EGL_TRUE;
    }

#if defined(AZ_PLATFORM_LINUX)
    bool SDisplayConnection::CreateX11Window()
    {
        // We need to create an actual window and a window-renderable surface
        EGLint visualID;
        if (!eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &visualID))
        {
            AZ_Assert(false, "Error: eglGetConfigAttrib failed - [0x%08x]", eglGetError());
            return false;
        }

        // TODO Linux - Get these from somewhere else besides the cvars
        ICVar* widthCVar = gEnv->pConsole->GetCVar("r_width");
        ICVar* heightCVar = gEnv->pConsole->GetCVar("r_height");
        int width = static_cast<int>(widthCVar->GetIVal());
        int height = static_cast<int>(heightCVar->GetIVal());
        const char* title = "Placeholder Title";


        // Get the XVisualInfo config that matches the EGL config.
        int numberXVisuals = 0;
        XVisualInfo visualInfoTemplate;
        visualInfoTemplate.visualid = visualID;
        XVisualInfo* visualInfo = XGetVisualInfo(s_defaultDisplay, VisualIDMask, &visualInfoTemplate, &numberXVisuals);
        if (numberXVisuals == 0)
        {
            AZ_Assert(false, "XGetVisualInfo failed to match egl configurations");
            return false;
        }

        Window rootWindow = DefaultRootWindow(s_defaultDisplay);
        Colormap colorMap = XCreateColormap(s_defaultDisplay, rootWindow, visualInfo->visual, AllocNone);

        XSetWindowAttributes windowAttributes;
        windowAttributes.colormap = colorMap;
        windowAttributes.event_mask = ExposureMask | KeyPressMask;

        Window applicationWindow = XCreateWindow(s_defaultDisplay, rootWindow, 0, 0, width, height, 0, visualInfo->depth, InputOutput, visualInfo->visual, CWColormap | CWEventMask, &windowAttributes);

        m_surface = eglCreateWindowSurface(m_display, m_config, applicationWindow, NULL);
        if (!m_surface)
        {
            AZ_Assert(false, "Error: eglCreateWindowSurface failed - [0x%08x]", eglGetError());
            return false;
        }

        // Map the window and set the name
        XMapWindow(s_defaultDisplay, applicationWindow);
        XStoreName(s_defaultDisplay, applicationWindow, title);
        return true;
    }
#endif

    void SDisplayConnection::SetWindow(EGLNativeWindowType window)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        if (window != m_window)
        {
            if (m_window)
            {
                DestroySurface();
            }

            m_window = window;

            if (window)
            {
                CreateSurface();
                m_dirtyFlag = true;
            }
        }
    }

    bool SDisplayConnection::MakeCurrent(const TRenderingContext context) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        EGLSurface surface = context ? m_surface : EGL_NO_SURFACE;
        EGLBoolean res = eglMakeCurrent(m_display, surface, surface, context);
        AZ_Warning("Rendering", res == EGL_TRUE, "eglMakeCurrent failed [0x%08x]", eglGetError());
        return res == EGL_TRUE;
    }

    bool SDisplayConnection::SwapBuffers(const TRenderingContext context)
    {
        if (m_dirtyFlag)
        {
            // The surface was recreated so we need to make current again before doing the swap.
            if (!MakeCurrent(context))
            {
                return false;
            }
        }

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_dirtyFlag = false;
        if (m_surface == EGL_NO_SURFACE)
        {
            return false;
        }

        return eglSwapBuffers(m_display, m_surface) == EGL_TRUE;
    }

#elif defined(DXGL_USE_WGL)

    bool SetWindowPixelFormat(const TWindowContext& kWindowContext, const SPixelFormatSpec* pPixelFormatSpec)
    {
        int32 iPixelFormat(0);
        PIXELFORMATDESCRIPTOR kPixDesc;
        ZeroMemory(&kPixDesc, sizeof(PIXELFORMATDESCRIPTOR));
        // Check for wgl pixel format extension availability
        if (DXGL_WGL_EXTENSION_SUPPORTED(ARB_pixel_format) && pPixelFormatSpec)
        {
            int32 aiAttributes[128];
            int32* pAttrCursor = aiAttributes;

            *pAttrCursor++ = WGL_DRAW_TO_WINDOW_ARB;
            *pAttrCursor++ = GL_TRUE;
            *pAttrCursor++ = WGL_SUPPORT_OPENGL_ARB;
            *pAttrCursor++ = GL_TRUE;
            *pAttrCursor++ = WGL_DOUBLE_BUFFER_ARB;
            *pAttrCursor++ = GL_TRUE;
            *pAttrCursor++ = WGL_PIXEL_TYPE_ARB;
            *pAttrCursor++ = WGL_TYPE_RGBA_ARB;
            *pAttrCursor++ = WGL_RED_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uRedBits;
            *pAttrCursor++ = WGL_GREEN_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uGreenBits;
            *pAttrCursor++ = WGL_BLUE_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uBlueBits;
            *pAttrCursor++ = WGL_ALPHA_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uAlphaBits;
            *pAttrCursor++ = WGL_RED_SHIFT_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uRedShift;
            *pAttrCursor++ = WGL_GREEN_SHIFT_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uGreenShift;
            *pAttrCursor++ = WGL_BLUE_SHIFT_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uBlueShift;
            *pAttrCursor++ = WGL_ALPHA_SHIFT_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uAlphaShift;
            *pAttrCursor++ = WGL_COLOR_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->GetColorBits();
            *pAttrCursor++ = WGL_DEPTH_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uDepthBits;
            *pAttrCursor++ = WGL_STENCIL_BITS_ARB;
            *pAttrCursor++ = pPixelFormatSpec->m_pLayout->m_uStencilBits;

            // Sample counts 0 and 1 do not require multisampling attribute
            if (pPixelFormatSpec->m_uNumSamples > 1)
            {
                *pAttrCursor++ = WGL_SAMPLES_ARB;
                *pAttrCursor++ = pPixelFormatSpec->m_uNumSamples;
            }

            // Request SRGB pixel format only when needed, skip this attribute otherwise (fix for pedantic drivers)
            if (pPixelFormatSpec->m_bSRGB)
            {
                *pAttrCursor++ = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
                *pAttrCursor++ = GL_TRUE;
            }

            // Mark end of the attribute list
            *pAttrCursor = 0;

            uint32 uNumFormats;
            if (!DXGL_UNWRAPPED_FUNCTION(wglChoosePixelFormatARB)(kWindowContext, aiAttributes, NULL, 1, &iPixelFormat, &uNumFormats))
            {
                DXGL_ERROR("wglChoosePixelFormatARB failed");
                return false;
            }
        }
        else
        {
            kPixDesc.nSize           = sizeof(PIXELFORMATDESCRIPTOR);
            kPixDesc.nVersion        = 1;
            kPixDesc.dwFlags         = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
            kPixDesc.iPixelType      = PFD_TYPE_RGBA;
            kPixDesc.iLayerType      = PFD_MAIN_PLANE;
            if (pPixelFormatSpec != NULL)
            {
                kPixDesc.cRedBits        = pPixelFormatSpec->m_pLayout->m_uRedBits;
                kPixDesc.cGreenBits      = pPixelFormatSpec->m_pLayout->m_uGreenBits;
                kPixDesc.cBlueBits       = pPixelFormatSpec->m_pLayout->m_uBlueBits;
                kPixDesc.cAlphaBits      = pPixelFormatSpec->m_pLayout->m_uAlphaBits;
                kPixDesc.cRedShift       = pPixelFormatSpec->m_pLayout->m_uRedShift;
                kPixDesc.cGreenShift     = pPixelFormatSpec->m_pLayout->m_uGreenShift;
                kPixDesc.cBlueShift      = pPixelFormatSpec->m_pLayout->m_uBlueShift;
                kPixDesc.cAlphaShift     = pPixelFormatSpec->m_pLayout->m_uAlphaShift;
                kPixDesc.cColorBits      = pPixelFormatSpec->m_pLayout->GetColorBits();
                kPixDesc.cDepthBits      = pPixelFormatSpec->m_pLayout->m_uDepthBits;
                kPixDesc.cStencilBits    = pPixelFormatSpec->m_pLayout->m_uStencilBits;
                if (pPixelFormatSpec->m_uNumSamples > 1 || pPixelFormatSpec->m_bSRGB)
                {
                    DXGL_WARNING("wglChoosePixelFormatARB not available - multisampling and sRGB settings will be ignored");
                }
            }
            else
            {
                kPixDesc.cColorBits      = 32;
            }

            iPixelFormat = ChoosePixelFormat(kWindowContext, &kPixDesc);
            if (iPixelFormat == 0)
            {
                DXGL_ERROR("ChoosePixelFormat failed");
                return false;
            }

            if (pPixelFormatSpec == NULL &&
                DescribePixelFormat(kWindowContext, iPixelFormat, sizeof(kPixDesc), &kPixDesc) == 0)
            {
                DXGL_ERROR("DescribePixelFormat failed");
                return false;
            }
        }

        if (!SetPixelFormat(kWindowContext, iPixelFormat, &kPixDesc))
        {
            DXGL_ERROR("SetPixelFormat failed");
            return false;
        }
        return true;
    }

#endif

#if defined(WIN32)

    namespace NWin32Helper
    {
        enum
        {
            eWS_Windowed   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            eWS_FullScreen = WS_POPUP
        };

        bool GetDisplayRect(RECT* pRect, SOutput* pOutput)
        {
            DEVMODEA kCurrentMode;
            if (EnumDisplaySettingsA(pOutput->m_strDeviceName.c_str(), ENUM_CURRENT_SETTINGS, &kCurrentMode) != TRUE)
            {
                DXGL_ERROR("Could not retrieve the current display settings for display %s", pOutput->m_strDeviceName.c_str());
                return false;
            }

            DXGL_TODO("Check if scaling according to the GetDeviceCaps is required");
            pRect->left   = kCurrentMode.dmPosition.x;
            pRect->top    = kCurrentMode.dmPosition.y;
            pRect->right  = kCurrentMode.dmPosition.x + kCurrentMode.dmPelsWidth;
            pRect->bottom = kCurrentMode.dmPosition.y + kCurrentMode.dmPelsHeight;
            return true;
        }

        bool SetFullScreenContext(SOutput* pOutput, TNativeDisplay kNativeDisplay, DEVMODEA& kDevMode)
        {
            if (ChangeDisplaySettingsExA(pOutput->m_strDeviceName.c_str(), &kDevMode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                DXGL_ERROR("Could not change display settings");
                return false;
            }

            RECT kFullScreenRect;
            if (!GetDisplayRect(&kFullScreenRect, pOutput))
            {
                return false;
            }

            HWND kWindowHandle(WindowFromDC(kNativeDisplay));
            DWORD uStyle(GetWindowLong(kWindowHandle, GWL_STYLE));
            uStyle &= ~eWS_Windowed;
            uStyle |= eWS_FullScreen;
            if (SetWindowLong(kWindowHandle, GWL_STYLE, uStyle) == 0)
            {
                DXGL_WARNING("Could not set the window style");
            }
            if (SetWindowPos(kWindowHandle, NULL, kFullScreenRect.left, kFullScreenRect.top, kFullScreenRect.right - kFullScreenRect.left, kFullScreenRect.bottom - kFullScreenRect.top, SWP_NOCOPYBITS) != TRUE)
            {
                DXGL_WARNING("Could not set window posititon");
            }
            return true;
        }

        bool UnsetFullScreenContext(SOutput* pOutput)
        {
            if (ChangeDisplaySettingsExA(pOutput->m_strDeviceName.c_str(), NULL, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                DXGL_ERROR("Could not restore original display settings");
                return false;
            }
            return true;
        }

        bool ResizeWindowContext(TNativeDisplay kNativeDisplay, uint32 uWidth, uint32 uHeight)
        {
            HWND kWindowHandle(WindowFromDC(kNativeDisplay));

            RECT kWindowRect;
            if (!GetWindowRect(kWindowHandle, &kWindowRect))
            {
                DXGL_WARNING("Could not retrieve window rectangle - moving to origin");
                kWindowRect.left = kWindowRect.right = kWindowRect.top = kWindowRect.bottom = 0;
            }
            kWindowRect.right =  kWindowRect.left + GetSystemMetrics(SM_CXDLGFRAME) * 2 + uWidth;
            kWindowRect.bottom = kWindowRect.top  + GetSystemMetrics(SM_CXDLGFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) + uHeight;

            DWORD uStyle(GetWindowLong(kWindowHandle, GWL_STYLE));
            uStyle &= ~eWS_FullScreen;
            uStyle |= eWS_Windowed;

            if (SetWindowLong(kWindowHandle, GWL_STYLE, uStyle) == 0)
            {
                DXGL_WARNING("Could not set the window style");
            }
            if (SetWindowPos(kWindowHandle, NULL, kWindowRect.left, kWindowRect.top, kWindowRect.right - kWindowRect.left, kWindowRect.bottom - kWindowRect.top, SWP_NOCOPYBITS) != TRUE)
            {
                DXGL_WARNING("Could not set window posititon");
            }

            return true;
        }
    };

#endif

#if defined(WIN32)
    SOutput* GetWindowOutput(SAdapter* pAdapter, const TNativeDisplay& kNativeDisplay)
    {
        RECT kWindowRect;
        HWND kWindowHandle(WindowFromDC(kNativeDisplay));
        if (kWindowHandle == NULL || GetWindowRect(kWindowHandle, &kWindowRect) != TRUE)
        {
            DXGL_ERROR("Could not retrieve the device window rectangle");
            return nullptr;
        }

        int32 iWindowCenterX((kWindowRect.left + kWindowRect.right) / 2);
        int32 iWindowCenterY((kWindowRect.top + kWindowRect.bottom) / 2);

        uint32 uOutput;
        SOutput* pMinDistOutput(NULL);
        uint32 uMinDistSqr;
        for (uOutput = 0; uOutput < pAdapter->m_kOutputs.size(); ++uOutput)
        {
            SOutput* pOutput(pAdapter->m_kOutputs.at(uOutput));
            RECT kDisplayRect;
            if (!NWin32Helper::GetDisplayRect(&kDisplayRect, pOutput))
            {
                return nullptr;
            }

            if (kWindowRect.left   >= kDisplayRect.left  &&
                kWindowRect.right  <= kDisplayRect.right &&
                kWindowRect.top    >= kDisplayRect.top   &&
                kWindowRect.bottom <= kDisplayRect.bottom)
            {
                return pOutput; // Window completely inside the display rectangle
            }
            int32 iDistX(iWindowCenterX - (kDisplayRect.left + kDisplayRect.right) / 2);
            int32 iDistY(iWindowCenterY - (kDisplayRect.top + kDisplayRect.bottom) / 2);
            uint32 uCenterDistSqr(iDistX * iDistX + iDistY * iDistY);
            if (uOutput == 0 || uCenterDistSqr < uMinDistSqr)
            {
                uMinDistSqr = uCenterDistSqr;
                pMinDistOutput = pOutput;
            }
        }

        return pMinDistOutput;
    }
#endif // defined(WIN32)

#if defined(WIN32)

    void DevModeToDisplayMode(SDisplayMode* pDisplayMode, const DEVMODEA& kDevMode)
    {
        pDisplayMode->m_uBitsPerPixel = kDevMode.dmBitsPerPel;
        pDisplayMode->m_uWidth        = kDevMode.dmPelsWidth;
        pDisplayMode->m_uHeight       = kDevMode.dmPelsHeight;
        pDisplayMode->m_uFrequency    = kDevMode.dmDisplayFrequency;
    }

#endif // defined(WIN32)

    struct SDummyWindow
    {
        TNativeDisplay m_kNativeDisplay;
#if defined(WIN32)
        HWND m_kWndHandle;
        ATOM m_kWndClassAtom;

        static LRESULT CALLBACK DummyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
#endif //defined(WIN32)

        SDummyWindow()
            : m_kNativeDisplay()
#if defined(WIN32)
            , m_kWndHandle(NULL)
            , m_kWndClassAtom(0)
#endif //defined(WIN32)
        {
        }

        bool Initialize(const SPixelFormatSpec* pPixelFormatSpec)
        {
#if defined(DXGL_USE_EGL)
            // No need to create a window because we are going to use an EGL pixel buffer surface.
            m_kNativeDisplay = AZStd::make_shared<EGLNativePlatform>(EGL_DEFAULT_DISPLAY, EGL_NULL_VALUE);
#elif defined(WIN32)
            WNDCLASSW kWndClass;
            kWndClass.style = CS_HREDRAW | CS_VREDRAW;
            kWndClass.lpfnWndProc = (WNDPROC)iSystem->GetRootWindowMessageHandler();
            kWndClass.cbClsExtra = 0;
            kWndClass.cbWndExtra = 0;
            kWndClass.hInstance = NULL;
            kWndClass.hIcon = LoadIconA(NULL, (LPCSTR)IDI_WINLOGO);
            kWndClass.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
            kWndClass.hbrBackground = NULL;
            kWndClass.lpszMenuName = NULL;
            kWndClass.lpszClassName = L"Dummy DXGL window class";

            if ((m_kWndClassAtom = RegisterClassW(&kWndClass)) == NULL ||
                (m_kWndHandle = CreateWindowW((LPCWSTR)MAKEINTATOM(m_kWndClassAtom), L"Dummy DXGL window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL)) == NULL ||
                (m_kNativeDisplay = GetDC(m_kWndHandle)) == NULL)
            {
                DXGL_ERROR("Creation of the dummy DXGL window failed (%d)", GetLastError());
                return false;
            }

    #if defined(DXGL_USE_WGL)
            if (!SetWindowPixelFormat(m_kNativeDisplay, pPixelFormatSpec))
            {
                return false;
            }
    #endif //DXGL_USE_WGL
#endif
            return true;
        }

        void Shutdown()
        {
#if defined(WIN32)
            if (m_kWndHandle != NULL)
            {
                DestroyWindow(m_kWndHandle);
            }
            if (m_kWndClassAtom != 0)
            {
                UnregisterClassW((LPCWSTR)MAKEINTATOM(m_kWndClassAtom), NULL);
            }
#endif
        }
    };


    ////////////////////////////////////////////////////////////////////////////
    // CDevice implementation
    ////////////////////////////////////////////////////////////////////////////


#if DXGL_FULL_EMULATION
    uint32 CDevice::ms_uNumContextsPerDevice = 16;
#else
    uint32 CDevice::ms_uNumContextsPerDevice = 1;
#endif

#if DXGL_EXTENSION_LOADER && defined(AZ_PLATFORM_LINUX)
    // Linux needs an early call to LoadEarlyGLEntryPoints, so we need to forward declare it.
    bool LoadEarlyGLEntryPoints();
#endif // DXGL_EXTENSION_LOADER

    CDevice* CDevice::ms_pCurrentDevice = NULL;

    CDevice::CDevice(SAdapter* pAdapter, const SFeatureSpec& kFeatureSpec, const SPixelFormatSpec& kPixelFormatSpec)
        : m_spAdapter(pAdapter)
        , m_kFeatureSpec(kFeatureSpec)
        , m_kPixelFormatSpec(kPixelFormatSpec)
        , m_kContextFenceIssued(false)
        , m_texturesStreamingFunctorId(0)
    {
        if (ms_pCurrentDevice == NULL)
        {
            ms_pCurrentDevice = this;
        }
        m_pCurrentContextTLS = CreateTLS();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
    }

    CDevice::~CDevice()
    {
        Shutdown();
        DestroyTLS(m_pCurrentContextTLS);
        if (ms_pCurrentDevice == this)
        {
            ms_pCurrentDevice = NULL;
        }
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
    }

#if !defined(WIN32)
    bool CDevice::CreateWindow(const char* title, uint32 width, uint32 height, bool fullscreen, HWND * handle)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(ANDROID)
        // Windows is already created by the Native Activity. We just return a pointer to the ANativeWindow.
        ANativeWindow* nativeWindow = AZ::Android::Utils::GetWindow();
        *handle = reinterpret_cast<HWND>(nativeWindow);
#elif defined(AZ_PLATFORM_LINUX)
        // Get the default display and root window handles.
        // We are currently only rendering to a pixel buffer and not yet creating an actual window via X11
        Display* defaultDisplay = XOpenDisplay(NULL);
        AZ_Assert(defaultDisplay, "XOpenDisplay failed");

        Window rootWindow = DefaultRootWindow(defaultDisplay);

#if DXGL_EXTENSION_LOADER
        if (!LoadEarlyGLEntryPoints())
        {
            return false;
        }
#endif // DXGL_EXTENSION_LOADER

        *handle = reinterpret_cast<HWND>(rootWindow);
        s_defaultDisplay = defaultDisplay;
#else
#error "Not implemented for this platform"
#endif
        InitWindow(*handle, width, height);
        m_windowSizes[handle] = std::make_pair(width, height);
        return true;
    }

    void CDevice::DestroyWindow(HWND handle)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(ANDROID)
        // Nothing to do since the window is destroyed by the OS when the Native Activity is destroyed
#elif defined(AZ_PLATFORM_LINUX)
        AZ_Assert(0, "TODO Linux OpenGL");
#else
#error "Not implemented for this platform"
#endif
        m_windowSizes.erase(handle);
    }

    void CDevice::InitWindow(HWND handle, uint32 width, uint32 height)
    {
#if defined(ANDROID)
        ANativeWindow* nativeWindow = reinterpret_cast<ANativeWindow*>(handle);
        // We need to set the windows size to match the engine width and height. The Android compositor will upscale it to fullscreen.
        ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBX_8888); // discard alpha
#elif defined(AZ_PLATFORM_LINUX)
        // TODO Linux - What needs done here?
        //AZ_Assert(0, "TODO Linux OpenGL");
#endif
    }
#endif // !defined(WIN32)

    void CDevice::OnApplicationWindowCreated()
    {
#if defined(ANDROID)
        uint32 width, height;
        if (m_windowSizes.empty())
        {
            AZ_Error("OpenGL", false, "Could not find window size. Using backbuffer size.");
            width = gcpRendD3D->GetBackbufferWidth();
            height = gcpRendD3D->GetBackbufferHeight();
        }
        else
        {
            // Android only uses one screen. Just use the first one in the list.
            std::pair<uint32, uint32> windowSize = m_windowSizes.begin()->second;
            width = windowSize.first;
            height = windowSize.second;
        }

        HWND window = reinterpret_cast<HWND>(AZ::Android::Utils::GetWindow());
        CDevice::InitWindow(window, width, height);
#elif defined(AZ_PLATFORM_LINUX)
        AZ_Assert(0, "TODO Linux OpenGL");
#endif
    }

    void CDevice::OnApplicationWindowRedrawNeeded()
    {
#if defined(ANDROID)
        if (gEnv && gEnv->pConsole && gEnv->pRenderer)
        {
            ICVar* widthCVar = gEnv->pConsole->GetCVar("r_width");
            ICVar* heightCVar = gEnv->pConsole->GetCVar("r_height");

            int width, height;
            if (AZ::Android::Utils::GetWindowSize(width, height))
            {
                gcpRendD3D->GetClampedWindowSize(width, height);

                widthCVar->Set(width);
                heightCVar->Set(height);

                // We need to wait for the render thread to finish before we set the new dimensions.
                // Since Android has a separate render thread, it'll be in the middle of rendering the scene when this function is called.
                if (!gRenDev->m_pRT->IsRenderThread(true))
                {
                    gEnv->pRenderer->GetRenderThread()->WaitFlushFinishedCond();
                }

                gcpRendD3D->SetWidth(widthCVar->GetIVal());
                gcpRendD3D->SetHeight(heightCVar->GetIVal());

                InitWindow(AZ::Android::Utils::GetWindow(), width, height);
                DetectOutputs(*m_spAdapter, m_spAdapter->m_kOutputs);
            }
        }
#elif defined(AZ_PLATFORM_LINUX)
        AZ_Assert(0, "TODO Linux OpenGL");
#endif
    }

    void CDevice::Configure(uint32 uNumSharedContexts)
    {
        ms_uNumContextsPerDevice = min((uint32)MAX_NUM_CONTEXT_PER_DEVICE, 1 + uNumSharedContexts);
    }


    static void OnChangeTexturesStreaming(ICVar* pCVar, CDevice* device)
    {
        int32 newVal = pCVar->GetIVal();
        if (newVal > 0 && !device->IsFeatureSupported(NCryOpenGL::eF_CopyImage))
        {
            AZ_Warning("Rendering", false, "Disabling Textures Streaming because is not supported on this device.");
            newVal = 0;
        }

        pCVar->Set(newVal);
    }

    bool CDevice::Initialize(const TNativeDisplay& kDefaultNativeDisplay)
    {
        if (kDefaultNativeDisplay == NULL)
        {
            return false;
        }
        else
        {
            m_kDefaultNativeDisplay = kDefaultNativeDisplay;
        }

        std::vector<TRenderingContext> kRenderingContexts;
        if (!CreateRenderingContexts(m_kDefaultWindowContext, kRenderingContexts, m_kFeatureSpec, m_kPixelFormatSpec, m_kDefaultNativeDisplay))
        {
            return false;
        }

        m_kContexts.reserve(kRenderingContexts.size());
        uint32 uContext;
        for (uContext = 0; uContext < kRenderingContexts.size(); ++uContext)
        {
            const TRenderingContext& kRenderingContext(kRenderingContexts.at(uContext));
            TWindowContext windowContext = m_kDefaultWindowContext;
            CContext::ContextType type = uContext == 0 ? CContext::RenderingType : CContext::ResourceType;
#if defined(DXGL_USE_EGL)
            // We use the window's surface for the context that will do the actual rendering and 1x1 PBuffer surfaces for the loading threads.
            if (type == CContext::ResourceType)
            {
                windowContext.reset(SDisplayConnection::Create(m_kPixelFormatSpec, AZStd::make_shared<EGLNativePlatform>(EGL_DEFAULT_DISPLAY, EGL_NULL_VALUE)));
            }
#endif // ANDROID

            CContext* pContext(new CContext(this, kRenderingContext, windowContext, uContext, type));
            MakeCurrent(windowContext, kRenderingContext);

            if (uContext == 0)
            {
                InitializeResourceUnitPartitions();
            }

            if (!pContext->Initialize())
            {
                delete pContext;
                return false;
            }

            m_kFreeContexts[pContext->GetType()].Push(*pContext);
            m_kContexts.push_back(pContext);
        }

        MakeCurrent(m_kDefaultWindowContext, NULL);

        // Check for textures streaming support
        if (ICVar* cVar = gEnv->pConsole->GetCVar("r_texturesStreaming"))
        {
            SFunctor onChange;
            onChange.Set(OnChangeTexturesStreaming, cVar, this);
            m_texturesStreamingFunctorId = cVar->AddOnChangeFunctor(onChange);
            onChange.Call();
        }

        return true;
    }

    void CDevice::Shutdown()
    {
        MakeCurrent(m_kDefaultWindowContext, NULL);

        TContexts::const_iterator kContextIter = m_kContexts.begin();
        const TContexts::const_iterator kContextEnd = m_kContexts.end();
        while (kContextIter != kContextEnd)
        {
            TRenderingContext kRenderingContext((*kContextIter)->GetRenderingContext());
            delete *kContextIter;
            // Delete context after all resources have been released. Avoids memory
            // leaks an crashes with non-nvidia drivers
#if defined(DXGL_USE_EGL)
            eglDestroyContext(m_kDefaultWindowContext->GetDisplay(), kRenderingContext);
#elif defined(DXGL_USE_WGL)
            wglDeleteContext(kRenderingContext);
#else
#error "Not supported on this platform"
#endif
            ++kContextIter;
        }
        m_kContexts.clear();

        if (m_kDefaultWindowContext != NULL)
        {
            ReleaseWindowContext(m_kDefaultWindowContext);
        }

        if (ICVar* cVar = gEnv->pConsole->GetCVar("r_texturesStreaming"))
        {
            cVar->RemoveOnChangeFunctor(m_texturesStreamingFunctorId);
        }
    }

    bool CDevice::Present(const TWindowContext& kTargetWindowContext)
    {
#if defined(DXGL_USE_EGL)
        CContext* currentContext = GetCurrentContext();
        return kTargetWindowContext->SwapBuffers(currentContext ? currentContext->GetRenderingContext() : nullptr);
#elif defined(WIN32)
        return SwapBuffers(kTargetWindowContext) == TRUE;
#else
        DXGL_NOT_IMPLEMENTED;
        return false;
#endif
    }

    CContext* CDevice::ReserveContext()
    {
        CContext* pCurrentContext(static_cast<CContext*>(GetTLSValue(m_pCurrentContextTLS)));
        CContext* pReservedContext(NULL);
        if (pCurrentContext != NULL)
        {
            pReservedContext = pCurrentContext->GetReservedContext();
        }

        if (pReservedContext == NULL)
        {
            pReservedContext = AllocateContext();
            if (pReservedContext == NULL)
            {
                return NULL;
            }
        }

        if (pCurrentContext == NULL)
        {
            pCurrentContext = pReservedContext;
            SetCurrentContext(pCurrentContext);
        }

        pReservedContext->IncReservationCount();
        pCurrentContext->SetReservedContext(pReservedContext);

        return pCurrentContext;
    }

    void CDevice::ReleaseContext()
    {
        CContext* pCurrentContext(GetCurrentContext());
        assert(pCurrentContext != NULL);
        CContext* pReservedContext(pCurrentContext->GetReservedContext());
        assert(pReservedContext != NULL);

        if (pReservedContext->DecReservationCount() == 0)
        {
            if (pCurrentContext == pReservedContext)
            {
                SetCurrentContext(NULL);
            }
            pCurrentContext->SetReservedContext(NULL);
            FreeContext(pReservedContext);
        }
    }

    CContext* CDevice::AllocateContext(CContext::ContextType type /*= CContext::ResourceType*/)
    {
        CContext* pContext(static_cast<CContext*>(m_kFreeContexts[type].Pop()));
        if (pContext == NULL)
        {
            DXGL_ERROR("CDevice::AllocateContext failed - no free context available. Please increase the number of contexts at initialization time (currently %d).", (int)m_kContexts.size());
        }
        return pContext;
    }

    void CDevice::FreeContext(CContext* pContext)
    {
        m_kFreeContexts[pContext->GetType()].Push(*pContext);
    }

    void CDevice::BindContext(CContext* pContext)
    {
        CContext* pCurrentContext(GetCurrentContext());
        if (pCurrentContext != NULL)
        {
            pContext->SetReservedContext(pCurrentContext->GetReservedContext());
        }

        SetCurrentContext(pContext);
    }

    void CDevice::UnbindContext(CContext* pContext)
    {
        CContext* pCurrentContext(static_cast<CContext*>(GetTLSValue(m_pCurrentContextTLS)));
        assert(pCurrentContext != NULL);
        SetCurrentContext(pCurrentContext->GetReservedContext());
    }

    void CDevice::SetCurrentContext(CContext* pContext)
    {
        CContext* pPreviousContext(static_cast<CContext*>(GetTLSValue(m_pCurrentContextTLS)));

        bool bSuccess = false;
        if (pContext != NULL)
        {
            bSuccess = MakeCurrent(pContext->GetWindowContext(), pContext->GetRenderingContext());
        }
        else if (pPreviousContext != NULL)
        {
            bSuccess = MakeCurrent(pPreviousContext->GetWindowContext(), NULL);
        }

        SetTLSValue(m_pCurrentContextTLS, pContext);

        if (!bSuccess)
        {
            DXGL_ERROR("SetCurrentContext failed");
        }
    }

    NCryOpenGL::CContext* CDevice::GetCurrentContext()
    {
        return static_cast<CContext*>(GetTLSValue(m_pCurrentContextTLS));
    }

    uint32 CDevice::GetMaxContextCount()
    {
        return ms_uNumContextsPerDevice;
    }

    void CDevice::IssueFrameFences()
    {
        for (uint32 uContext = 0; uContext < m_kContexts.size(); ++uContext)
        {
            m_kContextFenceIssued.Set(uContext, true);
        }
    }

    bool CDevice::CreateRenderingContexts(
        TWindowContext& kWindowContext,
        std::vector<TRenderingContext>& kRenderingContexts,
        const SFeatureSpec& kFeatureSpec,
        const SPixelFormatSpec& kPixelFormatSpec,
        const TNativeDisplay& kNativeDisplay)
    {
        if (!CreateWindowContext(kWindowContext, kFeatureSpec, kPixelFormatSpec, kNativeDisplay))
        {
            return false;
        }

#if defined(DXGL_USE_EGL)
    #if defined(AZ_PLATFORM_LINUX)
        EGLint aiContextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE};
    #else
        SVersion version = GetRequiredGLVersion();
        EGLint aiContextAttributes[] = 
        {
            EGL_CONTEXT_MAJOR_VERSION, static_cast<EGLint>(version.m_uMajorVersion),
            EGL_CONTEXT_MINOR_VERSION, static_cast<EGLint>(version.m_uMinorVersion),
            EGL_NONE
        };
    #endif
#elif defined(DXGL_USE_WGL)
        int32 aiAttributes[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, kFeatureSpec.m_kVersion.m_uMajorVersion,
            WGL_CONTEXT_MINOR_VERSION_ARB, kFeatureSpec.m_kVersion.m_uMinorVersion,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if DXGL_DEBUG_CONTEXT
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif // DXGL_DEBUG_CONTEXT
            0
        };
#endif

        kRenderingContexts.reserve(ms_uNumContextsPerDevice);
        for (uint32 uContext = 0; uContext < ms_uNumContextsPerDevice; ++uContext)
        {
#if defined(DXGL_USE_EGL)
            TRenderingContext kRenderingContext(eglCreateContext(kWindowContext->GetDisplay(), 
                                                                kWindowContext->GetConfig(), 
                                                                uContext > 0 ? kRenderingContexts.at(0) : EGL_NO_CONTEXT,
                                                                aiContextAttributes));
#elif defined(DXGL_USE_WGL)
            TRenderingContext kSharedRenderingContext(NULL);
            if (uContext > 0)
            {
                kSharedRenderingContext = kRenderingContexts.at(0);
            }
            TRenderingContext kRenderingContext(DXGL_UNWRAPPED_FUNCTION(wglCreateContextAttribsARB)(kWindowContext, kSharedRenderingContext, aiAttributes));
#endif

            if (!kRenderingContext)
            {
                DXGL_ERROR("Failed to create context %d", uContext);
                return false;
            }

            kRenderingContexts.push_back(kRenderingContext);
        }

#if DXGL_DEBUG_OUTPUT_VERBOSITY
        if (kFeatureSpec.m_kFeatures.Get(eF_DebugOutput))
        {
            GLenum aeSeverityLevels[] =
            {
                GL_DEBUG_SEVERITY_HIGH,
                GL_DEBUG_SEVERITY_MEDIUM,
                GL_DEBUG_SEVERITY_LOW,
                GL_DEBUG_SEVERITY_NOTIFICATION
            };

            for (uint32 uContext = 0; uContext < kRenderingContexts.size(); ++uContext)
            {
                MakeCurrent(kWindowContext, kRenderingContexts.at(uContext));
                glEnable(GL_DEBUG_OUTPUT);
#if defined(DXGL_DEBUG_OUTPUT_SYNCHRONOUS)
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif //defined(DXGL_DEBUG_OUTPUT_SYNCHRONOUS)
                if (glDebugMessageCallback)
                {
                    glDebugMessageCallback((GLDEBUGPROC)&DebugCallback, NULL);
                }
#if defined(OPENGL_ES)
                else
                {
                    glDebugMessageCallbackKHR((GLDEBUGPROC)&DebugCallback, NULL);
                }
#endif //defined(OPENGL_ES)

                for (uint32 uSeverityLevel = 0; uSeverityLevel < DXGL_ARRAY_SIZE(aeSeverityLevels); ++uSeverityLevel)
                {
                    if (glDebugMessageControl)
                    {
                        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, aeSeverityLevels[uSeverityLevel], 0, NULL, (uSeverityLevel <= DXGL_DEBUG_OUTPUT_VERBOSITY) ? GL_TRUE : GL_FALSE);
                    }
#if defined(OPENGL_ES)
                    else
                    {
                        glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, aeSeverityLevels[uSeverityLevel], 0, NULL, (uSeverityLevel <= DXGL_DEBUG_OUTPUT_VERBOSITY) ? GL_TRUE : GL_FALSE);
                    }
#endif //defined(OPENGL_ES)
                }
            }
            MakeCurrent(kWindowContext, NULL);
        }
#endif // DXGL_DEBUG_OUTPUT_VERBOSITY

        return true;
    }

    bool CDevice::SetFullScreenState(const SFrameBufferSpec& kFrameBufferSpec, bool bFullScreen, SOutput* pOutput)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        if (bFullScreen)
        {
            if (pOutput == NULL)
            {
                pOutput = GetWindowOutput(m_spAdapter, m_kDefaultNativeDisplay);
            }
            if (pOutput == NULL)
            {
                DXGL_ERROR("Could not retrieve the display output corresponding to the window context");
                return false;
            }

            if (pOutput != m_spFullScreenOutput)
            {
                DEVMODEA kReqDevMode;
                memset(&kReqDevMode, 0, sizeof(kReqDevMode));
                kReqDevMode.dmSize = sizeof(kReqDevMode);
                kReqDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
                kReqDevMode.dmPelsWidth = kFrameBufferSpec.m_uWidth;
                kReqDevMode.dmPelsHeight = kFrameBufferSpec.m_uHeight;
                kReqDevMode.dmBitsPerPel = kFrameBufferSpec.m_pLayout->GetPixelBits();

                if (!NWin32Helper::SetFullScreenContext(pOutput, m_kDefaultNativeDisplay, kReqDevMode))
                {
                    return false;
                }

                m_spFullScreenOutput = pOutput;
            }
        }
        else
        {
            if (m_spFullScreenOutput != NULL)
            {
                if (!NWin32Helper::UnsetFullScreenContext(m_spFullScreenOutput))
                {
                    return false;
                }

                m_spFullScreenOutput = NULL;
            }
        }
        return true;
#elif defined(ANDROID)
        // Android is always full screen.
        return true;
#else
        DXGL_NOT_IMPLEMENTED;
        return false;
#endif
    }

    bool CDevice::ResizeTarget(const SDisplayMode& kTargetMode)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        if (kTargetMode.m_uBitsPerPixel != m_kPixelFormatSpec.m_pLayout->GetPixelBits())
        {
            DXGL_WARNING("ResizeTarget does not support changing the window pixel format");
            return false;
        }

        if (m_spFullScreenOutput != NULL)
        {
            DEVMODEA kDevMode;
            memset(&kDevMode, 0, sizeof(kDevMode));
            kDevMode.dmSize = sizeof(kDevMode);
            kDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
            kDevMode.dmPelsWidth = kTargetMode.m_uWidth;
            kDevMode.dmPelsHeight = kTargetMode.m_uHeight;
            kDevMode.dmBitsPerPel = kTargetMode.m_uBitsPerPixel;
            if (kTargetMode.m_uFrequency != 0)
            {
                kDevMode.dmFields |= DM_DISPLAYFREQUENCY;
                kDevMode.dmDisplayFrequency = kTargetMode.m_uFrequency;
            }

            return NWin32Helper::SetFullScreenContext(m_spFullScreenOutput, m_kDefaultNativeDisplay, kDevMode);
        }
        else
        {
            return NWin32Helper::ResizeWindowContext(m_kDefaultNativeDisplay, kTargetMode.m_uWidth, kTargetMode.m_uHeight);
        }
#elif defined(ANDROID)
        DXGL_WARNING("ResizeTarget is not supported on this platform");
        return false;
#elif defined(AZ_PLATFORM_LINUX)
        AZ_Assert(0, "TODO Linux OpenGL");
#else
#error "Not implemented on this platform"
#endif
        return false;
    }

    bool CDevice::MakeCurrent(const TWindowContext& kWindowContext, TRenderingContext kRenderingContext)
    {
#if defined(DXGL_USE_EGL)
        return kWindowContext->MakeCurrent(kRenderingContext);
#elif defined(DXGL_USE_WGL)
        return wglMakeCurrent(kRenderingContext == NULL ? NULL : kWindowContext, kRenderingContext) == TRUE;
#else
        DXGL_NOT_IMPLEMENTED;
        return false;
#endif
    }

    DXGL_TODO("Move default partitions somewhere else/find a better way since it's not engine-related - possibly pass through DXGLInitialize");

#define _PARTITION_PRED(_First, _Count) { _First, _Count },
#define PARTITION_PRED(_Argument) _PARTITION_PRED _Argument
#define PARTITION(_Vertex, _Fragment, _Geometry, _TessControl, _TessEvaluation, _Compute) \
    { DXGL_SHADER_TYPE_MAP(PARTITION_PRED, _Vertex, _Fragment, _Geometry, _TessControl, _TessEvaluation, _Compute) },

    const TPipelineResourceUnitPartitionBound g_akTextureUnitBounds[] =
    {
        //         VERTEX      FRAGMENT    GEOMETRY    TESSCTL     TESSEVAL    COMPUTE
        PARTITION((0,  10), (0,  16), (0,   6), (0,   0), (0,   0), (0,   0))           // Graphics
        PARTITION((0,   0), (0,   0), (0,   0), (0,   0), (0,   0), (0,  32))           // Compute
    };

    const TPipelineResourceUnitPartitionBound g_akUniformBufferUnitBounds[] =
    {
        //         VERTEX      FRAGMENT    GEOMETRY    TESSCTL     TESSEVAL    COMPUTE
        PARTITION((0,   12), (0,   12), (0,   12), (0,   12), (0,   12), (0,   0))           // Graphics
        PARTITION((0,   0), (0,   0), (0,   0), (0,   0), (0,   0), (0,  16))           // Compute
    };

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS

    const TPipelineResourceUnitPartitionBound g_akStorageBufferUnitBounds[] =
    {
        //         VERTEX      FRAGMENT    GEOMETRY    TESSCTL     TESSEVAL    COMPUTE
        PARTITION((14,   2), (16,   8), (0,   0), (0,   0), (0,   0), (0,   0))         // Graphics
        PARTITION((0,   0), (0,   0), (0,   0), (0,   0), (0,   0), (16,   8))          // Compute
    };

#endif

#if DXGL_SUPPORT_SHADER_IMAGES

    const TPipelineResourceUnitPartitionBound g_akImageUnitBounds[] =
    {
        //         VERTEX      FRAGMENT    GEOMETRY    TESSCTL     TESSEVAL    COMPUTE
        PARTITION((0,   0), (0,   8), (0,   0), (0,   0), (0,   0), (0,   0))           // Graphics
        PARTITION((0,   0), (0,   0), (0,   0), (0,   0), (0,   0), (0,   8))           // Compute
    };

#endif

#undef PARTITION
#undef PARTITION_PRED
#undef _PARTITION_PRED

    void CDevice::InitializeResourceUnitPartitions()
    {
        const SCapabilities& kCapabilities(m_spAdapter->m_kCapabilities);

        PartitionResourceIndices(eRUT_Texture,       g_akTextureUnitBounds,       DXGL_ARRAY_SIZE(g_akTextureUnitBounds));
        PartitionResourceIndices(eRUT_UniformBuffer, g_akUniformBufferUnitBounds, DXGL_ARRAY_SIZE(g_akUniformBufferUnitBounds));
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        PartitionResourceIndices(eRUT_StorageBuffer, g_akStorageBufferUnitBounds, DXGL_ARRAY_SIZE(g_akStorageBufferUnitBounds));
#endif
#if DXGL_SUPPORT_SHADER_IMAGES
        if (m_spAdapter->m_kFeatures.Get(eF_ShaderImages))
        {
            PartitionResourceIndices(eRUT_Image, g_akImageUnitBounds, DXGL_ARRAY_SIZE(g_akImageUnitBounds));
        }
#endif
    }

    bool CDevice::IsFeatureSupported(EFeature feature) const
    {
        AZ_Assert(feature < eF_NUM, "Invalid EFeature %d", feature);
        return m_kFeatureSpec.m_kFeatures.Get(feature);
    }

    const char* GetResourceUnitTypeName(EResourceUnitType eUnitType)
    {
        switch (eUnitType)
        {
        case eRUT_Texture:
            return "Texture unit";
        case eRUT_UniformBuffer:
            return "Uniform buffer unit";
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        case eRUT_StorageBuffer:
            return "Storage buffer unit";
#endif
#if DXGL_SUPPORT_SHADER_IMAGES
        case eRUT_Image:
            return "Image unit";
#endif
        default:
            assert(false);
            return "?";
        }
    }

    bool TryDistributeResourceIndices(
        SIndexPartition& kPartition,
        const SResourceUnitCapabilities& kCapabilities,
        const TPipelineResourceUnitPartitionBound& akStageBounds)
    {
        uint32 uTotBelowLimit = 0;
        uint32 uTotAvailable = kCapabilities.m_aiMaxTotal;

        uint32 uTotUsed = 0;
        for (uint32 uStage = 0; uStage < eST_NUM; ++uStage)
        {
            kPartition.m_akStages[uStage].m_uCount = akStageBounds[uStage].m_uNumUnits;
            uTotUsed += akStageBounds[uStage].m_uNumUnits;

            if (kCapabilities.m_aiMaxPerStage[uStage] < kPartition.m_akStages[uStage].m_uCount)
            {
                return false;
            }

            uTotBelowLimit += kCapabilities.m_aiMaxPerStage[uStage] - kPartition.m_akStages[uStage].m_uCount;
        }

        if (uTotUsed > uTotAvailable)
        {
            return false;
        }

        while (uTotAvailable > uTotUsed && uTotBelowLimit > 0)
        {
            uint32 uTotRemaining = uTotAvailable - uTotUsed;
            uint32 uTotAssigned = 0;
            for (uint32 uStage = 0; uStage < eST_NUM; ++uStage)
            {
                uint32 uBelowLimit = kCapabilities.m_aiMaxPerStage[uStage] - kPartition.m_akStages[uStage].m_uCount;
                if (uBelowLimit > 0)
                {
                    uint32 uAssigned = min(uTotRemaining - uTotAssigned, max(1u, uTotRemaining * uBelowLimit / uTotBelowLimit));
                    kPartition.m_akStages[uStage].m_uCount += uAssigned;
                    uTotAssigned += uAssigned;
                }
            }
            assert(uTotAssigned > 0 && uTotAssigned <= uTotRemaining);
            uTotUsed += uTotAssigned;
        }

        for (uint32 uStage = 0, uFirstSlot = 0; uStage < eST_NUM; ++uStage)
        {
            kPartition.m_akStages[uStage].m_uFirstIn = akStageBounds[uStage].m_uFirstUnit;
            kPartition.m_akStages[uStage].m_uFirstOut = uFirstSlot;
            uFirstSlot += kPartition.m_akStages[uStage].m_uCount;
        }

        return true;
    }

    void CDevice::PartitionResourceIndices(
        EResourceUnitType eUnitType,
        const TPipelineResourceUnitPartitionBound* akPartitionBounds,
        uint32 uNumPartitions)
    {
        CDevice::TPartitions& kPartitions = m_kResourceUnitPartitions[eUnitType];
        const SResourceUnitCapabilities& kCapabilities = m_spAdapter->m_kCapabilities.m_akResourceUnits[eUnitType];

        kPartitions.reserve(uNumPartitions);

        for (uint32 uPartition = 0; uPartition < uNumPartitions; ++uPartition)
        {
            const TPipelineResourceUnitPartitionBound& akStageBounds(akPartitionBounds[uPartition]);

            SIndexPartition kPartition;
            if (TryDistributeResourceIndices(kPartition, kCapabilities, akStageBounds))
            {
                kPartitions.push_back(kPartition);
            }
            else
            {
                DXGL_WARNING("%s partition %d is not supported by this configuration - it will not be used", GetResourceUnitTypeName(eUnitType), uPartition);
            }
        }
    }

    struct SDummyContext
    {
        SDummyWindow m_kDummyWindow;
        TRenderingContext m_kRenderingContext;
#if defined(DXGL_USE_EGL)
        TWindowContext m_kDisplayConnection;
#endif //DXGL_USE_EGL
        bool m_kIsInitialized;

        SDummyContext()
            : m_kRenderingContext(NULL)
            , m_kIsInitialized(false)
        {
        }

        ~SDummyContext()
        {
            Shutdown();
        }

        bool Initialize()
        {
            if (m_kIsInitialized)
            {
                return true;
            }

            if (!m_kDummyWindow.Initialize(NULL))
            {
                return false;
            }
#if defined(DXGL_USE_EGL)
            SPixelFormatSpec pixelFormat;
            SUncompressedLayout layout;
            ZeroMemory(&pixelFormat, sizeof(pixelFormat));
            ZeroMemory(&layout, sizeof(layout));
            pixelFormat.m_pLayout = &layout;

            m_kDisplayConnection.reset(SDisplayConnection::Create(pixelFormat, m_kDummyWindow.m_kNativeDisplay));
            if (!m_kDisplayConnection)
            {
                DXGL_ERROR("Creation of the dummy DXGL window failed");
                return false;
            }

#if defined(AZ_PLATFORM_LINUX)
            SVersion version = GetRequiredGLVersion();
            EGLint aiContextAttributes[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE};
#else
            SVersion version = GetRequiredGLVersion();
            EGLint aiContextAttributes[] = {
                EGL_CONTEXT_MAJOR_VERSION, static_cast<EGLint>(version.m_uMajorVersion),
                EGL_CONTEXT_MINOR_VERSION, static_cast<EGLint>(version.m_uMinorVersion),
                EGL_NONE};
#endif
            m_kRenderingContext = eglCreateContext(m_kDisplayConnection->GetDisplay(),
                                                   m_kDisplayConnection->GetConfig(), 
                                                    EGL_NO_CONTEXT, 
                                                    aiContextAttributes);
            if (m_kRenderingContext == EGL_NO_CONTEXT)
            {
                DXGL_ERROR("Dummy DXGL context creation failed: [0x%08x]", eglGetError());
                return false;
            }
            if (!m_kDisplayConnection->MakeCurrent(m_kRenderingContext))
            {
                DXGL_ERROR("Dummy DXGL context MakeCurrent failed: [0x%08x]", eglGetError());
                return false;
            }
#elif defined(DXGL_USE_WGL)
            m_kRenderingContext = wglCreateContext(m_kDummyWindow.m_kNativeDisplay);
            if (m_kRenderingContext == NULL ||
                wglMakeCurrent(m_kDummyWindow.m_kNativeDisplay, m_kRenderingContext) != TRUE)
            {
                DXGL_ERROR("Dummy DXGL context creation failed");
                return false;
            }
#else
#   error "Not Implemented"
#endif
            m_kIsInitialized = true;
            return true;
        }

        void Shutdown()
        {
            if (!m_kIsInitialized)
            {
                return;
            }

#if defined(DXGL_USE_EGL)
            m_kDisplayConnection->MakeCurrent(nullptr);
            if (m_kRenderingContext != NULL)
            {
                eglDestroyContext(m_kDisplayConnection->GetDisplay(), m_kRenderingContext);
            }
#elif defined(DXGL_USE_WGL)
            wglMakeCurrent(NULL, NULL);
            if (m_kRenderingContext != 0)
            {
                wglDeleteContext(m_kRenderingContext);
            }
#else
#   error "Not Implemented"
#endif
            m_kDummyWindow.Shutdown();
            m_kIsInitialized = false;
        }
    };

    bool FeatureLevelToFeatureSpec(SFeatureSpec& kFeatureSpec, D3D_FEATURE_LEVEL eFeatureLevel, NCryOpenGL::SAdapter* pGLAdapter)
    {
#if DXGLES && !defined(DXGL_ES_SUBSET)
        //LYTODO: seems like our ES version should line up differently with eFeatureLevel
        if (eFeatureLevel == D3D_FEATURE_LEVEL_11_0)
        {
            uint32 adapterGLVersion = pGLAdapter->m_sVersion.ToUint();
            if (adapterGLVersion >= DXGLES_VERSION_31)
            {
                kFeatureSpec.m_kVersion.m_uMajorVersion = 3;
                kFeatureSpec.m_kVersion.m_uMinorVersion = 1;
            }
            else if (adapterGLVersion == DXGLES_VERSION_30)
            {
                kFeatureSpec.m_kVersion.m_uMajorVersion = 3;
                kFeatureSpec.m_kVersion.m_uMinorVersion = 0;
            }
            else
            {
                DXGL_ERROR("Could not match feature level to openGL version. Feature level = %d, Adapter GL Version = %u", eFeatureLevel, adapterGLVersion);
                return false;
            }
        }
        else
        {
            DXGL_ERROR("Feature level not implemented on OpenGL ES");
            return false;
        }
#else
        switch (eFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_9_1:
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_3:
            kFeatureSpec.m_kVersion.m_uMajorVersion = 2;
            kFeatureSpec.m_kVersion.m_uMinorVersion = 0;
            break;
        case D3D_FEATURE_LEVEL_10_0:
        case D3D_FEATURE_LEVEL_10_1:
            kFeatureSpec.m_kVersion.m_uMajorVersion = 3;
            kFeatureSpec.m_kVersion.m_uMinorVersion = 3;
            break;
        case D3D_FEATURE_LEVEL_11_0:
            kFeatureSpec.m_kVersion.m_uMajorVersion = 4;
            kFeatureSpec.m_kVersion.m_uMinorVersion = 3;
            break;
        default:
            DXGL_ERROR("Unknown feature level");
            return false;
        }
#endif

        kFeatureSpec.m_kFeatures.Set(eF_ComputeShader, eFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
        return true;
    }

    void GetStandardPixelFormatSpec(SPixelFormatSpec& kPixelFormatSpec)
    {
        kPixelFormatSpec.m_pLayout = GetGIFormatInfo(eGIF_R8G8B8A8_UNORM_SRGB)->m_pUncompressed;
        kPixelFormatSpec.m_uNumSamples = 1;
        kPixelFormatSpec.m_bSRGB = true;
    }

    bool SwapChainDescToFrameBufferSpec(SFrameBufferSpec& kFrameBufferSpec, const DXGI_SWAP_CHAIN_DESC& kSwapChainDesc)
    {
        EGIFormat eGIFormat(GetGIFormat(kSwapChainDesc.BufferDesc.Format));
        if (eGIFormat == eGIF_NUM)
        {
            return false;
        }

        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(eGIFormat));

        kFrameBufferSpec.m_uWidth = kSwapChainDesc.BufferDesc.Width;
        kFrameBufferSpec.m_uHeight = kSwapChainDesc.BufferDesc.Height;
        kFrameBufferSpec.m_uNumSamples = kSwapChainDesc.SampleDesc.Count;
        kFrameBufferSpec.m_bSRGB = pFormatInfo->m_pTexture->m_bSRGB;
        kFrameBufferSpec.m_pLayout = pFormatInfo->m_pUncompressed;

        return true;
    }

    bool GetNativeDisplay(TNativeDisplay& kNativeDisplay, HWND kWindowHandle)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        HDC deviceContext = GetDC(kWindowHandle);
        if (deviceContext == NULL)
        {
            DXGL_ERROR("Could not retrieve the DC of the swap chain output window");
            return false;
        }
#   if defined(DXGL_USE_EGL)
        HWND nativeWindow = WindowFromDC(deviceContext);
        if (!nativeWindow)
        {
            DXGL_ERROR("Could not retrieve window from device context");
            return false;
        }
        kNativeDisplay = AZStd::make_shared<EGLNativePlatform>(deviceContext, nativeWindow);
#   else
        kNativeDisplay = deviceContext;
#   endif // defined(DXGL_USE_EGL)
        return true;
#elif defined(ANDROID)
        kNativeDisplay = AZStd::make_shared<EGLNativePlatform>(static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY), static_cast<EGLNativeWindowType>(kWindowHandle));
        return true;
#elif defined(AZ_PLATFORM_LINUX)
        kNativeDisplay = AZStd::make_shared<EGLNativePlatform>(s_defaultDisplay, reinterpret_cast<EGLNativeWindowType>(kWindowHandle));
        return true;
#else
#error "Not supported on this platform"
#endif
    }

    bool CreateWindowContext(
        TWindowContext& kWindowContext,
        const SFeatureSpec& kFeatureSpec,
        const SPixelFormatSpec& kPixelFormatSpec,
        const TNativeDisplay& kNativeDisplay)
    {
#if defined(DXGL_USE_EGL)
        kWindowContext.reset(SDisplayConnection::Create(kPixelFormatSpec, kNativeDisplay));
        if (kWindowContext == NULL)
        {
            return false;
        }
#elif defined(WIN32)
        kWindowContext = kNativeDisplay;
        if (!SetWindowPixelFormat(kWindowContext, &kPixelFormatSpec))
        {
            return false;
        }
#endif

        return true;
    }

    void ReleaseWindowContext(TWindowContext& kWindowContext)
    {
#if defined(DXGL_USE_EGL)
        kWindowContext = nullptr;
#endif //DXGL_USE_EGL
    }

#if DXGL_SUPPORT_QUERY_INTERNAL_FORMAT_SUPPORT

    bool QueryInternalFormatSupport(GLenum eTarget, GLenum eInternalFormat, GLenum eQueryName, uint32 uFlag, uint32& uMask)
    {
        if (DXGL_GL_EXTENSION_SUPPORTED(ARB_internalformat_query2))
        {
            GLint iSupported;
            glGetInternalformativ(eTarget, eInternalFormat, eQueryName, 1, &iSupported);
            bool bSupported;
            switch (iSupported)
            {
            case GL_NONE:
                bSupported = false;
                break;
            case GL_CAVEAT_SUPPORT:
                DXGL_WARNING("Internal format supported but not optimal");
                bSupported = true;
                break;
            case GL_FULL_SUPPORT:
            case GL_TRUE:
                bSupported = true;
                break;
            default:
                DXGL_ERROR("Invalid parameter returned by internal format query");
                return false;
            }
            uMask = bSupported ? uMask | uFlag : uMask & ~uFlag;
            return true;
        }
        return false;
    }

    void QueryInternalFormatTexSupport(GLenum eTarget, GLenum eInternalFormat, uint32 uTexFlag, uint32& uMask)
    {
        if (QueryInternalFormatSupport(eTarget, eInternalFormat, GL_INTERNALFORMAT_SUPPORTED, uTexFlag, uMask) && (uMask & uTexFlag) != 0)
        {
#if !defined(_RELEASE)
            GLint iPreferred;
            glGetInternalformativ(eTarget, eInternalFormat, GL_INTERNALFORMAT_PREFERRED, 1, &iPreferred);
            if (iPreferred != eInternalFormat)
            {
                DXGL_WARNING("Internal format supported but not preferred");
            }
#endif //!defined(_RELEASE)
        }
    }

#endif // DXGL_SUPPORT_QUERY_INTERNAL_FORMAT_SUPPORT

    uint32 DetectGIFormatSupport(EGIFormat eGIFormat)
    {
        uint32 uSupport(0);

        const SGIFormatInfo* pFormatInfo(GetGIFormatInfo(eGIFormat));
        if (pFormatInfo == NULL)
        {
            return uSupport;
        }

        uSupport = pFormatInfo->m_uDefaultSupport;

        const STextureFormat* pTextureFormat(pFormatInfo->m_pTexture);
        if (pTextureFormat != NULL)
        {
#if DXGL_SUPPORT_QUERY_INTERNAL_FORMAT_SUPPORT
            QueryInternalFormatTexSupport(GL_TEXTURE_1D, pTextureFormat->m_iInternalFormat, D3D11_FORMAT_SUPPORT_TEXTURE1D, uSupport);
            QueryInternalFormatTexSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D, uSupport);
            QueryInternalFormatTexSupport(GL_TEXTURE_3D, pTextureFormat->m_iInternalFormat, D3D11_FORMAT_SUPPORT_TEXTURE3D, uSupport);
            QueryInternalFormatTexSupport(GL_TEXTURE_CUBE_MAP, pTextureFormat->m_iInternalFormat, D3D11_FORMAT_SUPPORT_TEXTURECUBE, uSupport);
            QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_MIPMAP, D3D11_FORMAT_SUPPORT_MIP, uSupport);
#else
            DXGL_TODO("Use an alternative way to detect texture format support such as proxy textures");
            uSupport |=
                D3D11_FORMAT_SUPPORT_TEXTURE1D
                | D3D11_FORMAT_SUPPORT_TEXTURE2D
                | D3D11_FORMAT_SUPPORT_TEXTURE3D
                | D3D11_FORMAT_SUPPORT_TEXTURECUBE
                | D3D11_FORMAT_SUPPORT_MIP;
#endif
        }
        else
        {
            uSupport &= ~(
                    D3D11_FORMAT_SUPPORT_TEXTURE1D
                    | D3D11_FORMAT_SUPPORT_TEXTURE2D
                    | D3D11_FORMAT_SUPPORT_TEXTURE3D
                    | D3D11_FORMAT_SUPPORT_TEXTURECUBE
                    | D3D11_FORMAT_SUPPORT_MIP);
        }

        const SUncompressedLayout* pUncompressedLayout(pFormatInfo->m_pUncompressed);
        if (pUncompressedLayout != NULL && pTextureFormat != NULL)
        {
#if DXGL_SUPPORT_QUERY_INTERNAL_FORMAT_SUPPORT
            if (QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_FRAMEBUFFER_RENDERABLE,  D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL, uSupport))
            {
                uint32 uColorRenderable(0);
                QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_COLOR_RENDERABLE, D3D11_FORMAT_SUPPORT_RENDER_TARGET, uColorRenderable);
                uint32 uDepthRenderable(0);
                QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_DEPTH_RENDERABLE, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL, uDepthRenderable);
                uint32 uStencilRenderable(0);
                QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_STENCIL_RENDERABLE, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL, uStencilRenderable);
                uSupport |= uColorRenderable | uDepthRenderable | uStencilRenderable;

                QueryInternalFormatSupport(GL_TEXTURE_2D, pTextureFormat->m_iInternalFormat, GL_FRAMEBUFFER_BLEND, D3D11_FORMAT_SUPPORT_BLENDABLE, uSupport);
            }
#else
            DXGL_TODO("Use an alternative way to detect format renderability such as per-platform tables in GLFormat.cpp");
            uSupport |=
                D3D11_FORMAT_SUPPORT_RENDER_TARGET
                | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET
                | D3D11_FORMAT_SUPPORT_BLENDABLE
                | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
#endif
        }
        else
        {
            uSupport &= ~(
                    D3D11_FORMAT_SUPPORT_RENDER_TARGET
                    | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET
                    | D3D11_FORMAT_SUPPORT_BLENDABLE
                    | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
        }
        return uSupport;
    }

#if DXGL_SUPPORT_GETTEXIMAGE

    bool DetectIfCopyImageWorksOnCubeMapFaces()
    {
        GLuint auInput[16 * 3 * 3];
        for (uint32 uPixel = 0; uPixel < DXGL_ARRAY_SIZE(auInput); ++uPixel)
        {
            auInput[uPixel] = (GLuint)uPixel;
        }

        GLuint auTextures[2];
        glGenTextures(2, auTextures);
        glTextureStorage2DEXT(auTextures[0], GL_TEXTURE_2D, 1, GL_RGBA8, 4 * 3, 4 * 3);
        glTextureStorage2DEXT(auTextures[1], GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 4, 4);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        glTextureSubImage2DEXT(auTextures[0], GL_TEXTURE_2D, 0, 0, 0, 4 * 3, 4 * 3, GL_RGBA, GL_UNSIGNED_BYTE, auInput);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_SKIP_ROWS, 0);
        glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        GLuint auFace[16];
        GLuint auOutput[16 * 6];
        bool bCopied(true);
        for (uint32 uFace = 0; uFace < 6; ++uFace)
        {
            uint32 uX((uFace % 3) * 4), uY((uFace / 3) * 4);
            glCopyImageSubData(auTextures[0], GL_TEXTURE_2D, 0, uX, uY, 0, auTextures[1], GL_TEXTURE_CUBE_MAP, 0, 0, 0, uFace, 4, 4, 1);
            glGetTextureImageEXT(auTextures[1], GL_TEXTURE_CUBE_MAP_POSITIVE_X + uFace, 0, GL_RGBA, GL_UNSIGNED_BYTE, auFace);
            for (uint32 uRow = 0; uRow < 4; ++uRow)
            {
                memcpy(auOutput + (uRow + uY) * 4 * 3 + uX, auFace + uRow * 4, 4 * sizeof(GLuint));
            }
        }

        glDeleteTextures(2, auTextures);

        return memcmp(auInput, auOutput, sizeof(auOutput)) == 0;
    }

#endif //DXGL_SUPPORT_GETTEXIMAGE

#define ELEMENT(_Enum) _Enum,

    static const GLenum g_aeMaxTextureUnits[] =
    {
        DXGL_SHADER_TYPE_MAP(ELEMENT,
            GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
            GL_MAX_TEXTURE_IMAGE_UNITS,
            GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,
            GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,
            GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,
            GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS)
        GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
    };

    static const GLenum g_aeMaxUniformBufferUnits[] =
    {
        DXGL_SHADER_TYPE_MAP(ELEMENT,
            GL_MAX_VERTEX_UNIFORM_BLOCKS,
            GL_MAX_FRAGMENT_UNIFORM_BLOCKS,
            GL_MAX_GEOMETRY_UNIFORM_BLOCKS,
            GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,
            GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,
            GL_MAX_COMPUTE_UNIFORM_BLOCKS)
        GL_MAX_UNIFORM_BUFFER_BINDINGS
    };

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS

    static const GLenum g_aeMaxStorageBufferUnits[] =
    {
        DXGL_SHADER_TYPE_MAP(ELEMENT,
            GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
            GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
            GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,
            GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,
            GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,
            GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS)
        GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
    };

#endif

#if DXGL_SUPPORT_SHADER_IMAGES

    static const GLenum g_aeMaxImageUnits[] =
    {
        DXGL_SHADER_TYPE_MAP(ELEMENT,
            GL_MAX_VERTEX_IMAGE_UNIFORMS,
            GL_MAX_FRAGMENT_IMAGE_UNIFORMS,
            GL_MAX_GEOMETRY_IMAGE_UNIFORMS,
            GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,
            GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,
            GL_MAX_COMPUTE_IMAGE_UNIFORMS)
        GL_MAX_IMAGE_UNITS
    };

#endif

#undef ELEMENT

    void DetectResourceUnitCapabilities(SResourceUnitCapabilities* pCapabilities, const GLenum* aeMaxUnits)
    {
        memset(pCapabilities->m_aiMaxPerStage, 0, sizeof(pCapabilities->m_aiMaxPerStage));
        for (uint32 uStage = 0; uStage < eST_NUM; ++uStage)
        {
            glGetIntegerv(aeMaxUnits[uStage], pCapabilities->m_aiMaxPerStage + uStage);
        }

        pCapabilities->m_aiMaxTotal = 0;
        glGetIntegerv(aeMaxUnits[eST_NUM], &pCapabilities->m_aiMaxTotal);
    }

    void DetectContextFeatures(TFeatures& kFeatures, const SCapabilities& kCapabilities, const SVersion& version, const unsigned int driverVendor)
    {
        uint32 glVersion = version.ToUint();
#if DXGLES || defined(DXGL_ES_SUBSET)
        bool gles30orHigher = glVersion >= DXGLES_VERSION_30;
        bool gles31orHigher = glVersion >= DXGLES_VERSION_31;
        bool gles32orHigher = glVersion >= DXGLES_VERSION_32;

        kFeatures.Set(eF_IndexedBoolState, gles31orHigher);
        kFeatures.Set(eF_StencilOnlyFormat, gles31orHigher);
        kFeatures.Set(eF_MultiSampledTextures, gles31orHigher);
        kFeatures.Set(eF_DrawIndirect, gles31orHigher);
        kFeatures.Set(eF_StencilTextures, gles31orHigher);
        kFeatures.Set(eF_AtomicCounters, gles31orHigher);
        kFeatures.Set(eF_DispatchIndirect, gles31orHigher);
        kFeatures.Set(eF_ShaderImages, gles31orHigher);
        kFeatures.Set(eF_TextureViews, gles30orHigher && DXGL_GL_EXTENSION_SUPPORTED(EXT_texture_view) && !DXGL_SUPPORT_NSIGHT_SINCE(4_1) && !DXGL_SUPPORT_VOGL);
        kFeatures.Set(eF_SeparablePrograms, gles31orHigher || DXGL_GL_EXTENSION_SUPPORTED(EXT_separate_shader_objects));
        kFeatures.Set(eF_ComputeShader, gles31orHigher);
        kFeatures.Set(eF_DualSourceBlending, false);
        kFeatures.Set(eF_IndependentBlending, gles32orHigher);
        // glCopyImageSubData causes a crash on Mali GPUs. Disabling it for now.
        kFeatures.Set(eF_CopyImage, (gles32orHigher || DXGL_GL_EXTENSION_SUPPORTED(EXT_copy_image)) && driverVendor != RenderCapabilities::s_gpuVendorIdARM);
        // OpenGLES doesn't support depth clamping but we emulate it by writing the depth in the pixel shader.
        // Unfortunately Qualcomm OpenGL ES 3.0 drivers have a bug and they don't support modifying the depth in the pixel shader.
        kFeatures.Set(eF_DepthClipping, !(glVersion == DXGLES_VERSION_30 && driverVendor == RenderCapabilities::s_gpuVendorIdQualcomm));

        bool isAnisotropicFilteringEnabled = DXGL_GL_EXTENSION_SUPPORTED(EXT_texture_filter_anisotropic);
        kFeatures.Set(eF_TextureAnisotropicFiltering, isAnisotropicFilteringEnabled);

        bool textureBorderClamp = false;
#if defined(DXGL_ES_SUBSET)
        textureBorderClamp = true;
#endif // defined(DXGL_ES_SUBSET)
        kFeatures.Set(eF_TextureBorderClamp, textureBorderClamp || DXGL_GL_EXTENSION_SUPPORTED(EXT_texture_border_clamp));
        kFeatures.Set(eF_DebugOutput, DXGL_GL_EXTENSION_SUPPORTED(KHR_debug));

#else
        // OpenGL
        bool gl32orHigher = glVersion >= DXGL_VERSION_32;
        bool gl41orHigher = glVersion >= DXGL_VERSION_41;
        bool gl42orHigher = glVersion >= DXGL_VERSION_42;
        bool gl43orHigher = glVersion >= DXGL_VERSION_43;
        bool gl44orHigher = glVersion >= DXGL_VERSION_44;

        kFeatures.Set(eF_DepthClipping, true);
        kFeatures.Set(eF_IndexedBoolState, gl32orHigher);
        kFeatures.Set(eF_StencilOnlyFormat, gl32orHigher);
        kFeatures.Set(eF_TextureBorderClamp, gl32orHigher);
        kFeatures.Set(eF_MultiSampledTextures, gl41orHigher);
        kFeatures.Set(eF_DrawIndirect, gl41orHigher);
        kFeatures.Set(eF_SeparablePrograms, gl41orHigher || DXGL_GL_EXTENSION_SUPPORTED(ARB_separate_shader_objects));
        kFeatures.Set(eF_StencilTextures, gl42orHigher);
        kFeatures.Set(eF_AtomicCounters, gl42orHigher);
        kFeatures.Set(eF_DispatchIndirect, gl43orHigher);
        kFeatures.Set(eF_ShaderImages, gl43orHigher);
        kFeatures.Set(eF_VertexAttribBinding, gl43orHigher || DXGL_GL_EXTENSION_SUPPORTED(ARB_vertex_attrib_binding));
        kFeatures.Set(eF_TextureViews, (gl43orHigher || DXGL_GL_EXTENSION_SUPPORTED(ARB_texture_view)) && !DXGL_SUPPORT_NSIGHT_SINCE(4_1) && !DXGL_SUPPORT_VOGL);
        kFeatures.Set(eF_DebugOutput, gl43orHigher || DXGL_GL_EXTENSION_SUPPORTED(KHR_debug));
        kFeatures.Set(eF_ComputeShader, gl43orHigher || DXGL_GL_EXTENSION_SUPPORTED(ARB_compute_shader));
        kFeatures.Set(eF_BufferStorage, gl44orHigher || DXGL_GL_EXTENSION_SUPPORTED(ARB_buffer_storage));
        kFeatures.Set(eF_IndependentBlending, true);
        kFeatures.Set(eF_CopyImage, gl43orHigher);
#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        // Technically dual source blending is supported since OpenGL 3.3 but you need to declare the fragment shader output with the 
        // position and the index (for OpenGL < 4.4): 
        // layout(location = 0, index = 1) out vec4 diffuseColor1; <== SR1 for dual source blending      
        // Unfortunately the DX shader bytecode doesn't distinguish between a normal COLOR1 output or a COLOR1 for dual blending so the HLSL cross compiler
        // doesn't know that it needs to generate a different declaration.
        kFeatures.Set(eF_DualSourceBlending, gl44orHigher);
#else
        kFeatures.Set(eF_DualSourceBlending, gl41orHigher);
#endif // DXGL_GLSL_FROM_HLSLCROSSCOMPILER

#endif //#if DXGLES || defined(DXGL_ES_SUBSET)

#if DXGL_GLSL_FROM_HLSLCROSSCOMPILER
        DXGL_TODO(
            "At the moment HLSLcc does guarantee exact interface matching between programs. "
            "This can lead to pipelines that fail validation or worse with undefined behavior because they have "
            "in one stage user-defined output variables that are ignored by the following stage."
            "Investigate if this can be fixed.")
            kFeatures.Set(eF_SeparablePrograms, false);
#endif

#ifndef AZ_PLATFORM_ANDROID
        DXGL_TODO("Workaround for the NVIDIA 331.113 x64 linux driver crash - investigate");
        kFeatures.Set(eF_VertexAttribBinding, false);
#endif // !AZ_PLATFORM_ANDROID

        DXGL_TODO("Workaround for the multi-threaded GL driver crash - investigate");
        kFeatures.Set(eF_MultiBind, false);
    }

    bool DetectFeaturesAndCapabilities(TFeatures& kFeatures, SCapabilities& kCapabilities, const SVersion& version, const unsigned int driverVendor)
    {
        glGetIntegerv(GL_MAX_SAMPLES, &kCapabilities.m_iMaxSamples);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &kCapabilities.m_iMaxVertexAttribs);
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &kCapabilities.m_iShaderStorageBufferOffsetAlignment);
#endif
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &kCapabilities.m_iUniformBufferOffsetAlignment);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &kCapabilities.m_iMaxUniformBlockSize);

        DetectResourceUnitCapabilities(&kCapabilities.m_akResourceUnits[eRUT_Texture],       g_aeMaxTextureUnits);
        DetectResourceUnitCapabilities(&kCapabilities.m_akResourceUnits[eRUT_UniformBuffer], g_aeMaxUniformBufferUnits);
#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        DetectResourceUnitCapabilities(&kCapabilities.m_akResourceUnits[eRUT_StorageBuffer], g_aeMaxStorageBufferUnits);
#endif
        DetectContextFeatures(kFeatures, kCapabilities, version, driverVendor);
#if DXGL_SUPPORT_SHADER_IMAGES
        if (kFeatures.Get(eF_ShaderImages))
        {
            DetectResourceUnitCapabilities(&kCapabilities.m_akResourceUnits[eRUT_Image], g_aeMaxImageUnits);
        }
#endif

#if DXGL_SUPPORT_VERTEX_ATTRIB_BINDING
        if (kFeatures.Get(eF_VertexAttribBinding))
        {
            glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &kCapabilities.m_iMaxVertexAttribBindings);
            glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &kCapabilities.m_iMaxVertexAttribRelativeOffset);

            if (kCapabilities.m_iMaxVertexAttribBindings > MAX_VERTEX_ATTRIB_BINDINGS)
            {
                kCapabilities.m_iMaxVertexAttribBindings = MAX_VERTEX_ATTRIB_BINDINGS;
            }
        }
#endif

        for (uint32 uGIFormat = 0; uGIFormat < eGIF_NUM; ++uGIFormat)
        {
            kCapabilities.m_auFormatSupport[uGIFormat] = DetectGIFormatSupport((EGIFormat)uGIFormat);
        }
        
        // Assume it works
        kCapabilities.m_bCopyImageWorksOnCubeMapFaces = true;
#if DXGL_SUPPORT_GETTEXIMAGE
        if(kFeatures.Get(eF_CopyImage))
        {
            kCapabilities.m_bCopyImageWorksOnCubeMapFaces = DetectIfCopyImageWorksOnCubeMapFaces();
        }
#endif // DXGL_SUPPORT_GETTEXIMAGE


        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &kCapabilities.m_maxRenderTargets);
        kCapabilities.m_plsSizeInBytes = 0;
#if defined(GL_EXT_shader_pixel_local_storage)
        if (DXGL_GL_EXTENSION_SUPPORTED(EXT_shader_pixel_local_storage))
        {
            glGetIntegerv(GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_FAST_SIZE_EXT, &kCapabilities.m_plsSizeInBytes);
        }
#endif // GL_EXT_shader_pixel_local_storage

#if defined(GL_EXT_shader_framebuffer_fetch)
        if (DXGL_GL_EXTENSION_SUPPORTED(EXT_shader_framebuffer_fetch))
        {
            kCapabilities.m_frameBufferFetchSupport.set(RenderCapabilities::FBF_ALL_COLORS);
            kCapabilities.m_frameBufferFetchSupport.set(RenderCapabilities::FBF_COLOR0);
        }
#endif // GL_EXT_shader_framebuffer_fetch

#if defined(GL_ARM_shader_framebuffer_fetch)
        if (DXGL_GL_EXTENSION_SUPPORTED(ARM_shader_framebuffer_fetch))
        {
            // Check that we can fetch COLOR0 when using multiple render targets.
            GLboolean mrtSupport = GL_FALSE;
            glGetBooleanv(GL_FRAGMENT_SHADER_FRAMEBUFFER_FETCH_MRT_ARM, &mrtSupport);
            if (mrtSupport)
            {
                kCapabilities.m_frameBufferFetchSupport.set(RenderCapabilities::FBF_COLOR0);
            }
        }
#endif // GL_ARM_shader_framebuffer_fetch

#if defined(GL_ARM_shader_framebuffer_fetch_depth_stencil)
        if (DXGL_GL_EXTENSION_SUPPORTED(ARM_shader_framebuffer_fetch_depth_stencil))
        {
            kCapabilities.m_frameBufferFetchSupport.set(RenderCapabilities::FBF_DEPTH);
            kCapabilities.m_frameBufferFetchSupport.set(RenderCapabilities::FBF_STENCIL);
        }
#endif // GL_ARM_shader_framebuffer_fetch_depth_stencil

        return true;
    }

    size_t DetectVideoMemory()
    {
#if DXGL_EXTENSION_LOADER
    #if !DXGLES
        if (DXGL_GL_EXTENSION_SUPPORTED(NVX_gpu_memory_info))
        {
            GLint iVMemKB = 0;
            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &iVMemKB);
            return iVMemKB * 1024;
        }
        else if (DXGL_GL_EXTENSION_SUPPORTED(ATI_meminfo))
        {
            GLint aiTexFreeMemoryInfo[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, aiTexFreeMemoryInfo);
            return aiTexFreeMemoryInfo[0] * 1024;
        }
    #else
        DXGL_TODO("Not yet implemented for GLES");
    #endif
        return 0;
#elif defined(MAC)
        return static_cast<size_t>(GetVRAMForDisplay(0));
#elif defined(IOS)
        DXGL_TODO("Not yet implemented for iOS")
        return 0;
#elif defined(DXGL_USE_EGL)
        DXGL_TODO("Not yet implemented for EGL");
        return 0;
#else
#error "Not implemented on this platform"
#endif
    }

    unsigned int DetectDriverVendor(const char* szVendorName)
    {
        struct
        {
            uint16 m_uPCIID;
            const char* m_szName;
        } akKnownVendors[] =
        {
            { RenderCapabilities::s_gpuVendorIdNVIDIA, "NVIDIA Corporation" },
            { RenderCapabilities::s_gpuVendorIdNVIDIA, "Nouveau" },
            { RenderCapabilities::s_gpuVendorIdAMD, "ATI Technologies Inc." },
            { RenderCapabilities::s_gpuVendorIdAMD, "Advanced Micro Devices, Inc." },
            { RenderCapabilities::s_gpuVendorIdIntel, "Intel" },
            { RenderCapabilities::s_gpuVendorIdIntel, "Intel Inc." },
            { RenderCapabilities::s_gpuVendorIdIntel, "Intel Corporation" },
            { RenderCapabilities::s_gpuVendorIdIntel, "Intel Open Source Technology Center" },
            { RenderCapabilities::s_gpuVendorIdQualcomm, "Qualcomm" },
            { RenderCapabilities::s_gpuVendorIdARM, "ARM" },
            // Rally US2888 - VendorID detection for Imagination, Samsung, etc.
        };

        for (uint32 uVendor = 0; uVendor < DXGL_ARRAY_SIZE(akKnownVendors); ++uVendor)
        {
            if (azstricmp(szVendorName, akKnownVendors[uVendor].m_szName) == 0)
            {
                return akKnownVendors[uVendor].m_uPCIID;
            }
        }

        return 0;
    }

#if DXGL_EXTENSION_LOADER
    bool LoadEarlyGLEntryPoints()
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(DXGL_USE_LOADER_GLAD)
#   if defined(DXGL_USE_EGL)
        if (gladLoaderLoadEGL(NULL) == 0)
        {
            DXGL_ERROR("Failed to retrieve EGL entry points");
            return false;
        }
#   endif
#else
#error "Not implemented on this platform"
#endif
        return true;
    }

    bool LoadGLEntryPoints(SDummyContext& kDummyContext)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(DXGL_USE_LOADER_GLAD)
#   if defined(DXGL_USE_EGL)
        if (gladLoaderLoadEGL(NULL) == 0)
        {
            DXGL_ERROR("Failed to retrieve EGL entry points");
            return false;
        }
#   elif defined(DXGL_USE_GLX)
        if (gladLoaderLoadGLX(NULL, 0) == 0)
        {
            DXGL_ERROR("Failed to retrieve GLX entry points");
            return false;
        }
#   endif

#if defined(OPENGL_ES)
        int ret = gladLoaderLoadGLES2();
#else
        int ret = gladLoaderLoadGL();
#endif //defined(OPENGL_ES)
        
        if (ret == 0)
        {
            DXGL_ERROR("Failed to retrieve GL entry points");
            return false;
        }

#   if defined(DXGL_USE_WGL)
        if (gladLoaderLoadWGL(kDummyContext.m_kDummyWindow.m_kNativeDisplay) == 0)
        {
            DXGL_ERROR("Failed to retrieve WGL entry points");
            return false;
        }
#   endif 

#elif defined(DXGL_USE_LOADER_GLEW)
        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            DXGL_ERROR("Failed to init GLEW. Error %s", glewGetErrorString(err));
            return false;
        }
#   if defined(DXGL_USE_WGL)
        err = wglewInit();
        if (GLEW_OK != err)
        {
            DXGL_ERROR("Failed to init WGL GLEW. Error %s", glewGetErrorString(err));
            return false;
        }
#   elif defined(DXGL_USE_GLX)
        err = glxewInit();
        if (GLEW_OK != err)
        {
            DXGL_ERROR("Failed to init WGL GLEW. Error %s", glewGetErrorString(err));
            return false;
        }
#   endif
#else
#error "Not implemented on this platform"
#endif
        return true;
    }
#endif //DXGL_EXTENSION_LOADER
    bool GetGLVersion(SAdapterPtr& pAdapter)
    {
        if (!pAdapter)
        {
            return false;
        }

        O3de::OpenGL::ClearErrors();
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        AZ_Assert(major >=0, "Invalid OpenGL major version %d", major);
        AZ_Assert(minor >= 0, "Invalid OpenGL minor version %d", minor);

        pAdapter->m_sVersion.m_uMajorVersion = static_cast<uint32>(major);
        pAdapter->m_sVersion.m_uMinorVersion = static_cast<uint32>(minor);
        return O3de::OpenGL::CheckError() == GL_NO_ERROR;
    }

    bool ParseExtensions(SAdapterPtr& pAdapter)
    {
        if (!pAdapter)
        {
            return false;
        }

        int num = 0, index;
        bool result = true;
        O3de::OpenGL::ClearErrors();
        glGetIntegerv(GL_NUM_EXTENSIONS, &num);
        for (index = 0; index < num; ++index)
        {
            const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, index));
            if (!extension)
            {
                result = false;
                AZ_Warning("Renderer", false, "Failed to get extension %d for adapter %s %s", index, pAdapter->m_strVendor.c_str(), pAdapter->m_strRenderer.c_str());
                continue;
            }

            pAdapter->AddExtension(extension);
        }

        return result && O3de::OpenGL::CheckError() == GL_NO_ERROR;
    }

    bool DetectAdapters(std::vector<SAdapterPtr>& kAdapters)
    {
        // Linux needs access to EGL much earlier than other EGL platforms do, so the EGL entry points are loaded in CreateWindow
#if DXGL_EXTENSION_LOADER && !defined(AZ_PLATFORM_LINUX)
        if (!LoadEarlyGLEntryPoints())
        {
            return false;
        }
#endif // DXGL_EXTENSION_LOADER

        SDummyContext kDummyContext;
        if (!kDummyContext.Initialize())
        {
            return false;
        }

#if DXGL_EXTENSION_LOADER
        if (!LoadGLEntryPoints(kDummyContext))
        {
            return false;
        }
#endif // DXGL_EXTENSION_LOADER

        SAdapterPtr spAdapter(new SAdapter);
        spAdapter->m_strRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        spAdapter->m_strVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        spAdapter->m_strVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        spAdapter->m_uVRAMBytes = DetectVideoMemory();
        spAdapter->m_eDriverVendor = DetectDriverVendor(spAdapter->m_strVendor.c_str());
        spAdapter->m_sVersion.m_uMajorVersion = 0;
        spAdapter->m_sVersion.m_uMinorVersion = 0;
        bool result = GetGLVersion(spAdapter);
        AZ_Warning("Renderer", result, "Failed to get the OpenGL version for adapter %s %s", spAdapter->m_strVendor.c_str(), spAdapter->m_strRenderer.c_str());
        result = ParseExtensions(spAdapter);
        AZ_Warning("Renderer", result, "Failed to parse OpenGL Extensions for adapter %s %s", spAdapter->m_strVendor.c_str(), spAdapter->m_strRenderer.c_str());

        if (gEnv->pRenderer)
        {
            gEnv->pRenderer->SetApiVersion(spAdapter->m_strVersion.c_str());
        }

        if (!DetectFeaturesAndCapabilities(spAdapter->m_kFeatures, spAdapter->m_kCapabilities, spAdapter->m_sVersion, spAdapter->m_eDriverVendor))
        {
            return false;
        }
        kAdapters.push_back(spAdapter);
        return true;
    }

    bool CheckAdapterCapabilities(const SAdapter& kAdapter, AZStd::string* pErrorMsg /*=nullptr*/)
    {
        // Check the openGL(ES) version
        uint32 version = kAdapter.m_sVersion.ToUint();
        uint32 requiredVersion;
#if DXGLES
        requiredVersion = DXGLES_REQUIRED_VERSION;
#else
        requiredVersion = DXGL_REQUIRED_VERSION;
#endif

        if (version < requiredVersion)
        {
            if (pErrorMsg)
            {
                *pErrorMsg = AZStd::string::format("Device %s %s doesn't support the minimum version needed of OpenGL (ES). Required %u, found %u.", kAdapter.m_strVendor.c_str(),
                                                                                                                                                kAdapter.m_strRenderer.c_str(),
                                                                                                                                                requiredVersion, version);
            }
            return false;
        }

        int maxBufferUniform = kAdapter.m_kCapabilities.m_akResourceUnits[eRUT_UniformBuffer].m_aiMaxTotal;
        if (maxBufferUniform < MIN_UNIFORM_BUFFERS_REQUIRED)
        {
            if (pErrorMsg)
            {
                *pErrorMsg = AZStd::string::format("Device %s %s doesn't support enough uniform buffers. Required %d, found %d",  kAdapter.m_strVendor.c_str(), kAdapter.m_strRenderer.c_str(), 
                                                                                                                                MIN_UNIFORM_BUFFERS_REQUIRED, maxBufferUniform);
            }
            return false;
        }
                
        return true;
    }

    bool DetectOutputs(const SAdapter& kAdapter, std::vector<SOutputPtr>& kOutputs)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        uint32 uDisplay(0);
        DISPLAY_DEVICEA kDisplayDevice;
        ZeroMemory(&kDisplayDevice, sizeof(kDisplayDevice));
        kDisplayDevice.cb = sizeof(kDisplayDevice);
        while (EnumDisplayDevicesA(NULL, uDisplay, &kDisplayDevice, 0))
        {
            SOutputPtr spOutput(new SOutput);
            spOutput->m_strDeviceID = kDisplayDevice.DeviceID;
            spOutput->m_strDeviceName = kDisplayDevice.DeviceName;

            DEVMODEA kDevMode;
            ZeroMemory(&kDevMode, sizeof(kDevMode));
            kDevMode.dmSize = sizeof(kDevMode);

            SDisplayMode kDisplayMode;
            uint32 uModeID(0);
            while (EnumDisplaySettingsA(kDisplayDevice.DeviceName, uModeID, &kDevMode))
            {
                DevModeToDisplayMode(&kDisplayMode, kDevMode);
                ++uModeID;

                spOutput->m_kModes.push_back(kDisplayMode);
            }

            if (!spOutput->m_kModes.empty())
            {
                if (!EnumDisplaySettingsA(kDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &kDevMode))
                {
                    DXGL_ERROR("Could not retrieve the desktop display mode mode for display %d", uDisplay);
                    return false;
                }
                DevModeToDisplayMode(&spOutput->m_kDesktopMode, kDevMode);

                kOutputs.push_back(spOutput);
            }
            ++uDisplay;
        }
        return true;
#elif defined(ANDROID)
        ANativeWindow* nativeWindow = AZ::Android::Utils::GetWindow();
        if (!nativeWindow)
        {
            DXGL_ERROR("Failed to get native window");
            return false;
        }

        int widthPixels, heightPixels;
        if (!AZ::Android::Utils::GetWindowSize(widthPixels, heightPixels))
        {
            DXGL_ERROR("Failed to get window size");
            return false;
        }

        gcpRendD3D->GetClampedWindowSize(widthPixels, heightPixels);

        SDisplayMode mode;
        mode.m_uWidth = static_cast<uint32>(widthPixels);
        mode.m_uHeight = static_cast<uint32>(heightPixels);
        mode.m_uFrequency = 0;
        mode.m_nativeFormat = ANativeWindow_getFormat(nativeWindow);

        SOutputPtr output(new SOutput());
        output->m_strDeviceID = "0";
        output->m_strDeviceName = "Main Output";
        output->m_kModes.push_back(mode);
        output->m_kDesktopMode = mode;
        kOutputs.push_back(output);
        return true;
#elif defined(AZ_PLATFORM_LINUX)
        // TODO Linux - Query window dims from adapter
        int widthPixels = 1280;
        int heightPixels = 720;
        gcpRendD3D->GetClampedWindowSize(widthPixels, heightPixels);

        SDisplayMode mode;
        mode.m_uWidth = static_cast<uint32>(widthPixels);
        mode.m_uHeight = static_cast<uint32>(heightPixels);
        mode.m_uFrequency = 0;

        SOutputPtr output(new SOutput());
        output->m_strDeviceID = "0";
        output->m_strDeviceName = "Main Output";
        output->m_kModes.push_back(mode);
        output->m_kDesktopMode = mode;
        kOutputs.push_back(output);
        return true;
#else
        DXGL_NOT_IMPLEMENTED;
        return false;
#endif
    }

    bool CheckFormatMultisampleSupport(SAdapter* pAdapter, EGIFormat eFormat, uint32 uNumSamples)
    {
        return
            uNumSamples <= pAdapter->m_kCapabilities.m_iMaxSamples;
    }

    void GetDXGIModeDesc(DXGI_MODE_DESC* pDXGIModeDesc, const SDisplayMode& kDisplayMode)
    {
        pDXGIModeDesc->Width = kDisplayMode.m_uWidth;
        pDXGIModeDesc->Height = kDisplayMode.m_uHeight;
        pDXGIModeDesc->RefreshRate.Numerator = kDisplayMode.m_uFrequency;
        pDXGIModeDesc->RefreshRate.Denominator = 1;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        DXGL_TODO("Check if there is a better way of mapping GL display modes to formats")
        switch (kDisplayMode.m_uBitsPerPixel)
        {
        case 32:
            pDXGIModeDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case 64:
            pDXGIModeDesc->Format = DXGI_FORMAT_R16G16B16A16_UNORM;
            break;
        default:
            pDXGIModeDesc->Format = DXGI_FORMAT_UNKNOWN;
            break;
        }
#elif defined(ANDROID)
        switch (kDisplayMode.m_nativeFormat)
        {
        case WINDOW_FORMAT_RGBA_8888:
            pDXGIModeDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case WINDOW_FORMAT_RGBX_8888:
            pDXGIModeDesc->Format = DXGI_FORMAT_B8G8R8X8_UNORM;
            break;
        case WINDOW_FORMAT_RGB_565:
            pDXGIModeDesc->Format = DXGI_FORMAT_B5G6R5_UNORM;
            break;
        default:
            pDXGIModeDesc->Format = DXGI_FORMAT_UNKNOWN;
            break;
        }
#elif defined(AZ_PLATFORM_LINUX)
        // Do nothing?
#else
        DXGL_NOT_IMPLEMENTED;
#endif

        pDXGIModeDesc->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        pDXGIModeDesc->Scaling =  DXGI_MODE_SCALING_UNSPECIFIED;
    }

    bool GetDisplayMode(SDisplayMode* pDisplayMode, const DXGI_MODE_DESC& kDXGIModeDesc)
    {
        pDisplayMode->m_uWidth = kDXGIModeDesc.Width;
        pDisplayMode->m_uHeight = kDXGIModeDesc.Height;
        if (kDXGIModeDesc.RefreshRate.Denominator != 0)
        {
            pDisplayMode->m_uFrequency = kDXGIModeDesc.RefreshRate.Numerator / kDXGIModeDesc.RefreshRate.Denominator;
        }
        else
        {
            pDisplayMode->m_uFrequency = 0;
        }
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GLDEVICE_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(GLDevice_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
        switch (kDXGIModeDesc.Format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            pDisplayMode->m_uBitsPerPixel = 32;
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            pDisplayMode->m_uBitsPerPixel = 64;
            break;
        default:
        {
            EGIFormat eGIFormat(GetGIFormat(kDXGIModeDesc.Format));
            const SGIFormatInfo* pFormatInfo;
            if (eGIFormat == eGIF_NUM ||
                (pFormatInfo = GetGIFormatInfo(eGIFormat)) == NULL ||
                pFormatInfo->m_pUncompressed == NULL)
            {
                DXGL_ERROR("Invalid DXGI format for display mode");
                return false;
            }
            pDisplayMode->m_uBitsPerPixel = pFormatInfo->m_pUncompressed->GetPixelBits();
        }
        break;
        }
#elif defined(ANDROID)
        switch (kDXGIModeDesc.Format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            pDisplayMode->m_nativeFormat = WINDOW_FORMAT_RGBA_8888;
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            pDisplayMode->m_nativeFormat = WINDOW_FORMAT_RGBX_8888;
            break;
        case DXGI_FORMAT_B5G6R5_UNORM:
            pDisplayMode->m_nativeFormat = WINDOW_FORMAT_RGB_565;
            break;
        default:
            AZ_Assert(false, "Invalid DXGI_MODE_DESC format %d", kDXGIModeDesc.Format);
            return false;
        }
#endif
        DXGL_TODO("Consider scanline order and scaling if possible");

        return true;
    }

#if DXGL_DEBUG_OUTPUT_VERBOSITY

    void DXGL_DEBUG_CALLBACK_CONVENTION DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, void* userParam)
    {
        //  this filters out the debug messages earlier saving the performance which might be broken by excessive string creation.
        if ((type == GL_DEBUG_SEVERITY_LOW) || (type == GL_DEBUG_SEVERITY_NOTIFICATION))
        {
            return;
        }

        ::string errorMessage, sourceStr, typeStr, severityStr;
        ELogSeverity eLogSeverity = eLS_Warning;

        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
            sourceStr = "OpenGL";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            sourceStr = "Windows";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            sourceStr = "Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            sourceStr = "Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            sourceStr = "Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            sourceStr = "Other";
            break;
        }

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeStr = "Deprecated behavior";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeStr = "Undefined behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeStr = "Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeStr = "Performance";
            break;
        case GL_DEBUG_TYPE_OTHER:
            typeStr = "Other";
            break;
        }

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            severityStr = "High";
            eLogSeverity = eLS_Error;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severityStr = "Medium";
            eLogSeverity = eLS_Warning;
            break;
        case GL_DEBUG_SEVERITY_LOW:
            severityStr = "Low";
            eLogSeverity = eLS_Info;
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severityStr = "Info";
            eLogSeverity = eLS_Info;
            break;
        }
        
        //Anyone needing more information on OpenGL rendering in non-debug builds should enable this section of code for additional information
        //It's DEBUG only to help obtain a cleaner log on some Android devices which would otherwise be inundated with messages from this section of code
#if defined(DEBUG) 
        if (eLogSeverity != eLS_Info)
        {
            errorMessage.Format("OpenGLError:\nSource: %s\nType: %s\nId: %i\nSeverity: %s\nMessage: %s\n", sourceStr.c_str(), typeStr.c_str(), id, severityStr.c_str(), message);
            NCryOpenGL::LogMessage(eLogSeverity,  errorMessage.c_str());
        }
#endif
    }

#endif //DXGL_DEBUG_OUTPUT_VERBOSITY

#if DXGL_TRACE_CALLS

    void CallTracePrintf(const char* szFormat, ...)
    {
        CDevice* pDevice(CDevice::GetCurrentDevice());
        if (pDevice == NULL)
        {
            return;
        }

        CContext* pCurrentContext(pDevice->GetCurrentContext());
        if (pCurrentContext == NULL)
        {
            return;
        }

        char acBuffer[512];
        va_list kArgs;
        va_start(kArgs, szFormat);
        vsprintf_s(acBuffer, szFormat, kArgs);
        va_end(kArgs);

        pCurrentContext->CallTraceWrite(acBuffer);
    }

    void CallTraceFlush()
    {
        CDevice* pDevice(CDevice::GetCurrentDevice());
        if (pDevice == NULL)
        {
            return;
        }

        CContext* pCurrentContext(pDevice->GetCurrentContext());
        if (pCurrentContext == NULL)
        {
            return;
        }

        pCurrentContext->CallTraceFlush();
    }

#endif

#if DXGL_CHECK_ERRORS

    void CheckErrors()
    {
        enum
        {
            MAX_ERROR_QUERIES = 4
        };
        uint32 uNumQueries(0);
        GLenum eErrorCode;
        while ((eErrorCode = DXGL_UNWRAPPED_FUNCTION(glGetError)()) != GL_NO_ERROR)
        {
            const char* szName;
            const char* szMessage;
            switch (eErrorCode)
            {
#if defined(WIN32)
            case GL_CONTEXT_LOST:
                szName = "GL_CONTEXT_LOST";
                szMessage = "Context has been lost and reset by the driver";
                break;
#endif
            case GL_INVALID_ENUM:
                szName = "GL_INVALID_ENUM";
                szMessage = "Enum argument out of range";
                break;
            case GL_INVALID_VALUE:
                szName = "GL_INVALID_VALUE";
                szMessage = "Numeric argument out of range";
                break;
            case GL_INVALID_OPERATION:
                szName = "GL_INVALID_OPERATION";
                szMessage = "Operation illegal in current state";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                szName = "GL_INVALID_FRAMEBUFFER_OPERATION";
                szMessage = "Framebuffer object is not complete";
                break;
            case GL_OUT_OF_MEMORY:
                szName = "GL_OUT_OF_MEMORY";
                szMessage = "Not enough memory left to execute command";
                break;
            case GL_STACK_OVERFLOW:
                szName = "GL_STACK_OVERFLOW";
                szMessage = "Command would cause a stack overflow";
                break;
            case GL_STACK_UNDERFLOW:
                szName = "GL_STACK_UNDERFLOW";
                szMessage = "Command would cause a stack underflow";
                break;
            default:
                szName = "?";
                szMessage = "Unknown GL error";
                break;
            }
            DXGL_ERROR("GL error: %s (0x%04X) - %s", szName, eErrorCode, szMessage);
            if (++uNumQueries > MAX_ERROR_QUERIES)
            {
                DXGL_ERROR("GL error limit reached - probably no context set");
                break;
            }
        }
    }

#endif //DXGL_CHECK_ERRORS
} // namespace NCryOpenGL

namespace O3de
{
    namespace OpenGL
    {
#if defined(LY_ENABLE_OPENGL_ERROR_CHECKING)
        unsigned int CheckError()
        {
            GLenum errorCode = glGetError();
            while (errorCode != GL_NO_ERROR)
            {
                ::string errorMessage = string().Format("OpenGL Error: [0x%08x]\n!", errorCode);
                NCryOpenGL::LogMessage(NCryOpenGL::ELogSeverity::eLS_Warning,  errorMessage.c_str());
                errorCode = glGetError();
            }
            return errorCode;
        }

        void ClearErrors()
        {
            while (glGetError() != GL_NO_ERROR);
        }
#endif
    } // namespace OpenGL
} // namespace O3de
