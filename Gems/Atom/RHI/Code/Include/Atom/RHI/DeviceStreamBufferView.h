/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    class DeviceBuffer;
    class InputStreamLayout;

    //! Provides a view into a buffer, to be used as vertex stream. The content of the view is a contiguous
    //! list of input vertex data. It is provided to the RHI back-end at draw time.
    //!
    //! Note that the buffer is further described in InputStreamLayout, through StreamChannelDescriptors
    //! and a StreamBufferDescriptor, which is provided to the RHI back-end at PSO compile time.
    //! - The view will be associated with one or more StreamChannelDescriptors to describe its specific content. 
    //!   Channels maybe be stored in separate StreamBufferViews (each view having a separate StreamChannelDescriptor)
    //!   or interleaved in a single DeviceStreamBufferView (one view having multiple StreamChannelDescriptors).
    //! - The view will correspond to a single StreamBufferDescriptor.
    class alignas(8) DeviceStreamBufferView
    {
    public:
        DeviceStreamBufferView() = default;

        DeviceStreamBufferView(
            const DeviceBuffer& buffer,
            uint32_t byteOffset,
            uint32_t byteCount,
            uint32_t byteStride);

        //! Returns the hash of the view. This hash is precomputed at creation time.
        HashValue64 GetHash() const;

        //! Returns the buffer associated with the view.
        const DeviceBuffer* GetBuffer() const;

        //! Returns the byte offset into the buffer.
        uint32_t GetByteOffset() const;

        //! Returns the number of bytes in the view.
        uint32_t GetByteCount() const;

        //! Returns the distance in bytes between consecutive vertex entries in the buffer.
        //! This must match the stride value in StreamBufferDescriptor.
        uint32_t GetByteStride() const;

        bool operator==(const DeviceStreamBufferView& other) const;

    private:
        HashValue64 m_hash = HashValue64{ 0 };
        const DeviceBuffer* m_buffer = nullptr;
        uint32_t m_byteOffset = 0;
        uint32_t m_byteCount = 0;
        uint32_t m_byteStride = 0;
    };

    //! Utility function for checking that the set of StreamBufferViews aligns with the InputStreamLayout
    bool ValidateStreamBufferViews(const InputStreamLayout& inputStreamLayout, AZStd::span<const DeviceStreamBufferView> streamBufferViews);
}
