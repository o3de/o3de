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

#pragma once

#include <winuser.h>
#include <EMotionFX/Rendering/OpenGL2/Source/RenderGLConfig.h>
#include <EMotionFX/Source/EMotionFXManager.h>

// global mouse handling variables
bool                gAltPressed                 = false;
bool                gLeftMouseButtonPressed     = false;
bool                gRightMouseButtonPressed    = false;
bool                gMiddleMouseButtonPressed   = false;
int32               gMousePosX                  = MCORE_INVALIDINDEX32;
int32               gMousePosY                  = MCORE_INVALIDINDEX32;
int32               gMouseScreenPosX            = MCORE_INVALIDINDEX32;
int32               gMouseScreenPosY            = MCORE_INVALIDINDEX32;
int32               gLastMousePosX              = MCORE_INVALIDINDEX32;
int32               gLastMousePosY              = MCORE_INVALIDINDEX32;
int32               gMouseDeltaX                = MCORE_INVALIDINDEX32;
int32               gMouseDeltaY                = MCORE_INVALIDINDEX32;
int32               gFrameBasedMouseDeltaX      = MCORE_INVALIDINDEX32;
int32               gFrameBasedMouseDeltaY      = MCORE_INVALIDINDEX32;
int32               gFrameBasedLastMousePosX    = MCORE_INVALIDINDEX32;
int32               gFrameBasedLastMousePosY    = MCORE_INVALIDINDEX32;
AZStd::string       gString;
wchar_t             gTitle[1024];

// structure for keyboard stuff, holds true/false for each key
typedef struct
{
    bool keyDown [256];
} Keys;

// contains Information vital to applications
typedef struct
{
    HINSTANCE       hInstance;
    #ifdef UNICODE
    wchar_t*        className;
    #else
    char*           className;
    #endif
} Application;

// window creation info
typedef struct
{
    Application*        application;
    #ifdef UNICODE
    wchar_t*            title;
    #else
    char*               title;
    #endif
    int                 width;
    int                 height;
    int                 bitsPerPixel;
    bool                isFullScreen;
} GL_WindowInit;


// contains information vital to a window
typedef struct
{
    Keys*               keys;
    HWND                hWnd;
    HDC                 hDC;
    HGLRC               hRC;
    GL_WindowInit       init;
    bool                isVisible;
} GL_Window;

// function forward declaration
bool DestroyWindowGL(GL_Window* window);

// global windows related variables
#define         WM_TOGGLEFULLSCREEN (WM_USER + 1)
static bool     gIsProgramLooping;
static bool     gCreateFullScreen;
bool            gARBMultisampleSupported            = false;
int             gARBMultisampleFormat               = 0;
GL_Window*      gWindow                             = nullptr;


// initialize multisampling, ssed to query the multisample frequencies
bool InitMultisample(HINSTANCE hInstance, HWND hWnd, PIXELFORMATDESCRIPTOR pfd)
{
    /*  if (GLEW_ARB_multisample == false)
        {
            gARBMultisampleSupported=false;
            return false;
        }*/

    // get our pixel format
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    if (!wglChoosePixelFormatARB)
    {
        gARBMultisampleSupported = false;
        return false;
    }

    // get our current device context
    HDC hDC = GetDC(hWnd);

    int     pixelFormat;
    int     valid;
    UINT    numFormats;
    float   fAttributes[] = {0, 0};
    int iAttributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 16,
        WGL_STENCIL_BITS_ARB, 0,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, 4,
        0, 0
    };

    // first we check to see if we can get a pixel format for 4 samples
    valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
    if (valid && numFormats >= 1)
    {
        gARBMultisampleSupported = true;
        gARBMultisampleFormat = pixelFormat;
        return gARBMultisampleSupported;
    }

    // our pixel format with 4 samples failed, test for 2 samples
    iAttributes[19] = 2;
    valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);
    if (valid && numFormats >= 1)
    {
        gARBMultisampleSupported = true;
        gARBMultisampleFormat = pixelFormat;
        return gARBMultisampleSupported;
    }

    return gARBMultisampleSupported;
}


