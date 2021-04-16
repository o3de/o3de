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

#include "PostProcPS.h"

namespace CAULDRON_VK
{
#define DOWNSAMPLEPS_MAX_MIP_LEVELS 12

    class DownSamplePS
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *m_pConstantBufferRing, StaticBufferPool *pStaticBufferPool, VkFormat outFormat);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mips);
        void OnDestroyWindowSizeDependentResources();

        void Draw(VkCommandBuffer cmd_buf);
        Texture *GetTexture() { return &m_result; }
        VkImageView GetTextureView(int i) { return m_mip[i].m_SRV; }
        void Gui();

        struct cbDownscale
        {
            float invWidth, invHeight;
            int mipLevel;
        };

    private:
        Device                     *m_pDevice;
        VkFormat                    m_outFormat;

        Texture                     m_result;

        struct Pass
        {
            VkImageView     RTV; //dest
            VkImageView     m_SRV; //src
            VkFramebuffer   frameBuffer;
            VkDescriptorSet descriptorSet;
        };

        Pass                         m_mip[DOWNSAMPLEPS_MAX_MIP_LEVELS];

        StaticBufferPool            *m_pStaticBufferPool;
        ResourceViewHeaps           *m_pResourceViewHeaps;
        DynamicBufferRing           *m_pConstantBufferRing;

        uint32_t                     m_Width;
        uint32_t                     m_Height;
        int                          m_mipCount;

        VkDescriptorSetLayout        m_descriptorSetLayout;

        PostProcPS                   m_downscale;

        VkRenderPass                 m_in;

        VkSampler                    m_sampler;
    };
}


