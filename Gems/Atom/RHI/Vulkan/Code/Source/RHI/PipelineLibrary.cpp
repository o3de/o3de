/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/PipelineLibrary.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

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

        RHI::ResultCode PipelineLibrary::InitInternal(RHI::Device& deviceBase, const RHI::DevicePipelineLibraryDescriptor& descriptor)
        {
            DeviceObject::Init(deviceBase);
            auto& device = static_cast<Device&>(deviceBase);

            VkPipelineCacheCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.initialDataSize = 0;
            createInfo.pInitialData = nullptr;

            if (descriptor.m_serializedData)
            {
                createInfo.initialDataSize = static_cast<size_t>(descriptor.m_serializedData->GetData().size());
                createInfo.pInitialData = descriptor.m_serializedData->GetData().data();
            }

            const VkResult result = device.GetContext().CreatePipelineCache(
                device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativePipelineCache);
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
                device.GetContext().DestroyPipelineCache(device.GetNativeDevice(), m_nativePipelineCache, VkSystemAllocator::Get());
                m_nativePipelineCache = VK_NULL_HANDLE;
            }
        }

        RHI::ResultCode PipelineLibrary::MergeIntoInternal(AZStd::span<const RHI::DevicePipelineLibrary * const> libraries)
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (libraries.empty())
            {
                return RHI::ResultCode::Success;
            }

            AZStd::vector<VkPipelineCache> pipelineCaches;
            pipelineCaches.reserve(libraries.size());
            for (const RHI::DevicePipelineLibrary* libraryBase : libraries)
            {
                const auto* library = static_cast<const PipelineLibrary*>(libraryBase);
                pipelineCaches.emplace_back(library->GetNativePipelineCache());
            }

            const VkResult result = device.GetContext().MergePipelineCaches(
                device.GetNativeDevice(), m_nativePipelineCache, static_cast<uint32_t>(pipelineCaches.size()), pipelineCaches.data());
            AssertSuccess(result);

            return ConvertResult(result);
        }

        RHI::ConstPtr<RHI::PipelineLibraryData> PipelineLibrary::GetSerializedDataInternal() const
        {
            auto& device = static_cast<Device&>(GetDevice());

            size_t dataSize = 0;
            VkResult result = device.GetContext().GetPipelineCacheData(device.GetNativeDevice(), m_nativePipelineCache, &dataSize, nullptr);
            AssertSuccess(result);
            if (result != VK_SUCCESS)
            {
                return RHI::ConstPtr<RHI::PipelineLibraryData>();
            }

            AZStd::vector<uint8_t> data(dataSize);
            result = device.GetContext().GetPipelineCacheData(device.GetNativeDevice(), m_nativePipelineCache, &dataSize, data.data());
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

        bool PipelineLibrary::SaveSerializedDataInternal([[maybe_unused]] const AZStd::string& filePath) const
        {
            //Vulkan drivers cannot save serialized data
            [[maybe_unused]] Device& device = static_cast<Device&>(GetDevice());
            AZ_Assert(!device.GetFeatures().m_isPsoCacheFileOperationsNeeded, "Explicit PSO cache operations should not be disabled for Vulkan");
            return false;
        }
    }
}
