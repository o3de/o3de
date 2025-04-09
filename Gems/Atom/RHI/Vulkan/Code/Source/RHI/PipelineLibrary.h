/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineLibrary.h>
#include <Atom/RHI.Reflect/PipelineLibraryData.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/Pipeline.h>

namespace AZ
{
    namespace Vulkan
    {
        class PipelineLibrary final
            : public RHI::DevicePipelineLibrary
        {
            using Base = RHI::DevicePipelineLibrary;

        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator);
            AZ_RTTI(PipelineLibrary, "EB865D8F-7753-4E06-8401-310CC1CF2378", Base);

            static RHI::Ptr<PipelineLibrary> Create();

            VkPipelineCache GetNativePipelineCache() const;

        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineLibrary
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DevicePipelineLibraryDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode MergeIntoInternal(AZStd::span<const RHI::DevicePipelineLibrary* const> libraries) override;
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override;
            bool SaveSerializedDataInternal(const AZStd::string& filePath) const override;
            //////////////////////////////////////////////////////////////////////////

            VkPipelineCache m_nativePipelineCache = VK_NULL_HANDLE;
        };
    }
}