// terminate the application
void TerminateApplication(GL_Window* window)
{
    PostMessage(window->hWnd, WM_QUIT, 0, 0);
    gIsProgramLooping = false;
}


// called when the window got resized
void Resize(uint32 width, uint32 height)
{
    //MCore::LogInfo("Resizing to %dx%d", width, height);

    gScreenWidth    = width;
    gScreenHeight   = height;

    // adjust the OpenGL viewport
    glViewport(0, 0, gScreenWidth, gScreenHeight);

    // pass the new window dimensions to the rendering engine
    if (gEngine != NULL)
    {
        gEngine->GetGBuffer()->Resize(gScreenWidth, gScreenHeight);
        gEngine->Resize(gScreenWidth, gScreenHeight);
    }
}


// change the screen resolution
bool ChangeScreenResolution(int width, int height, int bitsPerPixel)
{
    DEVMODE dmScreenSettings;                                           // device mode
    ZeroMemory (&dmScreenSettings, sizeof (DEVMODE));                   // make sure memory is cleared
    dmScreenSettings.dmSize             = sizeof (DEVMODE);             // size of the devmode structure
    dmScreenSettings.dmPelsWidth        = width;                        // select screen width
    dmScreenSettings.dmPelsHeight       = height;                       // select screen height
    dmScreenSettings.dmBitsPerPel       = bitsPerPixel;                 // select bits per pixel
    dmScreenSettings.dmFields           = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    if (ChangeDisplaySettings (&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
        return false;                                                   // display change failed
    }
    return true;                                                        // display change was successful
}


// create the OpenGL window
bool CreateWindowGL(GL_Window* window)
{
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    DWORD windowExtendedStyle = WS_EX_APPWINDOW;

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof (PIXELFORMATDESCRIPTOR),
        1,                                                              // version number
        PFD_DRAW_TO_WINDOW |                                            // format must support window
        PFD_SUPPORT_OPENGL |                                            // format must support OpenGL
        PFD_DOUBLEBUFFER,                                               // must support double buffering
        PFD_TYPE_RGBA,                                                  // request an RGBA format
        window->init.bitsPerPixel,                                      // select our color depth
        0, 0, 0, 0, 0, 0,                                               // color bits ignored
        1,                                                              // alpha buffer
        0,                                                              // shift bit ignored
        0,                                                              // no accumulation buffer
        0, 0, 0, 0,                                                     // accumulation bits ignored
        16,                                                             // 16Bit Z-Buffer (Depth Buffer)
        0,                                                              // no stencil buffer
        0,                                                              // no auxiliary buffer
        PFD_MAIN_PLANE,                                                 // main drawing layer
        0,                                                              // reserved
        0, 0, 0                                                         // layer masks ignored
    };

    RECT windowRect = {0, 0, window->init.width, window->init.height};
    GLuint PixelFormat;

    if (window->init.isFullScreen == true)
    {
        if (ChangeScreenResolution(window->init.width, window->init.height, window->init.bitsPerPixel) == false)
        {
            // fullscreen mode failed, run in windowed mode instead
            MessageBoxA(HWND_DESKTOP, "Mode switch failed.\nRunning in windowed mode.", "Error", MB_OK | MB_ICONEXCLAMATION);
            window->init.isFullScreen = false;
        }
        else
        {
            ShowCursor(false);                                          // turn off the cursor
            windowStyle = WS_POPUP;                                     // set the WindowStyle to WS_POPUP (Popup Window)
            windowExtendedStyle |= WS_EX_TOPMOST;                       // set the extended window style to WS_EX_TOPMOST
        }
    }
    else
    {
        // adjust window, account for window borders
        AdjustWindowRectEx(&windowRect, windowStyle, 0, windowExtendedStyle);
    }

    // create the OpenGL window
    window->hWnd = CreateWindowEx(windowExtendedStyle,                  // extended style
            window->init.application->className,                        // class name
            window->init.title,                                         // window title
            windowStyle,                                                // window Ssyle
            0, 0,                                                       // window X,Y position
            windowRect.right - windowRect.left,                         // window width
            windowRect.bottom - windowRect.top,                         // window height
            HWND_DESKTOP,                                               // desktop is window's parent
            0,                                                          // no menu
            window->init.application->hInstance,                        // pass the window instance
            window);

    if (window->hWnd == 0)
    {
        return false;
    }

    window->hDC = GetDC(window->hWnd);
    if (window->hDC == 0)
    {
        DestroyWindow(window->hWnd);                                    // destroy the window
        window->hWnd = 0;                                               // zero the window handle
        return false;
    }

    // our first pass, multisampling hasn't been created yet, so we create a window normally
    // if it is supported, then we're on our second pass
    // that means we want to use our pixel format for sampling
    // so set PixelFormat to gARBMultisampleFormat instead
    if (gARBMultisampleSupported == false /* || USE_MULTISAMPLING == false*/)
    {
        PixelFormat = ChoosePixelFormat (window->hDC, &pfd);                // find a compatible pixel format
        if (PixelFormat == 0)                                               // did we find a compatible format?
        {
            ReleaseDC(window->hWnd, window->hDC);                           // release our device context
            window->hDC = 0;                                                // zero the device context
            DestroyWindow (window->hWnd);                                   // destroy the window
            window->hWnd = 0;                                               // zero the window handle
            return false;
        }
    }
    else
    {
        PixelFormat = gARBMultisampleFormat;
    }

    // try to set the pixel format
    if (SetPixelFormat(window->hDC, PixelFormat, &pfd) == false)
    {
        ReleaseDC(window->hWnd, window->hDC);                           // release our device context
        window->hDC = 0;                                                // zero the device context
        DestroyWindow(window->hWnd);                                    // destroy the window
        window->hWnd = 0;                                               // zero the window handle
        return false;
    }

    // try to get the rendering context
    window->hRC = wglCreateContext(window->hDC);
    if (window->hRC == 0)
    {
        ReleaseDC(window->hWnd, window->hDC);                           // release our device context
        window->hDC = 0;                                                // zero the device context
        DestroyWindow(window->hWnd);                                    // destroy the window
        window->hWnd = 0;                                               // zero the window handle
        return false;
    }

    // make the current rendering context our rendering context
    if (wglMakeCurrent(window->hDC, window->hRC) == false)
    {
        // Failed
        wglDeleteContext(window->hRC);                                  // delete the rendering context
        window->hRC = 0;                                                // zero the rendering context
        ReleaseDC (window->hWnd, window->hDC);                          // release our device context
        window->hDC = 0;                                                // zero the device context
        DestroyWindow(window->hWnd);                                    // destroy the window
        window->hWnd = 0;                                               // zero the window handle
        return false;
    }


    //if (WGL_EXT_swap_control)
    {
        // Extension is supported, init pointers.
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
        {
            // this is another function from WGL_EXT_swap_control extension
            wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

            // turn off v-sync
            wglSwapIntervalEXT(0);
        }
    }


    // now that our window is created, we want to query what samples are available
    // we call our InitMultiSample window
    // if we return a valid context, we want to destroy our current window
    // and create a new one using the multisample interface.
    if (gARBMultisampleSupported == false)
    {
        if (InitMultisample(window->init.application->hInstance, window->hWnd, pfd))
        {
            DestroyWindowGL(window);
            return CreateWindowGL(window);
        }
    }

    ShowWindow(window->hWnd, SW_NORMAL);
    window->isVisible = true;
    //Resize(window->init.width, window->init.height);
    ZeroMemory(window->keys, sizeof(Keys));

    return true;
}


