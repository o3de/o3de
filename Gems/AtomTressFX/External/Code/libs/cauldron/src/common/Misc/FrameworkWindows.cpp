// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "FrameworkWindows.h"

LRESULT CALLBACK WindowProc(HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

static FrameworkWindows* dxSample = NULL;

static bool bIsFullScreen = false;
static bool bIsMinimized = false;
LONG lBorderedStyle = 0;
LONG lBorderlessStyle = 0;

int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, uint32_t Width, uint32_t Height, FrameworkWindows *pFramework)
{
    // create new DX sample
    dxSample = pFramework;

    HWND hWnd;
    WNDCLASSEX windowClass;

    // init window class
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = "WindowClass1";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, (LONG)Width, (LONG)Height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

    // This makes sure that in a multimonitor setup with different resolutions, get monitor info returns correct dimensions
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

                                                                  // create the window and store a handle to it
    hWnd = CreateWindowEx(NULL,
        "WindowClass1",    // name of the window class
        dxSample ? dxSample->GetName() : "None",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,    // we have no parent window, NULL
        NULL,    // we aren't using menus, NULL
        hInstance,    // application handle
        NULL);    // used with multiple windows, NULL

    if (dxSample)
        dxSample->OnCreate(hWnd);

    // show the window
    ShowWindow(hWnd, nCmdShow);
    lBorderedStyle = GetWindowLong(hWnd, GWL_STYLE);
    lBorderlessStyle = lBorderedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    // main loop
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        // check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // translate keystroke messages into the right format            
            DispatchMessage(&msg); // send the message to the WindowProc function
        }
        else
        {
            if (dxSample && bIsMinimized == false)
            {
                dxSample->OnRender();
            }
        }
    }

    if (dxSample)
    {
        dxSample->OnDestroy();
        delete dxSample;
    }

    // return this part of the WM_QUIT message to Windows
    return static_cast<char>(msg.wParam);
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    // When close button is clicked on window
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        break;
    }

    case WM_SIZE:
    {
        if (dxSample)
        {
            RECT clientRect = {};
            GetClientRect(hWnd, &clientRect);
            dxSample->OnResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

            bIsMinimized = (IsIconic(hWnd) == TRUE);

            return 0;
        }
        break;
    }

    case  WM_SYSKEYDOWN:
    {
        if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
        {
            bIsFullScreen = !bIsFullScreen;
            if (dxSample)
            {
                dxSample->SetFullScreen(bIsFullScreen);
            }
        }
        break;
    }

    // When window goes outof focus, use this event to fall back on SDR.
    // If we don't gracefully fallback to SDR, the renderer will output HDR colours which will look extremely bright and washed out.
    // However if you want to use breakpoints in HDR mode to inspect/debug values, you will have to comment this function call.
    case WM_ACTIVATE:
    {
        if (dxSample)
        {
            dxSample->OnActivate(wParam != WA_INACTIVE);
        }

        break;
    }

    }

    if (dxSample)
    {
        MSG msg;
        msg.hwnd = hWnd;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
        dxSample->OnEvent(msg);
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}
