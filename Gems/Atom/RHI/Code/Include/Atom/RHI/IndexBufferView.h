/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    class Buffer;

    uint32_t GetIndexFormatSize(IndexFormat indexFormat);

    //! A multi-device class representing a view onto a Buffer holding indices, distinct from
    //! actual view classes (like DeviceBufferView), there is no representation on the API level.
    //! Its device-specific buffers are provided to the RHI back-end at draw time.
    class alignas(8) IndexBufferView
    {
    public:
        IndexBufferView() = default;

        IndexBufferView(const Buffer& buffer, uint32_t byteOffset, uint32_t byteCount, IndexFormat format);

        //! Returns the device-specific DeviceIndexBufferView for the given index
        DeviceIndexBufferView GetDeviceIndexBufferView(int deviceIndex) const;

        //! Returns the hash of the view. This hash is precomputed at creation time.
        HashValue64 GetHash() const;

        //! Returns the buffer associated with the data in the view.
        const Buffer* GetBuffer() const;

        //! Returns the byte offset into the buffer returned by GetBuffer
        uint32_t GetByteOffset() const;

        //! Returns the number of bytes in the view.
        uint32_t GetByteCount() const;

        //! Returns the format of each index in the view.
        IndexFormat GetIndexFormat() const;

        //! Returns whether the view is valid and points to a buffer
        bool IsValid() const { return m_buffer != nullptr; }

    private:
        HashValue64 m_hash = HashValue64{ 0 };
        const Buffer* m_buffer = nullptr;
        uint32_t m_byteOffset = 0;
        uint32_t m_byteCount = 0;
        IndexFormat m_format = IndexFormat::Uint32;
    };
} // namespace AZ::RHI
