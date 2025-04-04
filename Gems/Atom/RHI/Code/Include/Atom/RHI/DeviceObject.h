/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>

namespace AZ::RHI
{
    //! A variant of Object associated with a Device instance. It holds a strong reference to
    //! the device and provides a simple accessor API.
    class DeviceObject
        : public Object
    {
    public:
        AZ_RTTI(DeviceObject, "{17D34F71-944C-4AF5-9823-627474C4C0A6}", Object);
        virtual ~DeviceObject() = default;

        /// Returns whether the device object is initialized.
        bool IsInitialized() const;

        //! Returns the device this object is associated with. It is only permitted to call
        //! this method when the object is initialized.
        Device& GetDevice() const;

    protected:
        DeviceObject() = default;

        /// The derived class should call this method to assign the device.
        void Init(Device& device);

        /// Clears the current bound device to null.
        void Shutdown() override;

    private:
        Ptr<Device> m_device = nullptr;
    };
}