// destroy the OpenGL window
bool DestroyWindowGL(GL_Window* window)
{
    if (window->hWnd != 0)
    {
        if (window->hDC != 0)
        {
            wglMakeCurrent(window->hDC, 0);
            if (window->hRC != 0)
            {
                wglDeleteContext (window->hRC);
                window->hRC = 0;
            }
            ReleaseDC(window->hWnd, window->hDC);
            window->hDC = 0;
        }
        DestroyWindow(window->hWnd);
        window->hWnd = 0;
    }

    if (window->init.isFullScreen)
    {
        ChangeDisplaySettings (NULL, 0);
        ShowCursor (true);
    }
    return true;
}


// window message procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get The Window Context
    GL_Window* window = gWindow;
    /*#ifdef MCORE_PLATFORM_WINDOWS
        GL_Window* window = (GL_Window*)( GetWindowLong(hWnd, GWLP_USERDATA) );
    #else
        GL_Window* window = (GL_Window*)( GetWindowLong(hWnd, GWL_USERDATA) );
    #endif*/

    switch (uMsg)
    {
    // system commands
    case WM_SYSCOMMAND:
    {
        switch (wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }
        break;
    }

    // window creation
    case WM_CREATE:
    {
        //CREATESTRUCT* creation = (CREATESTRUCT*)(lParam);
        //window = (GL_Window*)(creation->lpCreateParams);

            #ifdef MCORE_PLATFORM_WINDOWS
        SetWindowLong(hWnd, GWLP_USERDATA, (LONG)(window));
            #else
        SetWindowLong(hWnd, GWL_USERDATA, (LONG)(window));
            #endif
    }
        return 0;

    // closing the window
    case WM_CLOSE:
        TerminateApplication(window);
        return 0;

    // resizing
    case WM_SIZE:
        switch (wParam)
        {
        // window minimized event
        case SIZE_MINIMIZED:
            window->isVisible = false;
            return 0;

        // window maximized event
        case SIZE_MAXIMIZED:
            window->isVisible = true;
            Resize(LOWORD (lParam), HIWORD (lParam));
            return 0;

        // window got restored
        case SIZE_RESTORED:
            window->isVisible = true;
            Resize(LOWORD (lParam), HIWORD (lParam));
            return 0;
        }
        break;

    // fired when the mouse leaves the client window
    case WM_MOUSELEAVE:
    {
        SendMessage(gWindow->hWnd, WM_LBUTTONUP, 0, 0);
        SendMessage(gWindow->hWnd, WM_MBUTTONUP, 0, 0);
        SendMessage(gWindow->hWnd, WM_RBUTTONUP, 0, 0);
        break;
    }

    // mouse movement
    case WM_MOUSEMOVE:
    {
        if (gMousePosX != MCORE_INVALIDINDEX32)
        {
            gLastMousePosX = gMousePosX;
        }
        if (gMousePosY != MCORE_INVALIDINDEX32)
        {
            gLastMousePosY = gMousePosY;
        }

        // get mouse local window position
        gMousePosX = LOWORD(lParam);
        gMousePosY = HIWORD(lParam);

        // get mouse global screen position
        RECT windowRect;
        GetWindowRect(gWindow->hWnd, &windowRect);
        gMouseScreenPosX = gMousePosX + windowRect.left;
        gMouseScreenPosY = gMousePosY + windowRect.top;

        // enable leave event which gets fired when the mouse leaves the client window
        TRACKMOUSEEVENT trackMouseEvent;
        trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
        trackMouseEvent.dwFlags = TME_LEAVE;
        trackMouseEvent.hwndTrack = gWindow->hWnd;
        TrackMouseEvent(&trackMouseEvent);

        // get mouse move delta
        if (gMousePosX != MCORE_INVALIDINDEX32 && gMousePosY != MCORE_INVALIDINDEX32 &&
            gLastMousePosX != MCORE_INVALIDINDEX32 && gLastMousePosY != MCORE_INVALIDINDEX32)
        {
            gMouseDeltaX = gMousePosX - gLastMousePosX;
            gMouseDeltaY = gMousePosY - gLastMousePosY;

            if (gMouseDeltaX != 0 || gMouseDeltaY != 0)
            {
                // update the GUI
                if (gGUIManager != NULL)
                {
                    gGUIManager->OnMouseMove(gMousePosX, gMousePosY, gMouseDeltaX, gMouseDeltaY);
                }

                // update the camera
                if (window->keys->keyDown[VK_CONTROL] == true || gAltPressed == true && gFollowCharacter == false)
                {
                    gCamera->ProcessMouseInput(gMouseDeltaX, gMouseDeltaY, gLeftMouseButtonPressed, gMiddleMouseButtonPressed, gRightMouseButtonPressed);
                    gCamera->Update();
                }
            }
        }

        break;
    }

    // left mouse button
    case WM_LBUTTONDOWN:
    {
        gLeftMouseButtonPressed = true;
        if (gGUIManager != NULL)
        {
            gGUIManager->OnMouseButtonDown(true, false, false, gMousePosX, gMousePosY);
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        gLeftMouseButtonPressed = false;
        if (gGUIManager != NULL)
        {
            gGUIManager->OnMouseButtonUp(true, false, false, gMousePosX, gMousePosY);
        }
        break;
    }

    // right mouse button
    case WM_RBUTTONDOWN:
    {
        gRightMouseButtonPressed = true;
        if (gGUIManager != NULL)
        {
            gGUIManager->OnMouseButtonDown(false, false, true, gMousePosX, gMousePosY);
        }
        break;
    }
    case WM_RBUTTONUP:
    {
        gRightMouseButtonPressed = false;
        if (gGUIManager != NULL)
        {
            gGUIManager->OnMouseButtonUp(false, false, true, gMousePosX, gMousePosY);
        }
        break;
    }

    // middle mouse button
    case WM_MBUTTONDOWN:
    {
        gMiddleMouseButtonPressed = true;
        gGUIManager->OnMouseButtonDown(false, true, false, gMousePosX, gMousePosY);
        break;
    }
    case WM_MBUTTONUP:
    {
        gMiddleMouseButtonPressed = false;
        if (gGUIManager != NULL)
        {
            gGUIManager->OnMouseButtonUp(false, true, false, gMousePosX, gMousePosY);
        }
        break;
    }

    // key got pressed
    case WM_KEYDOWN:
        if ((wParam >= 0) && (wParam <= 255))
        {
            window->keys->keyDown[wParam] = true;
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_MENU)
        {
            gAltPressed = true;
        }
        break;

    case WM_SYSKEYUP:
        if (wParam == VK_MENU)
        {
            gAltPressed = false;
        }
        break;

    // key got released
    case WM_KEYUP:
    {
        const char  key = (char)wParam;
        //if (key == 'G')   gRenderGrid             ^= true;
        //if (key == 'W')   gRenderWireframe        ^= true;
        //if (key == 'A')   gRenderAABBs            ^= true;
        //if (key == 'S')   gRenderSkeleton         ^= true;
        //if (key == 'O')   gRenderOBBs             ^= true;
        //if (key == 'T')   gRenderTangents         ^= true;
        //if (key == 'V')   gRenderVertexNormals    ^= true;
        //if (key == 'F')   gRenderFaceNormals      ^= true;
        //if (key == 'C')   gRenderCollisionMeshes  ^= true;
        //if (key == 'H')   gRenderHelp             ^= true;

        if (key == VK_ESCAPE)
        {
            TerminateApplication(window);
        }

        if ((wParam >= 0) && (wParam <= 255))
        {
            window->keys->keyDown[wParam] = false;
            return 0;
        }

        break;
    }

    // toggle fullscreen mode
    case WM_TOGGLEFULLSCREEN:
        gCreateFullScreen = (gCreateFullScreen == true) ? false : true;
        PostMessage (hWnd, WM_QUIT, 0, 0);
        break;
    }

    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}


