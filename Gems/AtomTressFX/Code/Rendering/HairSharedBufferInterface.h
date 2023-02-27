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

    namespace Render
    {
        class HairSharedBufferAllocation;

        //! A class for allocating memory for skinning buffers
        class HairSharedBufferInterface
        {
        public:
            AZ_RTTI(AZ::Render::HairSharedBufferInterface, "{3CCB13CB-16FF-43F5-98DC-F36B2A9F8E5E}");

            HairSharedBufferInterface()
            {
                Interface<HairSharedBufferInterface>::Register(this);
            }

            virtual ~HairSharedBufferInterface()
            {
                Interface<HairSharedBufferInterface>::Unregister(this);
            }

            static HairSharedBufferInterface* Get()
            {
                return Interface<HairSharedBufferInterface>::Get();
            }

            //! Returns the shared buffer asset used for all Hair objects and passes
            virtual Data::Asset<RPI::BufferAsset> GetBufferAsset() const = 0;

            //! Returns the buffer that is used for all skinned mesh outputs
            virtual Data::Instance<RPI::Buffer> GetBuffer() = 0;

            //! If the allocation succeeds, returns a ref-counted pointer to a VirtualAddress which will be automatically freed if the ref-count drops to zero
            //! If the allocation fails, returns nullptr
            virtual AZStd::intrusive_ptr<HairSharedBufferAllocation> Allocate(size_t byteCount) = 0;

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
            AZ_DISABLE_COPY_MOVE(HairSharedBufferInterface);
        };

        class HairSharedBufferNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! This event will fire if memory is freed up, so a listener can wait for there to be free space
            //! and attempt to allocate memory again if it failed initially
            virtual void OnSharedBufferMemoryAvailable() = 0;
        };
        using SharedBufferNotificationBus = AZ::EBus<HairSharedBufferNotifications>;

        //! An intrusive_ptr wrapper around an RHI::Allocation that will automatically free the memory
        //! from the SharedBuffer when the ref count drops to zero
        class HairSharedBufferAllocation
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(HairSharedBufferAllocation, AZ::SystemAllocator);
            explicit HairSharedBufferAllocation(RHI::VirtualAddress virtualAddress)
                : m_virtualAddress(virtualAddress)
            {}

            ~HairSharedBufferAllocation()
            {
                if (!m_suppressSignalOnDeallocate)
                {
                    HairSharedBufferInterface::Get()->DeAllocate(m_virtualAddress);
                }
                else
                {
                    HairSharedBufferInterface::Get()->DeAllocateNoSignal(m_virtualAddress);
                }
            }

            //! If this function is called, the SharedBuffer will not signal when the memory is freed
            void SuppressSignalOnDeallocate() { m_suppressSignalOnDeallocate = true; }
            RHI::VirtualAddress GetVirtualAddress() const { return m_virtualAddress; }
        private:
            RHI::VirtualAddress m_virtualAddress;
            bool m_suppressSignalOnDeallocate = false;
        };
    } // namespace Render
} // namespace AZ
