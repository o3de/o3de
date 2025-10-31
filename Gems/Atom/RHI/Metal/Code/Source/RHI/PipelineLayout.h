/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/unordered_map.h>
#include <RHI/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class PipelineLayoutCache;
        class Device;

        class PipelineLayout
            : public RHI::Object
        {
            friend class PipelineLayoutCache;
        public:
            AZ_CLASS_ALLOCATOR(PipelineLayout, AZ::SystemAllocator);
            PipelineLayout() = default;
            ~PipelineLayout();
            
            void Init(id<MTLDevice> metalDevice, const RHI::PipelineLayoutDescriptor& descriptor);
            
            //Returns if an SRG is used at all
            const bool IsSRGUsed(size_t index) const;
            
            // Returns the hash of the pipeline layout provided by the descriptor.
            AZ::HashValue64 GetHash() const;
            
            /// Returns the SRG binding slot associated with the SRG flat index.
            size_t GetSlotByIndex(size_t index) const;
            
            /// Returns the SRG flat index associated with the SRG binding slot.
            size_t GetIndexBySlot(size_t slot) const;
            
            /// Returns the visibility enum with the SRG binding slot.
            const RHI::ShaderStageMask& GetSrgVisibility(uint32_t index) const;
            
            /// Returns srgVisibility data
            const ShaderResourceGroupVisibility& GetSrgResourcesVisibility(uint32_t index) const;
            
            /// Returns srgVisibility hash
            const AZ::HashValue64 GetSrgResourcesVisibilityHash(uint32_t index) const;
            
            /// Returns the root constant specific layout information
            uint32_t GetRootConstantsSize() const;
            uint32_t GetRootConstantsSlotIndex() const;
        private:
            PipelineLayout(PipelineLayoutCache& parentCache);
            
            template <typename T>
            friend struct AZStd::IntrusivePtrCountPolicy;
            
            void add_ref() const;
            void release() const;
            
            PipelineLayoutCache* m_parentCache = nullptr;
            AZ::HashValue64 m_hash = AZ::HashValue64{0u};
            AZStd::atomic_bool m_isCompiled = {false};
            mutable AZStd::atomic_int m_useCount = {0};

            /// Tables for mapping between SRG slots (sparse) to SRG indices (packed).
            AZStd::array<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_slotToIndexTable;
            AZStd::fixed_vector<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_indexToSlotTable;
            
            /// Visibility for SRG as a whole
            AZStd::fixed_vector<RHI::ShaderStageMask, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgVisibilities;
            
            /// Cache Visibility across all the resources within the SRG
            AZStd::fixed_vector<ShaderResourceGroupVisibility, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgResourcesVisibility;
            
            /// Cache Visibility hash across all the resources within the SRG
            AZStd::fixed_vector<AZ::HashValue64, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgResourcesVisibilityHash;
            
            uint32_t m_rootConstantSlotIndex = (uint32_t)-1;
            uint32_t m_rootConstantsSize = 0;
        };

        class PipelineLayoutCache final
        {
            friend class PipelineLayout;
        public:
            PipelineLayoutCache() = default;
            
            void Init(Device& device);
            
            void Shutdown();
            
            // Allocates an instance of a pipeline layout from a descriptor.
            RHI::ConstPtr<PipelineLayout> Allocate(const RHI::PipelineLayoutDescriptor& descriptor);
            
        private:
            
            // Attempts to release the PipelineLayout from the cache, but checks to make sure
            // a reference wasn't taken by another thread.
            void TryReleasePipelineLayout(const PipelineLayout* pipelineLayout);
            
            AZStd::mutex m_mutex;
            AZStd::unordered_map<AZ::HashValue64, PipelineLayout*> m_pipelineLayouts;
            
            Device* m_parentDevice = nullptr;
        };
    }
}
