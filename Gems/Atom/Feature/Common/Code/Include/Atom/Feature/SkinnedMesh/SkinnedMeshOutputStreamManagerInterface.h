/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/Event.h>

#include <AtomCore/Instance/Instance.h>
#include <Atom/RHI/Allocator.h>

namespace AZ
{
    namespace RPI
    {
        class BufferAsset;
        class Buffer;
    }

    namespace Render
    {
        class SkinnedMeshOutputStreamAllocation;

        //! A class for allocating memory for skinning buffers
        class SkinnedMeshOutputStreamManagerInterface
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshOutputStreamManagerInterface, "{14536F49-FF76-4B71-B0F4-5E5B73FA4C04}");

            SkinnedMeshOutputStreamManagerInterface()
            {
                Interface<SkinnedMeshOutputStreamManagerInterface>::Register(this);
            }

            virtual ~SkinnedMeshOutputStreamManagerInterface()
            {
                Interface<SkinnedMeshOutputStreamManagerInterface>::Unregister(this);
            }

            static SkinnedMeshOutputStreamManagerInterface* Get()
            {
                return Interface<SkinnedMeshOutputStreamManagerInterface>::Get();
            }

            //! Returns the buffer asset that is used for all skinned mesh outputs
            virtual Data::Asset<RPI::BufferAsset> GetBufferAsset() = 0;

            //! Returns the buffer that is used for all skinned mesh outputs
            virtual Data::Instance<RPI::Buffer> GetBuffer() = 0;

            //! If the allocation succeeds, returns a ref-counted pointer to a VirtualAddress which will be automatically freed if the ref-count drops to zero
            //! If the allocation fails, returns nullptr
            virtual AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> Allocate(size_t byteCount) = 0;

            //! Mark the memory as available and queue garbage collection to recycle it later (see RHI::Allocator::DeAllocate)
            //! After garbage collection is done signal handlers that memory has been freed
            virtual void DeAllocate(RHI::VirtualAddress allocation) = 0;

            //! Same as DeAllocate, but the signal after garbage collection is ignored
            //! If multiple allocations succeeded before one failed, use this to release the successful allocations
            //! without triggering new events indicating that new memory has been freed
            virtual void DeAllocateNoSignal(RHI::VirtualAddress allocation) = 0;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(SkinnedMeshOutputStreamManagerInterface);
        };

        class SkinnedMeshOutputStreamNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! This event will fire if memory is freed up, so a listener can wait for there to be free space
            //! and attempt to allocate memory again if it failed initially
            virtual void OnSkinnedMeshOutputStreamMemoryAvailable() = 0;
        };
        using SkinnedMeshOutputStreamNotificationBus = AZ::EBus<SkinnedMeshOutputStreamNotifications>;

        //! An intrusive_ptr wrapper around an RHI::Allocation that will automatically free the memory
        //! from the SkinnedMeshOutputStreamManager when the refcount drops to zero
        class SkinnedMeshOutputStreamAllocation
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshOutputStreamAllocation, AZ::SystemAllocator);
            explicit SkinnedMeshOutputStreamAllocation(RHI::VirtualAddress virtualAddress)
                : m_virtualAddress(virtualAddress)
            {}

            ~SkinnedMeshOutputStreamAllocation()
            {
                if (!m_suppressSignalOnDeallocate)
                {
                    SkinnedMeshOutputStreamManagerInterface::Get()->DeAllocate(m_virtualAddress);
                }
                else
                {
                    SkinnedMeshOutputStreamManagerInterface::Get()->DeAllocateNoSignal(m_virtualAddress);
                }
            }

            //! If this function is called, the SkinnedMeshOutputStreamManager will not signal when the memory is freed
            void SuppressSignalOnDeallocate() { m_suppressSignalOnDeallocate = true; }
            RHI::VirtualAddress GetVirtualAddress() const { return m_virtualAddress; }
        private:
            RHI::VirtualAddress m_virtualAddress;
            bool m_suppressSignalOnDeallocate = false;
        };
    } // namespace Render
} // namespace AZ
