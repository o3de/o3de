/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>

#define AZ_RHI_MULTI_DEVICE_OBJECT_MAP(Type) private: \
    AZStd::unordered_map<int, Ptr<Type>> m_device##Type##s; \
    public: \
    AZ_FORCE_INLINE Ptr<Type> GetDevice##Type(int deviceIndex) const \
    { \
        AZ_Error( \
            "MultiDevice" #Type, \
            m_device##Type##s.find(deviceIndex) != m_device##Type##s.end(), \
            "No Device" #Type " found for device index %d\n", \
            deviceIndex); \
        return m_device##Type##s.at(deviceIndex); \
    }

#define AZ_RHI_MULTI_DEVICE_OBJECT_MAP_PLURAL(Type, PluralType) private: \
    AZStd::unordered_map<int, Ptr<Type>> m_device##PluralType; \
    public: \
    AZ_FORCE_INLINE Ptr<Type> GetDevice##Type(int deviceIndex) const \
    { \
        AZ_Error( \
                  "MultiDevice" #Type, \
                                       m_device##PluralType.find(deviceIndex) != m_device##PluralType.end(), \
                  "No Device" #Type " found for device index %d\n", \
                  deviceIndex); \
        return m_device##PluralType.at(deviceIndex); \
    }


namespace AZ::RHI
{
    //! A variant of Object associated with a DeviceMask.
    //! In contrast to DeviceObject, which is device-specific and holds a strong reference to a specific device, 
    //! MultiDeviceObject only specifies on which device an object resides/operates, specified by a
    //! DeviceMask (1 bit per device).
    class MultiDeviceObject : public Object
    {
    public:
        AZ_RTTI(MultiDeviceObject, "{17D34F71-944C-4AF5-9823-627474C4C0A6}", Object);
        virtual ~MultiDeviceObject() = default;

        //! Returns whether the device object is initialized.
        bool IsInitialized() const;

        //! Returns the device this object is associated with. It is only permitted to call
        //! this method when the object is initialized.
        MultiDevice::DeviceMask GetDeviceMask() const;

    protected:
        MultiDeviceObject() = default;

        //! The derived class should call this method to assign the device.
        void Init(MultiDevice::DeviceMask deviceMask);

        //! Clears the current bound device to null.
        void Shutdown() override;

        //! Helper method that will iterate over all selected devices and call the provided callback
        template<typename T>
        void IterateDevices(T callback)
        {
            AZ_Error(
                "RPI::MultiDeviceObject", AZStd::to_underlying(m_deviceMask) != 0u, "Device mask is not initialized with a valid value.");

            int deviceCount = GetDeviceCount();

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if ((AZStd::to_underlying(m_deviceMask) >> deviceIndex) & 1)
                {
                    if (!callback(deviceIndex))
                    {
                        break;
                    }
                }
            }
        }

    private:
        //! Returns the number of initialized devices
        int GetDeviceCount() const;

        //! A bitmask denoting on which devices an object is present/valid/allocated
        MultiDevice::DeviceMask m_deviceMask{ 0u };
    };
} // namespace AZ::RHI
