/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/IndexBufferView.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceBuffer;

        uint32_t GetIndexFormatSize(IndexFormat indexFormat);

        class alignas(8) MultiDeviceIndexBufferView
        {
        public:
            MultiDeviceIndexBufferView() = default;

            MultiDeviceIndexBufferView(const MultiDeviceBuffer& buffer, uint32_t byteOffset, uint32_t byteCount, IndexFormat format);

            IndexBufferView GetDeviceIndexBufferView(int deviceIndex) const;

            /// Returns the hash of the view. This hash is precomputed at creation time.
            HashValue64 GetHash() const;

            /// Returns the buffer associated with the data in the view.
            const MultiDeviceBuffer* GetBuffer() const;

            /// Returns the byte offset into the buffer returned by GetBuffer
            uint32_t GetByteOffset() const;

            /// Returns the number of bytes in the view.
            uint32_t GetByteCount() const;

            /// Returns the format of each index in the view.
            IndexFormat GetIndexFormat() const;

        private:
            HashValue64 m_hash = HashValue64{ 0 };
            const MultiDeviceBuffer* m_buffer = nullptr;
            uint32_t m_byteOffset = 0;
            uint32_t m_byteCount = 0;
            IndexFormat m_format = IndexFormat::Uint32;
        };
    } // namespace RHI
} // namespace AZ
