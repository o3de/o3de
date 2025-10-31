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
    namespace RHI
    {
        class Buffer;
        class BufferView;
    }

    namespace RPI
    {
        class BufferAsset;
        class Buffer;
    }

    namespace Meshlets
    {
        class SharedBufferAllocation;

        //! A class for allocating memory for skinning buffers
        class SharedBufferInterface
        {
        public:
            AZ_RTTI(AZ::Meshlets::SharedBufferInterface, "{6048DAF9-7A05-41B3-94C8-FBBDB3A187D2}");

            SharedBufferInterface()
            {
                Interface<SharedBufferInterface>::Register(this);
            }

            virtual ~SharedBufferInterface()
            {
                Interface<SharedBufferInterface>::Unregister(this);
            }

            static SharedBufferInterface* Get()
            {
                return Interface<SharedBufferInterface>::Get();
            }

            //! Returns the buffer that is used for all skinned mesh outputs
            virtual Data::Instance<RPI::Buffer> GetBuffer() = 0;

            //! If the allocation succeeds, returns a ref-counted pointer to a VirtualAddress which will be automatically freed if the ref-count drops to zero
            //! If the allocation fails, returns nullptr
            virtual AZStd::intrusive_ptr<SharedBufferAllocation> Allocate(size_t byteCount) = 0;

            //! Mark the memory as available and queue garbage collection to recycle it later (see RHI::Allocator::DeAllocate)
            //! After garbage collection is done signal handlers that memory has been freed
            virtual void DeAllocate(RHI::VirtualAddress allocation) = 0;

            //! Same as DeAllocate, but the signal after garbage collection is ignored
            //! If multiple allocations succeeded before one failed, use this to release the successful allocations
            //! without triggering new events indicating that new memory has been freed
            virtual void DeAllocateNoSignal(RHI::VirtualAddress allocation) = 0;

            //! Update buffer's content with sourceData at an offset of bufferByteOffset
            virtual bool UpdateData(const void* sourceData, uint64_t sourceDataSizeInBytes, uint64_t bufferByteOffset = 0) = 0;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(SharedBufferInterface);
        };

        class SharedBufferNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! This event will fire if memory is freed up, so a listener can wait for there to be free space
            //! and attempt to allocate memory again if it failed initially
            virtual void OnSharedBufferMemoryAvailable() = 0;
        };
        using SharedBufferNotificationBus = AZ::EBus<SharedBufferNotifications>;

        //======================================================================
        //! An intrusive_ptr wrapper around an RHI::Allocation that will automatically free
        //!  the memory from the SharedBuffer when the ref count drops to zero.
        //! Allocated memory will be cleared using the underlying allocator system and
        //!  indirectly the garbage collection.
        //! Since the garbage collection is ran with delay of 3 frames due to CPU-GPU
        //!  latency, this might result in over allocation at reset / back from game mode.
        //======================================================================
        class SharedBufferAllocation
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedBufferAllocation, AZ::SystemAllocator);
            explicit SharedBufferAllocation(RHI::VirtualAddress virtualAddress)
                : m_virtualAddress(virtualAddress)
            {}

            ~SharedBufferAllocation()
            {
                if (!m_suppressSignalOnDeallocate)
                {
                    SharedBufferInterface::Get()->DeAllocate(m_virtualAddress);
                }
                else
                {
                    SharedBufferInterface::Get()->DeAllocateNoSignal(m_virtualAddress);
                }
            }

            //! If this function is called, the SharedBuffer will not signal when the memory is freed
            void SuppressSignalOnDeallocate() { m_suppressSignalOnDeallocate = true; }
            RHI::VirtualAddress GetVirtualAddress() const { return m_virtualAddress; }
        private:
            RHI::VirtualAddress m_virtualAddress;
            bool m_suppressSignalOnDeallocate = false;
        };
    } // namespace Meshlets
} // namespace AZ
