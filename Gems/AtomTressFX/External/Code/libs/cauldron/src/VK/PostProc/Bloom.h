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
#include "blurPS.h"

#define BLOOM_MAX_MIP_LEVELS 12

namespace CAULDRON_VK
{
    class Bloom
    {
    public:
        void OnCreate(
            Device* pDevice,
            ResourceViewHeaps       *pHeaps,
            DynamicBufferRing       *pConstantBufferRing,
            StaticBufferPool        *pResourceViewHeaps,
            VkFormat format
        );
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, Texture *pInput, int mipCount, Texture *pOutput);
        void OnDestroyWindowSizeDependentResources();

        void Draw(VkCommandBuffer cmd_buf);

        void Gui();

    private:
        Device                    *m_pDevice = nullptr;

        ResourceViewHeaps         *m_pResourceViewHeaps;
        DynamicBufferRing         *m_pConstantBufferRing;

        VkFormat                   m_outFormat;

        uint32_t                   m_Width;
        uint32_t                   m_Height;
        int                        m_mipCount;

        bool                       m_doBlur;
        bool                       m_doUpscale;

        struct cbBlend
        {
            float weight;
        };

        struct Pass
        {
            VkImageView     m_RTV;
            VkImageView     m_SRV;
            VkFramebuffer   m_frameBuffer;
            VkDescriptorSet m_descriptorSet;
            float m_weight;
        };

        Pass                       m_mip[BLOOM_MAX_MIP_LEVELS];
        Pass                       m_output;

        BlurPS                     m_blur;
        PostProcPS                 m_blendAdd;

        VkDescriptorSetLayout      m_descriptorSetLayout;

        VkSampler                  m_sampler;

        VkRenderPass               m_blendPass;
    };
}


