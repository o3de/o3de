/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/DX12/PipelineLayoutDescriptor.h>
#include <Atom/RHI/Object.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/unordered_map.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class PipelineLayoutCache;

        /**
         * PipelineLayouts are created from a cache. They are internally de-duplicated using the hash value computed
         * by the descriptor. Ownership of a particular element in the cache is still externally managed (via ConstPtr).
         * When all references to a particular instance are destroyed, the object is unregistered from the cache.
         * Therefore, ownership is still managed by the application and the cache doesn't grow unbounded.
         */
        class PipelineLayout final
        {
            friend class PipelineLayoutCache;
        public:
            AZ_CLASS_ALLOCATOR(PipelineLayout, AZ::SystemAllocator);
            PipelineLayout() = default;
            ~PipelineLayout();

            /// Initializes the pipeline layout.
            void Init(ID3D12DeviceX* dx12Device, const RHI::PipelineLayoutDescriptor& descriptor);

            /// Returns the number of root parameter bindings (1-to-1 with SRG).
            size_t GetRootParameterBindingCount() const;

            /// Returns the root parameter binding for the flat index.
            RootParameterBinding GetRootParameterBindingByIndex(size_t index) const;

            /// Returns the root parameter index for the Root Constants.
            RootParameterIndex GetRootConstantsRootParameterIndex() const;

            /// Returns the SRG binding slot associated with the SRG flat index.
            size_t GetSlotByIndex(size_t index) const;

            /// Returns the SRG flat index associated with the SRG binding slot.
            size_t GetIndexBySlot(size_t slot) const;

            /// Returns whether this pipeline layout has inline constants.
            bool HasRootConstants() const;

            const RHI::PipelineLayoutDescriptor& GetPipelineLayoutDescriptor() const;

            /// Returns the platform pipeline layout object.
            ID3D12RootSignature* Get() const;

            /// Returns the hash of the pipeline layout provided by the descriptor.
            HashValue64 GetHash() const;

        private:
            PipelineLayout(PipelineLayoutCache& parentPool);

            template <typename T>
            friend struct AZStd::IntrusivePtrCountPolicy;

            void add_ref() const;
            void release() const;

            /// Tables for mapping between SRG slots (sparse) to SRG indices (packed).
            AZStd::array<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_slotToIndexTable;
            AZStd::fixed_vector<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_indexToSlotTable;

            /// Table for mapping SRG index (packed) to Root Parameter Binding (DX12 command list bindings).
            AZStd::fixed_vector<RootParameterBinding, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_indexToRootParameterBindingTable;

            /// Root Parameter Index for root constants.
            RootParameterIndex m_rootConstantsRootParameterIndex;

            /// Tracks whether this pipeline layout has inline constants.
            bool m_hasRootConstants = false;

            RHI::Ptr<ID3D12RootSignature> m_signature;
            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_layoutDescriptor;
            HashValue64 m_hash = HashValue64{ 0 };
            PipelineLayoutCache* m_parentCache = nullptr;
            AZStd::atomic_bool m_isCompiled = {false};
            mutable AZStd::atomic_int m_useCount = {0};
        };

        class PipelineLayoutCache final
        {
            friend class PipelineLayout;
        public:
            PipelineLayoutCache() = default;

            void Init(Device& device);

            void Shutdown();

            /// Allocates an instance of a pipeline layout from a descriptor.
            RHI::ConstPtr<PipelineLayout> Allocate(const RHI::PipelineLayoutDescriptor& descriptor);

        private:
            //////////////////////////////////////////////////////////////////////////
            // Private API for PipelineLayout

            /// Attempts to release the PipelineLayout from the cache, but checks to make sure
            /// a reference wasn't taken by another thread.
            void TryReleasePipelineLayout(const PipelineLayout* pipelineLayout);

            //////////////////////////////////////////////////////////////////////////

            AZStd::mutex m_mutex;
            AZStd::unordered_map<uint64_t, PipelineLayout*> m_pipelineLayouts;

            // NOTE: This is a raw pointer because Device owns the cache.
            Device* m_parentDevice = nullptr;
        };
    }
}
