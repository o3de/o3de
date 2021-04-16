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

#include "Base/Device.h"
#include "Base/ShaderCompilerHelper.h"

namespace CAULDRON_VK
{
    class PostProcCS
    {
    public:
        void OnCreate(
            Device* pDevice,
            const std::string &filename,
            const std::string &entryPoint,
            VkDescriptorSetLayout descriptorSetLayout,
            uint32_t dwWidth, uint32_t dwHeight, uint32_t dwDepth,
            DefineList* userDefines = 0
        );
        void OnDestroy();
        void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo constantBuffer, VkDescriptorSet descSet, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ);

    private:
        Device* m_pDevice;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout;
    };
}

