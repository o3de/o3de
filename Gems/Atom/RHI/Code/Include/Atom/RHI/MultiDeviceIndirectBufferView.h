/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/MultiDeviceIndirectBufferSignature.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    class MultiDeviceBuffer;
    class MultiDeviceIndirectBufferSignature;

    //! Provides a view into a multi-device buffer, to be used as an indirect buffer. The content of the view is a contiguous
    //! list of commands sequences. Its device-specific buffers are provided to the RHI back-end at draw time.
    class alignas(8) MultiDeviceIndirectBufferView
    {
    public:
        MultiDeviceIndirectBufferView() = default;

        MultiDeviceIndirectBufferView(
            const MultiDeviceBuffer& buffer,
            const MultiDeviceIndirectBufferSignature& signature,
            uint32_t byteOffset,
            uint32_t byteCount,
            uint32_t byteStride);

        //! Returns the device-specific IndirectBufferView for the given index
        IndirectBufferView GetDeviceIndirectBufferView(int deviceIndex) const
        {
            AZ_Error("MultiDeviceIndirectBufferView", m_mdSignature, "No MultiDeviceIndirectBufferSignature available\n");
            AZ_Error("MultiDeviceIndirectBufferView", m_mdBuffer, "No MultiDeviceBuffer available\n");

            return IndirectBufferView(
                *m_mdBuffer->GetDeviceBuffer(deviceIndex),
                *m_mdSignature->GetDeviceIndirectBufferSignature(deviceIndex),
                m_byteOffset,
                m_byteCount,
                m_byteStride);
        }

        //! Returns the hash of the view. This hash is precomputed at creation time.
        HashValue64 GetHash() const;

        //! Returns the buffer associated with the view.
        const MultiDeviceBuffer* GetBuffer() const;

        //! Returns the byte offset into the buffer.
        uint32_t GetByteOffset() const;

        //! Returns the number of bytes in the view.
        uint32_t GetByteCount() const;

        //! Returns the distance in bytes between consecutive commands sequences.
        //! This must be larger or equal than the stride specify by the signature.
        uint32_t GetByteStride() const;

        //! Returns the signature of the indirect buffer that is associated with the view.
        const MultiDeviceIndirectBufferSignature* GetSignature() const;

    private:
        HashValue64 m_hash = HashValue64{ 0 };
        const MultiDeviceIndirectBufferSignature* m_mdSignature = nullptr;
        const MultiDeviceBuffer* m_mdBuffer = nullptr;
        uint32_t m_byteOffset = 0;
        uint32_t m_byteCount = 0;
        uint32_t m_byteStride = 0;
    };
} // namespace AZ::RHI
