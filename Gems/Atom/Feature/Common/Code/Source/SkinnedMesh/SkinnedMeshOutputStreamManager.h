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
            Data::Asset<RPI::BufferAsset> GetBufferAsset() const override;
            Data::Instance<RPI::Buffer> GetBuffer() override;

        private:
            // SystemTickBus
            void OnSystemTick() override;

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
        };
    } // namespace Render
} // namespace AZ
