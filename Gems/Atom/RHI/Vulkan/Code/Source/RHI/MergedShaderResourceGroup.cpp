/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/BufferView.h>
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>
#include <AzCore/std/parallel/lock.h>

namespace AZ
{
    namespace Vulkan
    {
        const char* MergedShaderResourceGroup::ConstantDataBufferName = "ConstantsDataBuffer";

        AZ::Name MergedShaderResourceGroup::GenerateMergedShaderInputName(const AZ::Name& shaderInputName, uint32_t bindingSlot)
        {
            return AZ::Name(AZStd::string::format("MergedSRG%d_%s", bindingSlot, shaderInputName.GetCStr()));
        }

        RHI::DeviceShaderResourceGroupData MergedShaderResourceGroup::MergeShaderResourceData(const ShaderResourceGroupArray& srgList) const
        {
            // Merge the SRG data of each of the SRG instances.
            const RHI::ShaderResourceGroupLayout* srgLayout = GetPool()->GetLayout();
            RHI::DeviceShaderResourceGroupData data(srgLayout);
            for (const auto* srg : srgList)
            {
                if (!srg)
                {
                    continue;
                }

                const auto& srgData = srg->GetData();
                // Add the buffer views
                const auto& bufferShaderInputList = srgData.GetLayout()->GetShaderInputListForBuffers();
                for (uint32_t i = 0; i < bufferShaderInputList.size(); ++i)
                {
                    const auto& shaderInputDesc = bufferShaderInputList[i];
                    RHI::ShaderInputBufferIndex shaderInputIndex = srgLayout->FindShaderInputBufferIndex(GenerateMergedShaderInputName(shaderInputDesc.m_name, srgData.GetLayout()->GetBindingSlot()));
                    if (!shaderInputIndex.IsValid())
                    {
                        AZ_Assert(false, "Failed to find merged shader input %s for SRG on slot %d", shaderInputDesc.m_name.GetCStr(), srgData.GetLayout()->GetBindingSlot());
                        continue;
                    }

                    const auto& sourceViews = srgData.GetBufferViewArray(RHI::ShaderInputBufferIndex(i));
                    AZStd::vector<const RHI::DeviceBufferView*> destViews;
                    AZStd::transform(sourceViews.begin(), sourceViews.end(), AZStd::back_inserter(destViews), [](const auto& view) { return view.get(); });
                    data.SetBufferViewArray(shaderInputIndex, destViews);
                }

                // Add the image views
                const auto& imageShaderInputList = srgData.GetLayout()->GetShaderInputListForImages();
                for (uint32_t i = 0; i < imageShaderInputList.size(); ++i)
                {
                    const auto& shaderInputDesc = imageShaderInputList[i];
                    RHI::ShaderInputImageIndex shaderInputIndex = srgLayout->FindShaderInputImageIndex(GenerateMergedShaderInputName(shaderInputDesc.m_name, srgData.GetLayout()->GetBindingSlot()));
                    if (!shaderInputIndex.IsValid())
                    {
                        AZ_Assert(false, "Failed to find merged shader input %s for SRG on slot %d", shaderInputDesc.m_name.GetCStr(), srgData.GetLayout()->GetBindingSlot());
                        continue;
                    }

                    const auto& sourceViews = srgData.GetImageViewArray(RHI::ShaderInputImageIndex(i));
                    AZStd::vector<const RHI::DeviceImageView*> destViews;
                    AZStd::transform(sourceViews.begin(), sourceViews.end(), AZStd::back_inserter(destViews), [](const auto& view) { return view.get(); });
                    data.SetImageViewArray(shaderInputIndex, destViews);
                }

                // Add the samplers
                const auto& samplerShaderInputList = srgData.GetLayout()->GetShaderInputListForSamplers();
                for (uint32_t i = 0; i < samplerShaderInputList.size(); ++i)
                {
                    const auto& shaderInputDesc = samplerShaderInputList[i];
                    RHI::ShaderInputSamplerIndex shaderInputIndex = srgLayout->FindShaderInputSamplerIndex(GenerateMergedShaderInputName(shaderInputDesc.m_name, srgData.GetLayout()->GetBindingSlot()));
                    if (!shaderInputIndex.IsValid())
                    {
                        AZ_Assert(false, "Failed to find merged shader input %s for SRG on slot %d", shaderInputDesc.m_name.GetCStr(), srgData.GetLayout()->GetBindingSlot());
                        continue;
                    }

                    data.SetSamplerArray(shaderInputIndex, srgData.GetSamplerArray(RHI::ShaderInputSamplerIndex(i)));
                }

                // Add the constant data
                if (srgData.GetLayout()->GetConstantDataSize())
                {
                    RHI::ShaderInputBufferIndex constantsDataBufferIndex = srgLayout->FindShaderInputBufferIndex(GenerateMergedShaderInputName(AZ::Name(ConstantDataBufferName), srgData.GetLayout()->GetBindingSlot()));
                    if (!constantsDataBufferIndex.IsValid())
                    {
                        AZ_Assert(false, "Failed to find constant data buffer for SRG on slot %d", srgData.GetLayout()->GetBindingSlot());
                        continue;
                    }

                    // Merged ShaderResourceGroups do not contain constant data.
                    // We just use the constant buffer that contains the constant data that was already built and populated
                    // by the original SRG.
                    data.SetBufferView(constantsDataBufferIndex, srg->GetCompiledData().GetConstantDataBufferView().get());
                }
            }

            return data;
        }

        bool MergedShaderResourceGroup::NeedsCompile() const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_compileMutex);
            for (uint32_t i = 0; i < m_mergedShaderResourceGroupList.size(); ++i)
            {
                if (m_mergedShaderResourceGroupList[i] && m_lastCompileFrameIteration[i].m_frameIteration != m_mergedShaderResourceGroupList[i]->GetLastCompileFrameIteration())
                {
                    return true;
                }
            }

            return false;
        }

        void MergedShaderResourceGroup::Compile()
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_compileMutex);
            RHI::DeviceShaderResourceGroupData srgData = MergeShaderResourceData(m_mergedShaderResourceGroupList);
            RHI::DeviceShaderResourceGroup::Compile(srgData, RHI::DeviceShaderResourceGroup::CompileMode::Sync);
            // Update the last frame iteration value.
            for (uint32_t i = 0; i < m_mergedShaderResourceGroupList.size(); ++i)
            {
                if (m_mergedShaderResourceGroupList[i])
                {
                    m_lastCompileFrameIteration[i].m_frameIteration = m_mergedShaderResourceGroupList[i]->GetLastCompileFrameIteration();
                }
            }
        }
    }
}
