/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/DeviceIndirectBufferView.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    class Buffer;
    class IndirectBufferSignature;

    //! Provides a view into a multi-device buffer, to be used as an indirect buffer. The content of the view is a contiguous
    //! list of commands sequences. Its device-specific buffers are provided to the RHI back-end at draw time.
    class alignas(8) IndirectBufferView
    {
    public:
        IndirectBufferView() = default;

        IndirectBufferView(
            const Buffer& buffer,
            const IndirectBufferSignature& signature,
            uint32_t byteOffset,
            uint32_t byteCount,
            uint32_t byteStride);

        //! The mutex stops the default generation
        IndirectBufferView(const IndirectBufferView& other)
            : m_hash{ other.m_hash }
            , m_buffer{ other.m_buffer }
            , m_signature{ other.m_signature }
            , m_byteOffset{ other.m_byteOffset }
            , m_byteCount{ other.m_byteCount }
            , m_byteStride{ other.m_byteStride }
        {
        }

        IndirectBufferView& operator=(const IndirectBufferView& other)
        {
            this->m_hash = other.m_hash;
            this->m_buffer = other.m_buffer;
            this->m_signature = other.m_signature;
            this->m_byteOffset = other.m_byteOffset;
            this->m_byteCount = other.m_byteCount;
            this->m_byteStride = other.m_byteStride;

            m_cache.clear();

            return *this;
        }

        //! Returns the device-specific DeviceIndirectBufferView for the given index
        const DeviceIndirectBufferView& GetDeviceIndirectBufferView(int deviceIndex) const
        {
            AZ_Error("IndirectBufferView", m_signature, "No IndirectBufferSignature available\n");
            AZ_Error("IndirectBufferView", m_buffer, "No Buffer available\n");

            AZStd::lock_guard lock(m_bufferViewMutex);
            auto iterator{ m_cache.find(deviceIndex) };
            if (iterator == m_cache.end())
            {
                //! Buffer view is not yet in the cache
                auto [new_iterator, inserted]{ m_cache.insert(AZStd::make_pair(
                    deviceIndex,
                    DeviceIndirectBufferView(
                        *m_buffer->GetDeviceBuffer(deviceIndex),
                        *m_signature->GetDeviceIndirectBufferSignature(deviceIndex),
                        m_byteOffset,
                        m_byteCount,
                        m_byteStride))) };
                if (inserted)
                {
                    return new_iterator->second;
                }
            }

            return iterator->second;
        }

        //! Returns the hash of the view. This hash is precomputed at creation time.
        HashValue64 GetHash() const;

        //! Returns the buffer associated with the view.
        const Buffer* GetBuffer() const;

        //! Returns the byte offset into the buffer.
        uint32_t GetByteOffset() const;

        //! Returns the number of bytes in the view.
        uint32_t GetByteCount() const;

        //! Returns the distance in bytes between consecutive commands sequences.
        //! This must be larger or equal than the stride specify by the signature.
        uint32_t GetByteStride() const;

        //! Returns the signature of the indirect buffer that is associated with the view.
        const IndirectBufferSignature* GetSignature() const;

    private:
        HashValue64 m_hash = HashValue64{ 0 };
        const IndirectBufferSignature* m_signature = nullptr;
        const Buffer* m_buffer = nullptr;
        uint32_t m_byteOffset = 0;
        uint32_t m_byteCount = 0;
        uint32_t m_byteStride = 0;

        //! Safe-guard access to DeviceIndirectBufferView cache during parallel access
        mutable AZStd::mutex m_bufferViewMutex{};
        mutable AZStd::unordered_map<int, DeviceIndirectBufferView> m_cache;
    };
} // namespace AZ::RHI
