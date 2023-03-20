/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>

#include <Atom/RHI/FreeListAllocator.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>

namespace AZ
{
    namespace RPI
    {
        class BufferAsset;
        class Buffer;
    }

    namespace Render
    {
        class SkinnedMeshOutputStreamManager
            : public SkinnedMeshOutputStreamManagerInterface
            , private SystemTickBus::Handler
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshOutputStreamManager, "{3107EC84-DDF6-46FD-8B2C-00431D1BB67C}", AZ::Render::SkinnedMeshOutputStreamManagerInterface);

            SkinnedMeshOutputStreamManager();
            ~SkinnedMeshOutputStreamManager();
            void Init();

            // SkinnedMeshOutputStreamManagerInterface
            AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> Allocate(size_t byteCount) override;
            void DeAllocate(RHI::VirtualAddress allocation) override;
            void DeAllocateNoSignal(RHI::VirtualAddress allocation) override;
            Data::Asset<RPI::BufferAsset> GetBufferAsset() override;
            Data::Instance<RPI::Buffer> GetBuffer() override;

        private:
            // SystemTickBus
            void OnSystemTick() override;

            void EnsureInit();
            void GarbageCollect();
            void CalculateAlignment();
            void CreateBufferAsset();

            Data::Asset<RPI::BufferAsset> m_bufferAsset;
            Data::Instance<RPI::Buffer> m_buffer;
            RHI::FreeListAllocator m_freeListAllocator;
            AZStd::mutex m_allocatorMutex;
            uint64_t m_alignment = 1;
            size_t m_sizeInBytes = 0;
            bool m_memoryWasFreed = false;
            bool m_broadcastMemoryAvailableEvent = false;
            bool m_needsInit = true;
        };
    } // namespace Render
} // namespace AZ
