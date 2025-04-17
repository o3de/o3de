/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>
#include <RHI/Device.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/ReleaseContainer.h>
#include <RHI/SpecializationConstantData.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<RayTracingPipelineState> RayTracingPipelineState::Create()
        {
            return aznew RayTracingPipelineState;
        }

        RHI::ResultCode RayTracingPipelineState::InitInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingPipelineStateDescriptor* descriptor)
        {
            Device &device = static_cast<Device&>(deviceBase);
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = physicalDevice.GetPhysicalDeviceRayTracingPipelineProperties();

            // map of shader names to hit shader stage indices, needed to get the index of the stage entries for the HitGroups
            typedef AZStd::map<AZStd::string_view, uint32_t> HitStageIndexMap;
            HitStageIndexMap hitStageIndices;

            // list of shader group names, needed to create the lookup from name to shader handle
            AZStd::vector<AZStd::string_view> shaderGroupNames;

            // process shader libraries into shader stages and groups
            AZStd::vector<VkPipelineShaderStageCreateInfo> stages;
            AZStd::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
            AZStd::vector<SpecializationConstantData> specializationDataVector(descriptor->GetShaderLibraries().size());

            m_shaderModules.reserve(descriptor->GetShaderLibraries().size());
            const auto& libraries = descriptor->GetShaderLibraries();
            for (uint32_t i = 0; i < libraries.size(); ++i)
            {
                const RHI::RayTracingShaderLibrary& shaderLibrary = libraries[i];
                const ShaderStageFunction* rayTracingFunction = azrtti_cast<const ShaderStageFunction*>(shaderLibrary.m_descriptor.m_rayTracingFunction.get());
                VkShaderModule& shaderModule = m_shaderModules.emplace_back();
                VkShaderModuleCreateInfo moduleCreateInfo = {};
                moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                moduleCreateInfo.codeSize = rayTracingFunction->GetByteCode(0).size();
                moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(rayTracingFunction->GetByteCode(0).data());
                device.GetContext().CreateShaderModule(
                    device.GetNativeDevice(), &moduleCreateInfo, VkSystemAllocator::Get(), &shaderModule);

                SpecializationConstantData& specializationData = specializationDataVector[i];
                specializationData.Init(shaderLibrary.m_descriptor);

                VkPipelineShaderStageCreateInfo stageCreateInfo = {};
                stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageCreateInfo.module = shaderModule;
                stageCreateInfo.pSpecializationInfo = specializationData.GetVkSpecializationInfo();

                // ray generation
                if (!shaderLibrary.m_rayGenerationShaderName.IsEmpty())
                {
                    VkPipelineShaderStageCreateInfo rayGenerationCreateInfo = stageCreateInfo;
                    rayGenerationCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                    rayGenerationCreateInfo.pName = shaderLibrary.m_rayGenerationShaderName.GetCStr();
                    stages.push_back(rayGenerationCreateInfo);

                    VkRayTracingShaderGroupCreateInfoKHR groupCreateInfo = {};
                    groupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    groupCreateInfo.pNext = nullptr;
                    groupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    groupCreateInfo.generalShader = aznumeric_cast<uint32_t>(stages.size() - 1);
                    groupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
                    groupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
                    groupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
                    groups.push_back(groupCreateInfo);

                    shaderGroupNames.push_back(shaderLibrary.m_rayGenerationShaderName.GetStringView());
                }

                // miss
                if (!shaderLibrary.m_missShaderName.IsEmpty())
                {
                    VkPipelineShaderStageCreateInfo missCreateInfo = stageCreateInfo;
                    missCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                    missCreateInfo.pName = shaderLibrary.m_missShaderName.GetCStr();
                    stages.push_back(missCreateInfo);

                    VkRayTracingShaderGroupCreateInfoKHR groupCreateInfo = {};
                    groupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    groupCreateInfo.pNext = nullptr;
                    groupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    groupCreateInfo.generalShader = aznumeric_cast<uint32_t>(stages.size() - 1);
                    groupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
                    groupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
                    groupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
                    groups.push_back(groupCreateInfo);

                    shaderGroupNames.push_back(shaderLibrary.m_missShaderName.GetStringView());
                }

                // closest hit
                if (!shaderLibrary.m_closestHitShaderName.IsEmpty())
                {
                    VkPipelineShaderStageCreateInfo closestHitCreateInfo = stageCreateInfo;
                    closestHitCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                    closestHitCreateInfo.pName = shaderLibrary.m_closestHitShaderName.GetCStr();
                    stages.push_back(closestHitCreateInfo);

                    hitStageIndices.insert(AZStd::pair<AZStd::string_view, uint32_t>(shaderLibrary.m_closestHitShaderName.GetStringView(), aznumeric_cast<uint32_t>(stages.size() - 1)));
                }

                // any hit
                if (!shaderLibrary.m_anyHitShaderName.IsEmpty())
                {
                    VkPipelineShaderStageCreateInfo anyHitCreateInfo = stageCreateInfo;
                    anyHitCreateInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                    anyHitCreateInfo.pName = shaderLibrary.m_anyHitShaderName.GetCStr();
                    stages.push_back(anyHitCreateInfo);

                    hitStageIndices.insert(AZStd::pair<AZStd::string_view, uint32_t>(shaderLibrary.m_anyHitShaderName.GetStringView(), aznumeric_cast<uint32_t>(stages.size() - 1)));
                }

                // intersection
                if (!shaderLibrary.m_intersectionShaderName.IsEmpty())
                {
                    VkPipelineShaderStageCreateInfo intersectionCreateInfo = stageCreateInfo;
                    intersectionCreateInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                    intersectionCreateInfo.pName = shaderLibrary.m_intersectionShaderName.GetCStr();
                    stages.push_back(intersectionCreateInfo);

                    hitStageIndices.insert(AZStd::pair<AZStd::string_view, uint32_t>(shaderLibrary.m_intersectionShaderName.GetStringView(), aznumeric_cast<uint32_t>(stages.size() - 1)));
                }
            }

            // create group entries for the hit group shaders, using the hitStageIndices map to retrieve the stage index for the shader
            for (const RHI::RayTracingHitGroup& hitGroup : descriptor->GetHitGroups())
            {
                uint32_t closestHitShaderIndex = VK_SHADER_UNUSED_KHR;
                uint32_t anytHitShaderIndex = VK_SHADER_UNUSED_KHR;
                uint32_t intersectionShaderIndex = VK_SHADER_UNUSED_KHR;

                if (!hitGroup.m_closestHitShaderName.IsEmpty())
                {
                    HitStageIndexMap::iterator itStage = hitStageIndices.find(hitGroup.m_closestHitShaderName.GetCStr());
                    AZ_Assert(itStage != hitStageIndices.end(), "HitGroup specified an unknown ClosestHitShader");

                    closestHitShaderIndex = itStage->second;
                }

                if (!hitGroup.m_anyHitShaderName.IsEmpty())
                {
                    HitStageIndexMap::iterator itStage = hitStageIndices.find(hitGroup.m_anyHitShaderName.GetCStr());
                    AZ_Assert(itStage != hitStageIndices.end(), "HitGroup specified an unknown AnyHitShader");

                    anytHitShaderIndex = itStage->second;
                }

                if (!hitGroup.m_intersectionShaderName.IsEmpty())
                {
                    HitStageIndexMap::iterator itStage = hitStageIndices.find(hitGroup.m_intersectionShaderName.GetCStr());
                    AZ_Assert(itStage != hitStageIndices.end(), "HitGroup specified an unknown IntersectionShader");

                    intersectionShaderIndex = itStage->second;
                }

                AZ_Assert(closestHitShaderIndex != VK_SHADER_UNUSED_KHR || anytHitShaderIndex != VK_SHADER_UNUSED_KHR, "HitGroup must specify at least one hit shader");

                VkRayTracingShaderGroupCreateInfoKHR groupCreateInfo = {};
                groupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                groupCreateInfo.pNext = nullptr;
                groupCreateInfo.type = hitGroup.m_intersectionShaderName.IsEmpty()
                    ? VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
                    : VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                groupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
                groupCreateInfo.closestHitShader = closestHitShaderIndex;
                groupCreateInfo.anyHitShader = anytHitShaderIndex;
                groupCreateInfo.intersectionShader = intersectionShaderIndex;
                groups.push_back(groupCreateInfo);

                shaderGroupNames.push_back(hitGroup.m_hitGroupName.GetStringView());
            }

            // store the pipeline layout
            m_pipelineLayout = static_cast<const PipelineState*>(descriptor->GetPipelineState())->GetPipelineLayout()->GetNativePipelineLayout();

            // create the ray tracing pipeline
            VkRayTracingPipelineCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.stageCount = static_cast<uint32_t>(stages.size());
            createInfo.pStages = stages.data();
            createInfo.groupCount = static_cast<uint32_t>(groups.size());
            createInfo.pGroups = groups.data();
            createInfo.maxPipelineRayRecursionDepth = descriptor->GetConfiguration().m_maxRecursionDepth;
            createInfo.layout = m_pipelineLayout;
            createInfo.basePipelineHandle = nullptr;
            createInfo.basePipelineIndex = 0;

            [[maybe_unused]] VkResult result = device.GetContext().CreateRayTracingPipelinesKHR(
                device.GetNativeDevice(), nullptr, nullptr, 1, &createInfo, VkSystemAllocator::Get(), &m_pipeline);
            AZ_Assert(result == VK_SUCCESS, "vkCreateRayTracingPipelinesKHR failed");

            // retrieve the shader handles
            uint32_t shaderHandleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
            m_shaderHandleData.resize(groups.size()* shaderHandleSize);

            result = device.GetContext().GetRayTracingShaderGroupHandlesKHR(
                device.GetNativeDevice(),
                m_pipeline,
                0,
                aznumeric_cast<uint32_t>(groups.size()),
                aznumeric_cast<uint32_t>(m_shaderHandleData.size()),
                m_shaderHandleData.data());
            AZ_Assert(result == VK_SUCCESS, "vkGetRayTracingShaderGroupHandlesKHR failed");

            // build the map of shader names to shader handles
            m_shaderHandles.clear();
            uint8_t* shaderHandle = m_shaderHandleData.data();

            for (auto& groupName : shaderGroupNames)
            {
                m_shaderHandles.insert(AZStd::make_pair(groupName, shaderHandle));
                shaderHandle += shaderHandleSize;
            }

            return RHI::ResultCode::Success;
        }

        uint8_t* RayTracingPipelineState::GetShaderHandle(const AZ::Name& shaderName) const
        {
            ShaderHandleMap::const_iterator itShaderHandle = m_shaderHandles.find(shaderName.GetStringView());
            if (itShaderHandle == m_shaderHandles.end())
            {
                return nullptr;
            }

            return itShaderHandle->second;
        }

        void RayTracingPipelineState::ShutdownInternal()
        {
            Device& device = static_cast<Device&>(GetDevice());

            device.QueueForRelease(
                new ReleaseContainer<VkPipeline>(device.GetNativeDevice(), m_pipeline, device.GetContext().DestroyPipeline));

            for (auto& shaderModule : m_shaderModules)
            {
                device.GetContext().DestroyShaderModule(device.GetNativeDevice(), shaderModule, VkSystemAllocator::Get());
            }
        }
    }
}