// register a window class for the application
bool RegisterWindowClass(Application* application)
{
    WNDCLASSEX windowClass;                                             // window class
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));                       // make sure memory is cleared
    windowClass.cbSize          = sizeof(WNDCLASSEX);                   // size of the windowClass structure
    windowClass.style           = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;   // redraws the window for any movement/resizing
    windowClass.lpfnWndProc     = (WNDPROC)(WindowProc);                // windowProc handles messages
    windowClass.hInstance       = application->hInstance;               // set the instance
    windowClass.hbrBackground   = (HBRUSH)(COLOR_APPWORKSPACE);         // class background brush color
    windowClass.hCursor         = LoadCursor(NULL, IDC_ARROW);          // load the arrow pointer
    windowClass.lpszClassName   = application->className;               // sets the applications classname
    if (RegisterClassEx(&windowClass) == 0)
    {
        // NOTE: Failure, Should Never Happen
        MessageBoxA(HWND_DESKTOP, "RegisterClassEx Failed!", "Error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    return true;
}


// render the scene
void Render(float timeDelta)
{
    if (gEngine->GetAdvancedRendering())
    {
        gEngine->SetClearColor(MCore::RGBAColor(0.15f, 0.15f, 0.15f, 1.0f));
    }
    else
    {
        gEngine->SetClearColor(MCore::RGBAColor(0.27f, 0.27f, 0.27f, 1.0f));
    }

    // render the scene
    if (gEngine->BeginRender() == true)
    {
        RenderGL::GLRenderUtil* renderUtil = gEngine->GetRenderUtil();
        glEnable(GL_DEPTH_TEST);

        // character follow mode
        if (gFollowCharacter == true)
        {
            if (gOrbitCamera != NULL && gFollowActorInstance != NULL)
            {
                // make the camera follow the character
                gOrbitCamera->SetTarget(gFollowActorInstance->GetLocalPosition() + MCore::Vector3(0.0f, 1.0f, 0.0f) * gFollowCharacterHeight * 0.5f);
                gOrbitCamera->Update();
            }
        }

        // update the camera
        gCamera->SetNearClipDistance(0.1f);
        gCamera->SetFarClipDistance(100.0f);
        gCamera->SetAspectRatio(gScreenWidth / (float)gScreenHeight);
        gCamera->SetScreenDimensions(gScreenWidth, gScreenHeight);
        gCamera->Update(timeDelta);

        // pass the camera information to OpenGL
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(gCamera->GetViewMatrix().m16);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(gCamera->GetProjectionMatrix().m16);
        glMatrixMode(GL_MODELVIEW);

        // render the grid
        RenderGrid();

        // update the example
        Update(timeDelta);
        ((MCommon::RenderUtil*)renderUtil)->RenderLines();

        // end rendering
        gEngine->EndRender();   // post processing has been done from this point on

        // render the camera orientation axis
        RenderCameraOrientationAxis();

        // render the GUI
        gGUIManager->Render();

        // render lines, text and textures
        renderUtil->RenderTextPeriods();
        renderUtil->RenderTextures();
        ((MCommon::RenderUtil*)renderUtil)->Render2DLines();

        // render the frames per second to the HUD
        //gString = AZStd::string::format("FPS %0.0f", gFPS);
        //gEngine->GetRenderUtil()->RenderText( gScreenWidth-55, 5, gString.AsChar(), MCore::RGBAColor(1,1,1,1), 11.0f );

        /*#ifdef MCORE_DEBUG
        // render camera info
        MCore::String camText;
        MCommon::OrbitCamera* orbitCamera = static_cast<MCommon::OrbitCamera*>(gCamera);
        camText.Format( "Camera: Alpha=%.1f, Beta=%.1f, CurrentDistance=%.1f, Target=(%.1f, %.1f, %.1f)", orbitCamera->GetAlpha(), orbitCamera->GetBeta(), orbitCamera->GetCurrentDistance(), orbitCamera->GetTarget().x, orbitCamera->GetTarget().y, orbitCamera->GetTarget().z );
        MCore::LogInfo( "gOrbitCamera->Set( %.2ff, %.2ff, %.2ff, Vector3(%.2ff, %.2ff, %.2ff) );", orbitCamera->GetAlpha(), orbitCamera->GetBeta(), orbitCamera->GetCurrentDistance(), orbitCamera->GetTarget().x, orbitCamera->GetTarget().y, orbitCamera->GetTarget().z );
        //gEngine->GetRenderUtil()->DrawFont( 5, 500, 0.1f, MCore::RGBAColor(1,1,1,1), 11.0f, camText.AsChar() );
        #endif*/

        // render HUD
        //RenderHelpScreen(renderUtil);
    }
}


// main
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // set the current working directory to the exe dir
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    int len = wcslen(exePath);
    int i = len - 1;
    while (i > 0)
    {
        if (exePath[i] == L'/' || exePath[i] == L'\\')
        {
            exePath[i] = L'\0';
            break;
        }
        --i;
    }
    SetCurrentDirectory(exePath);

    // init EMotion FX
    InitEMotionFX();

    Application         application;                                    // application structure
    GL_Window           window;                                         // window structure
    Keys                keys;                                           // key structure
    bool                isMessagePumpActive;                            // message pump active?
    MSG                 msg;                                            // window message structure

    #ifdef UNICODE
    application.className = L"OpenGL";                                  // application Class Name
    #else
    application.className = "OpenGL";                                   // application Class Name
    #endif

    application.hInstance = hInstance;                                  // application Instance

    AZStd::string caption;
    caption = AZStd::string::format("%s - API - http://www.emotionfx.com", EMotionFX::GetEMotionFX().GetVersionString());

    // fill out window
    ZeroMemory(&window, sizeof(GL_Window));                             // make sure memory is zeroed
    ZeroMemory(&keys, sizeof(Keys));                                    // zero keys structure
    window.keys                 = &keys;                                // window key structure
    window.init.application     = &application;                         // window application
    window.init.width           = gScreenWidth;                         // window width
    window.init.height          = gScreenHeight;                        // window height
    window.init.bitsPerPixel    = 32;                                   // bits per pixel
    window.init.isFullScreen    = false;                                // fullscreen?

    MCore::AbstractData data;
    caption.AsWChar(&data);
    window.init.title = (wchar_t*)data.GetPointer();

    gWindow = &window;

    // register a class for our window to use
    if (RegisterWindowClass(&application) == false)
    {
        data.Release();
        MessageBoxA(HWND_DESKTOP, "Error registering window class.", "Error", MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

    gIsProgramLooping = true;
    gCreateFullScreen = window.init.isFullScreen;


    // loop Until WM_QUIT Is Received
    while (gIsProgramLooping)
    {
        // create a OpenGL window
        window.init.isFullScreen = gCreateFullScreen;
        if (CreateWindowGL(&window) == true)
        {
            data.Release();

            // initialize OpenGL rendering engine
            gEngine = new RenderGL::GraphicsManager();
            if (gEngine->Init("../../Shared/Shaders/GLSL/") == false)
            {
                MessageBoxA(NULL, "OpenGL rendering engine failed to initialize.", "Failed to initialize", MB_OK | MB_ICONINFORMATION);
                ShutdownEMotionFX();
                TerminateApplication (&window);
                return -1;
            }

            RenderGL::GBuffer geomBuffer;
            gEngine->SetGBuffer(&geomBuffer);
            Resize(gScreenWidth, gScreenHeight);
            //gEngine->GetGBuffer()->Resize( gScreenWidth, gScreenHeight );
            gEngine->SetAdvancedRendering(false);
            gEngine->SetDOFEnabled(true);
            gEngine->SetBloomEnabled(true);

            gEngine->SetupSunsetRim();
            //gEngine->SetupBlueRim();

            //gEngine->SetRimColor( MCore::RGBAColor(0.15f, 0.15f, 0.15f) );

            // show the loading screen texture and display it
            RenderGL::Texture* loadingTexture = gEngine->LoadTexture("../../../Shared/Textures/Loading.png");
            gEngine->GetRenderUtil()->RenderTexture(loadingTexture, AZ::Vector2(0.0f, 0.0f));
            gEngine->GetRenderUtil()->RenderTextures();
            SwapBuffers(window.hDC);

            // initialize the GUI manager
            gGUIManager = new GUIManager();
            gGUIManager->Init();

            // create an orbit camera and set it to the rendering engine
            SetCamera(new MCommon::OrbitCamera());

            gRenderUtil = gEngine->GetRenderUtil();

            // call the init function of the example
            if (Init() == false)
            {
                MessageBox(NULL, L"Example initialization failed.", L"ERROR", MB_OK | MB_ICONINFORMATION);

                // shutdown example data
                Cleanup();

                delete gCamera;

                // release the rendering engine
                geomBuffer.Release();
                delete gEngine;

                // release the GUI manager
                delete gGUIManager;

                // clear the global string memory
                gString.Clear(false);
                caption.Clear(false);

                // release emfx and core
                ShutdownEMotionFX();

                TerminateApplication(&window);
                return -1;
            }
            // otherwise start the message pump
            else
            {
                // initialize the GUI
                InitGUI();

                // automatically zoom to the characters
                if (gAutomaticCameraZoom == true && gCamera != NULL)
                {
                    const MCore::AABB sceneAABB = gEngine->GetRenderUtil()->CalcSceneAABB();

                    if (sceneAABB.CheckIfIsValid() == true)
                    {
                        gCamera->ViewCloseup(sceneAABB, 0.5f);
                    }
                }

                // initialize was a success
                isMessagePumpActive = true;
                while (isMessagePumpActive == true)
                {
                    // success creating window, check for window messages
                    if (PeekMessage (&msg, window.hWnd, 0, 0, PM_REMOVE) != 0)
                    {
                        // check For WM_QUIT Message
                        if (msg.message != WM_QUIT)
                        {
                            DispatchMessage(&msg);
                        }
                        else
                        {
                            isMessagePumpActive = false;
                        }
                    }
                    else
                    {
                        if (window.isVisible == false)
                        {
                            // application Is Minimized Wait For A Message
                            WaitMessage();
                        }
                        else
                        {
                            // update the timer and calculate the frames per second
                            const double    timeDelta   = gTimer.GetTimeDelta();
                            static uint32   frameCount  = 0;
                            static double   fpsTime     = 0.0f;
                            static double   totalSlowTime = 0.0f;
                            static bool     firstFrame  = true;
                            static bool     showedDisableMessage = false;
                            fpsTime += timeDelta;
                            if (fpsTime >= 1.0)
                            {
                                gFPS        = frameCount / fpsTime;
                                frameCount  = 0;
                                fpsTime     = 0.0f;
                            }
                            frameCount++;

                            // check if we are going too slow or not
                            if (gFPS < 25.0f && firstFrame == false)
                            {
                                totalSlowTime += timeDelta;
                            }
                            else
                            {
                                totalSlowTime = 0.0f;
                            }

                            // disable advanced rendering if the fps was constantly too low
                            if (totalSlowTime > 3.0f && showedDisableMessage == false && gEngine->GetAdvancedRendering() == true)
                            {
                                gEngine->SetAdvancedRendering(false);
                                gRenderUtil->RenderTextPeriod(gScreenWidth / 2, gScreenHeight - 20, "Advanced rendering disabled due to low framerate", 7.0f, MCore::RGBAColor(1.0f, 1.0f, 0.0f), 12.0f, true);
                                showedDisableMessage = true;
                            }

                            firstFrame = false;

                            // handle frame based mouse input
                            gFrameBasedMouseDeltaX      = gMousePosX - gLastMousePosX;
                            gFrameBasedMouseDeltaY      = gMousePosY - gLastMousePosY;
                            gFrameBasedLastMousePosX    = gMousePosX;
                            gFrameBasedLastMousePosY    = gMousePosY;

                            // update and render the scene and switch buffers (double buffering)
                            Render(timeDelta);
                            SwapBuffers(window.hDC);
                        }
                    }
                }
            }

            DestroyWindowGL (&window);
        }
        else
        {
            data.Release();
            // error Creating Window
            MessageBox(HWND_DESKTOP, L"Error creating OpenGL window.", L"Error", MB_OK | MB_ICONEXCLAMATION);
            gIsProgramLooping = false;
        }
    }

    // shutdown example
    Cleanup();

    // destroy the GUI manager
    delete gGUIManager;

    // shutdown rendering
    delete gCamera;
    delete gEngine;

    // clear the global string memory
    gString.Clear(false);
    caption.Clear(false);

    // shutdown EMotion FX
    ShutdownEMotionFX();

    UnregisterClass(application.className, application.hInstance);
    return 0;
}
