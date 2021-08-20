/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
    
        bool ShaderResourceGroup::IsNullHeapNeededForVertexStage(const ShaderResourceGroupVisibility& srgResourcesVisInfo) const
        {
            return GetCompiledArgumentBuffer().IsNullHeapNeededForVertexStage(srgResourcesVisInfo);
        }
    }
}
