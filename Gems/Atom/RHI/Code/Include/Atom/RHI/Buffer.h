/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/Resource.h>

namespace AZ
{
    namespace RHI
    {
        struct BufferViewDescriptor;

        //! A buffer corresponds to a region of linear memory and used for rendering operations. The user
        //! manages the lifecycle of a buffer through a BufferPool.
        class Buffer : public Resource
        {
            using Base = Resource;
            friend class BufferPool;
            friend class BufferView;
            friend class RayTracingTlas;
            friend class TransientAttachmentPool;

        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator, 0);
            AZ_RTTI(Buffer, "{C5B6E979-E8CA-474B-BD9F-6A311A9EE07E}", Resource);
            Buffer() = default;
            virtual ~Buffer() = default;

            const BufferDescriptor& GetDescriptor() const;

            inline const Ptr<DeviceBuffer>& GetDeviceBuffer(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceBuffers.find(deviceIndex) != m_deviceBuffers.end(), "No DeviceBuffer found for device index %d\n", deviceIndex);
                return m_deviceBuffers.at(deviceIndex);
            }

            Ptr<BufferView> GetBufferView(const BufferViewDescriptor& bufferViewDescriptor);

            // Get the hash associated with the Buffer
            const HashValue64 GetHash() const;

        protected:
            void SetDescriptor(const BufferDescriptor& descriptor);

        private:
            void Invalidate();

            // The RHI descriptor for this buffer.
            BufferDescriptor m_descriptor;

            AZStd::unordered_map<int, Ptr<DeviceBuffer>> m_deviceBuffers;
        };
    } // namespace RHI
} // namespace AZ
