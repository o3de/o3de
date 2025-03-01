/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourceView.h>

namespace AZ::RHI
{
    class DeviceBuffer;
    class DeviceBufferView;

    //! A BufferView is a light-weight representation of a view onto a multi-device buffer.
    //! It holds a ConstPtr to a multi-device buffer as well as a BufferViewDescriptor
    //! Using both, single-device BufferViews can be retrieved
    class BufferView : public ResourceView
    {
    public:
        AZ_RTTI(BufferView, "{AB366B8F-F1B7-45C6-A0D8-475D4834FAD2}", ResourceView);
        virtual ~BufferView() = default;

        BufferView(const RHI::Buffer* buffer, BufferViewDescriptor descriptor, MultiDevice::DeviceMask deviceMask)
            : ResourceView{ buffer, deviceMask }
            , m_descriptor{ descriptor }
        {
        }

        //! Given a device index, return the corresponding DeviceBufferView for the selected device
        const RHI::Ptr<RHI::DeviceBufferView> GetDeviceBufferView(int deviceIndex) const;

        //! Return the contained multi-device buffer
        const RHI::Buffer* GetBuffer() const
        {
            return static_cast<const RHI::Buffer*>(GetResource());
        }

        //! Return the contained BufferViewDescriptor
        const BufferViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        AZStd::unordered_map<int, uint32_t> GetBindlessReadIndex() const;

        const DeviceResourceView* GetDeviceResourceView(int deviceIndex) const override
        {
            return GetDeviceBufferView(deviceIndex).get();
        }

        // Convenient helper function to get both of the indices (Read and ReadWrite) in the BindlessSrg for a given device:
        //     - If @outReadWriteIndex != nullptr, it will get the shader index (ReadWrite) in Bindless::m_RWByteAddressBuffer[].
        //     See "/o3de/Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features\Bindless.azsli" for details.
        // Returns the shader index (READ) in Bindless::m_ByteAddressBuffer[].
        uint32_t GetBindlessIndices(int deviceIndex, uint32_t* outReadWriteIndex = nullptr) const;

    private:
        //! The corresponding BufferViewDescriptor for this view.
        BufferViewDescriptor m_descriptor;
    };
}
