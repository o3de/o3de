/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/SingleDeviceStreamBufferView.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/span.h>

namespace AZ::RHI
{
    class MultiDeviceBuffer;
    class InputStreamLayout;

    //! Provides a view into a multi-device buffer, to be used as vertex stream. The content of the view is a contiguous
    //! list of input vertex data. Its device-specific buffer is provided to the RHI back-end at draw time for a given device.
    class alignas(8) MultiDeviceStreamBufferView
    {
    public:
        MultiDeviceStreamBufferView() = default;

        MultiDeviceStreamBufferView(const MultiDeviceBuffer& buffer, uint32_t byteOffset, uint32_t byteCount, uint32_t byteStride);

        //! Returns the device-specific SingleDeviceStreamBufferView for the given index
        SingleDeviceStreamBufferView GetDeviceStreamBufferView(int deviceIndex) const
        {
            AZ_Assert(m_mdBuffer, "No MultiDeviceBuffer available\n");

            return SingleDeviceStreamBufferView(*m_mdBuffer->GetDeviceBuffer(deviceIndex), m_byteOffset, m_byteCount, m_byteStride);
        }

        //! Returns the hash of the view. This hash is precomputed at creation time.
        HashValue64 GetHash() const;

        //! Returns the buffer associated with the view.
        const MultiDeviceBuffer* GetBuffer() const;

        //! Returns the byte offset into the buffer.
        uint32_t GetByteOffset() const;

        //! Returns the number of bytes in the view.
        uint32_t GetByteCount() const;

        //! Returns the distance in bytes between consecutive vertex entries in the buffer.
        //! This must match the stride value in StreamBufferDescriptor.
        uint32_t GetByteStride() const;

    private:
        HashValue64 m_hash = HashValue64{ 0 };
        const MultiDeviceBuffer* m_mdBuffer = nullptr;
        uint32_t m_byteOffset = 0;
        uint32_t m_byteCount = 0;
        uint32_t m_byteStride = 0;
    };

    //! Utility function for checking that the set of StreamBufferViews aligns with the InputStreamLayout
    bool ValidateStreamBufferViews(
        const InputStreamLayout& inputStreamLayout, AZStd::span<const MultiDeviceStreamBufferView> streamBufferViews);
} // namespace AZ::RHI
