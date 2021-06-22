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
#include "Atom_RHI_Metal_precompiled.h"

#include <RHI/ArgumentBuffer.h>
#include <RHI/ImageView.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ShaderResourceGroup> ShaderResourceGroup::Create()
        {
            return aznew ShaderResourceGroup();
        }
        
        void ShaderResourceGroup::UpdateCompiledDataIndex()
        {
            m_compiledDataIndex = (m_compiledDataIndex + 1) % RHI::Limits::Device::FrameCountMax;
        }
 
        const ArgumentBuffer& ShaderResourceGroup::GetCompiledArgumentBuffer() const
        {
            return *m_compiledArgBuffers[m_compiledDataIndex];
        }
    
        void ShaderResourceGroup::CollectUntrackedResources(id<MTLCommandEncoder> commandEncoder,
                                                            const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                                            ArgumentBuffer::ComputeResourcesToMakeResidentMap& resourcesToMakeResidentCompute,
                                                            ArgumentBuffer::GraphicsResourcesToMakeResidentMap& resourcesToMakeResidentGraphics) const
        {
            GetCompiledArgumentBuffer().CollectUntrackedResources(commandEncoder, srgResourcesVisInfo, resourcesToMakeResidentCompute, resourcesToMakeResidentGraphics);
        }
    }
}
