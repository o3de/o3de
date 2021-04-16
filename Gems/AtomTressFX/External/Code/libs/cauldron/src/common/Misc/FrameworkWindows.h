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

#pragma once

class FrameworkWindows
{
public:

    FrameworkWindows(LPCSTR name) : m_Name(name) {};
    virtual ~FrameworkWindows() {}

    LPCSTR GetName() { return m_Name; }
    uint32_t GetWidth() { return m_Width; }
    uint32_t GetHeight() { return m_Height; }

    virtual void OnActivate(bool windowActive) {};

    // Pure virtual functions

    virtual void OnCreate(HWND hWnd) = 0;
    virtual void OnDestroy() = 0;
    virtual void OnRender() = 0;
    virtual bool OnEvent(MSG msg) = 0;
    virtual void OnResize(uint32_t Width, uint32_t Height) = 0;
    virtual void SetFullScreen(bool fullscreen) = 0;

protected:
    // sample name
    LPCSTR m_Name;

    // viewport metrics
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};


int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, uint32_t Width, uint32_t Height, FrameworkWindows *pFramework);
