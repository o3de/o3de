/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/PipelineLibrary.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<PipelineLibrary> PipelineLibrary::Create()
        {
            return aznew PipelineLibrary();
        }

        VkPipelineCache PipelineLibrary::GetNativePipelineCache() const
        {
            return m_nativePipelineCache;
        }

        RHI::ResultCode PipelineLibrary::InitInternal(RHI::Device& deviceBase, const RHI::PipelineLibraryData* serializedData)
        {
            DeviceObject::Init(deviceBase);
            auto& device = static_cast<Device&>(deviceBase);

            VkPipelineCacheCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0; 
            createInfo.initialDataSize = 0;
            createInfo.pInitialData = nullptr;

            if (serializedData)
            {
                createInfo.initialDataSize = static_cast<size_t>(serializedData->GetData().size());
                createInfo.pInitialData = serializedData->GetData().data();
            }

            const VkResult result = vkCreatePipelineCache(device.GetNativeDevice(), &createInfo, nullptr, &m_nativePipelineCache);
            AssertSuccess(result);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        void PipelineLibrary::ShutdownInternal()
        {
            if (m_nativePipelineCache != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkDestroyPipelineCache(device.GetNativeDevice(), m_nativePipelineCache, nullptr);
                m_nativePipelineCache = VK_NULL_HANDLE;
            }
        }

        RHI::ResultCode PipelineLibrary::MergeIntoInternal(AZStd::array_view<const RHI::PipelineLibrary *> libraries)
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (libraries.empty())
            {
                return RHI::ResultCode::Success;
            }

            AZStd::vector<VkPipelineCache> pipelineCaches;
            pipelineCaches.reserve(libraries.size());
            for (const RHI::PipelineLibrary* libraryBase : libraries)
            {
                const auto* library = static_cast<const PipelineLibrary*>(libraryBase);
                pipelineCaches.emplace_back(library->GetNativePipelineCache());
            }

            const VkResult result = vkMergePipelineCaches(device.GetNativeDevice(), m_nativePipelineCache,
                static_cast<uint32_t>(pipelineCaches.size()), pipelineCaches.data());
            AssertSuccess(result);

            return ConvertResult(result);
        }

        RHI::ConstPtr<RHI::PipelineLibraryData> PipelineLibrary::GetSerializedDataInternal() const
        {
            auto& device = static_cast<Device&>(GetDevice());

            size_t dataSize = 0;
            VkResult result = vkGetPipelineCacheData(device.GetNativeDevice(), m_nativePipelineCache, &dataSize, nullptr);
            AssertSuccess(result);
            if (result != VK_SUCCESS)
            {
                return RHI::ConstPtr<RHI::PipelineLibraryData>();
            }

            AZStd::vector<uint8_t> data(dataSize);
            result = vkGetPipelineCacheData(device.GetNativeDevice(), m_nativePipelineCache, &dataSize, data.data());
            AssertSuccess(result);

            return RHI::PipelineLibraryData::Create(AZStd::move(data));
        }

        void PipelineLibrary::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativePipelineCache), name.data(), VK_OBJECT_TYPE_PIPELINE_CACHE, static_cast<Device&>(GetDevice()));
            }
        }
    }
}
