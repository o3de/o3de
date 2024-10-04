/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DeviceObject.h>

// Predefinition for unit test friend class
namespace UnitTest
{
    struct MultiDeviceDrawPacketData;
}

#define AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Type) AZ_FORCE_INLINE Ptr<Device##Type> GetDevice##Type(int deviceIndex) const \
{ \
    return GetDeviceObject<Device##Type>(deviceIndex); \
}

namespace AZ::RHI
{
    //! A variant of Object associated with a DeviceMask.
    //! In contrast to DeviceObject, which is device-specific and holds a strong reference to a specific device,
    //! MultiDeviceObject only specifies on which device an object resides/operates, specified by a
    //! DeviceMask (1 bit per device).
    class MultiDeviceObject : public Object
    {
        friend struct UnitTest::MultiDeviceDrawPacketData;

    public:
        AZ_RTTI(MultiDeviceObject, "{17D34F71-944C-4AF5-9823-627474C4C0A6}", Object);
        virtual ~MultiDeviceObject() = default;

        //! Returns whether the device object is initialized.
        bool IsInitialized() const;

        //! Helper method to check if a device index is set in the device mask.
        //! Note: A bit may be set but a device object may not actually exist,
        //! e.g. if the device index is bigger than the device count.
        AZ_FORCE_INLINE bool IsDeviceSet(int deviceIndex) const
        {
            return CheckBit(m_deviceMask, deviceIndex);
        }

        //! Helper method that will iterate over all available devices in a device mask and call the provided callback
        //! The callback is called with the device index and needs to return a boolean indicating whether to continue
        //! th iteration.
        template<typename T>
        static AZ_FORCE_INLINE void IterateDevices(MultiDevice::DeviceMask deviceMask, T callback)
        {
            AZ_Error(
                "RPI::MultiDeviceObject::IterateDevices",
                AZStd::to_underlying(deviceMask) != 0u,
                "Device mask is not initialized with a valid value.");

            int deviceCount = GetDeviceCount();

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (CheckBit(deviceMask, deviceIndex))
                {
                    if (!callback(deviceIndex))
                    {
                        break;
                    }
                }
            }
        }

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
        AZ_FORCE_INLINE void IterateDevices(T callback)
        {
            IterateDevices(m_deviceMask, callback);
        }

        //! Helper method that will iterate over all device objects and call the provided callback with a
        //! device index and the object expecting ResultCode::Success in order to continue the iteration
        template<typename T, typename U>
        AZ_FORCE_INLINE decltype(auto) IterateObjects(U callback)
        {
            if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, ResultCode>)
            {
                auto resultCode{ ResultCode::Success };

                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    if ((resultCode = callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject))) != ResultCode::Success)
                    {
                        break;
                    }
                }

                return resultCode;
            }
            else if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, bool>)
            {
                auto resultCode{ true };

                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    if ((resultCode = callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject))) != true)
                    {
                        break;
                    }
                }

                return resultCode;
            }
            else if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, void>)
            {
                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject));
                }
            }
            else
            {
                AZ_Error(
                    "MultiDeviceObject",
                    false,
                    "Return type of callback not supported\n");
            }
        }

        template<typename T, typename U>
        AZ_FORCE_INLINE decltype(auto) IterateObjects(U callback) const
        {
            if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, ResultCode>)
            {
                auto resultCode{ ResultCode::Success };

                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    if ((resultCode = callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject))) != ResultCode::Success)
                    {
                        break;
                    }
                }

                return resultCode;
            }
            else if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, bool>)
            {
                auto resultCode{ true };

                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    if ((resultCode = callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject))) != true)
                    {
                        break;
                    }
                }

                return resultCode;
            }
            else if constexpr (AZStd::is_same_v<AZStd::invoke_result_t<U, int, Ptr<T>>, void>)
            {
                for (auto& [deviceIndex, deviceObject] : m_deviceObjects)
                {
                    callback(deviceIndex, AZStd::static_pointer_cast<T>(deviceObject));
                }
            }
            else
            {
                AZ_Error(
                    "MultiDeviceObject",
                    false,
                    "Return type of callback not supported\n");
            }
        }

        template<typename T>
        AZ_FORCE_INLINE Ptr<T> GetDeviceObject(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceObject",
                m_deviceObjects.find(deviceIndex) != m_deviceObjects.end(),
                "No DeviceObject found for device index %d\n",
                deviceIndex);

            return AZStd::static_pointer_cast<T>(m_deviceObjects.at(deviceIndex));
        }

        //! A map of all device-specific objects, indexed by the device index
        AZStd::unordered_map<int, Ptr<DeviceObject>> m_deviceObjects;

    private:
        //! Returns the number of initialized devices
        static int GetDeviceCount();

        //! Pass on name to DeviceObjects
        virtual void SetNameInternal(const AZStd::string_view& name) override;

        //! A bitmask denoting on which devices an object is present/valid/allocated
        MultiDevice::DeviceMask m_deviceMask{ 0u };
    };
} // namespace AZ::RHI
