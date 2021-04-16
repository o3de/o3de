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

#include "Device.h"
#include "FreeSync2.h"

namespace CAULDRON_VK
{
    class SwapChain
    {
    public:
        void OnCreate(Device *pDevice, uint32_t numberBackBuffers, HWND hWnd, DisplayModes displayMode);
        void OnDestroy();

        void SetFullScreen(bool fullscreen);
        void OnCreateWindowSizeDependentResources(uint32_t dwWidth, uint32_t dwHeight, bool bVSyncOn, DisplayModes displayMode);
        void OnDestroyWindowSizeDependentResources();

        uint32_t WaitForSwapChain();
        void GetSemaphores(VkSemaphore *pImageAvailableSemaphore, VkSemaphore *pRenderFinishedSemaphores, VkFence *pCmdBufExecutedFences);
        void Present();

        // getters
        VkImage GetCurrentBackBuffer();
        VkImageView GetCurrentBackBufferRTV();
        bool IsFullScreen() const { return m_isFullScreen; }
        VkSwapchainKHR GetSwapChain() const { return m_swapChain; }
        VkFormat GetFormat() const { return m_swapChainFormat.format; };
        VkRenderPass GetRenderPass() { return m_render_pass_swap_chain; };
        VkFramebuffer GetFramebuffer(int i) const { return m_framebuffers[i]; }

    private:
        void CreateRTV();
        void DestroyRTV();
        void CreateRenderPass();
        void DestroyRenderPass();
        void CreateFramebuffers(uint32_t dwWidth, uint32_t dwHeight);
        void DestroyFramebuffers();

        HWND m_hWnd;
        Device *m_pDevice;

        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_swapChainFormat;

        VkQueue m_presentQueue;

        VkRenderPass m_render_pass_swap_chain = VK_NULL_HANDLE;

        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
        std::vector<VkFramebuffer> m_framebuffers;

        std::vector<VkFence>       m_cmdBufExecutedFences;
        std::vector<VkSemaphore>   m_ImageAvailableSemaphores;
        std::vector<VkSemaphore>   m_RenderFinishedSemaphores;

        uint32_t m_imageIndex = 0;
        uint32_t m_prevImageIndex = 0;
        uint32_t m_backBufferCount;

        // fullscreen/windowed vars
        bool m_isFullScreen = false;
        struct SavedWindowInfo
        {
            LONG_PTR Style;
            LONG_PTR ExStyle;
            bool IsMaximized;
            RECT WindowRect;
        };
        SavedWindowInfo m_windowedState = {};

        bool m_bVSyncOn = false;
    };
}
