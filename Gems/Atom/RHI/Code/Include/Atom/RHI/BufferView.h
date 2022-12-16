/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/ResourceView.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    namespace RHI
    {
        class Buffer;

        //! BufferView is contains a platform-specific descriptor mapping to a linear sub-region of a specific buffer resource.
        //! It associates 1-to-1 with a BufferViewDescriptor.
        class BufferView
            : public ResourceView
        {
        public:
            // Using SystemAllocator here instead of ThreadPoolAllocator as it gets slower when
            // we create thousands of buffer views related to SRGs
            AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator, 0);
            AZ_RTTI(BufferView, "{308564F0-9CBF-4863-B74B-1BCFE8C8E316}", ResourceView);
            virtual ~BufferView() = default;

            //! Initializes the buffer view with the provided buffer and view descriptor.
            ResultCode Init(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor);

            //! Returns the view descriptor used at initialization time.
            const BufferViewDescriptor& GetDescriptor() const;

            //! Returns the buffer associated with this view.
            const Buffer& GetBuffer() const;

            inline Ptr<DeviceBufferView> GetDeviceBufferView(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceBufferViews.find(deviceIndex) != m_deviceBufferViews.end(),
                    "No DeviceBufferView found for device index %d\n",
                    deviceIndex);
                return m_deviceBufferViews.at(deviceIndex);
            }

            Ptr<DeviceResourceView> GetDeviceResourceView(int deviceIndex) const override;

            //! Returns whether the view maps to the full buffer.
            bool IsFullView() const override final;

            //! Tells the renderer to ignore any validation related to this buffer's state and scope attachments.
            //! Assumes that the programmer is manually managing the Read/Write state of the buffer correctly.
            bool IgnoreFrameAttachmentValidation() const
            {
                return m_descriptor.m_ignoreFrameAttachmentValidation;
            }

            //! Returns the hash of the view.
            HashValue64 GetHash() const;

        protected:
            HashValue64 m_hash = HashValue64{ 0 };

        private:
            ResultCode InitInternal(const Resource& resource) override;
            void ShutdownInternal() override;
            ResultCode InvalidateInternal() override;

            bool ValidateForInit(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor) const;

            /// The RHI descriptor for this view.
            BufferViewDescriptor m_descriptor;

            AZStd::unordered_map<int, Ptr<DeviceBufferView>> m_deviceBufferViews;
        };
    }
}
