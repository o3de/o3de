/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A variant of Object associated with a DeviceMask. It provides a simple accessor API.
         */
        class MultiDeviceObject : public Object
        {
        public:
            AZ_RTTI(MultiDeviceObject, "{17D34F71-944C-4AF5-9823-627474C4C0A6}", Object);
            virtual ~MultiDeviceObject() = default;

            /// Returns whether the device object is initialized.
            bool IsInitialized() const;

            /**
             * Returns the device this object is associated with. It is only permitted to call
             * this method when the object is initialized.
             */
            DeviceMask GetDeviceMask() const;

        protected:
            MultiDeviceObject() = default;

            /// The derived class should call this method to assign the device.
            void Init(DeviceMask deviceMask);

            /// Clears the current bound device to null.
            void Shutdown() override;

            template<typename T>
            void IterateDevices(T callback)
            {
                AZ_Error("RPI::MultiDeviceObject", m_deviceMask != 0, "Device mask is not initialized with a valid value.");

                int deviceCount = GetDeviceCount();

                for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
                {
                    if ((m_deviceMask >> deviceIndex) & 1)
                    {
                        if (!callback(deviceIndex))
                            break;
                    }
                }
            }

        private:
            int GetDeviceCount() const;

            DeviceMask m_deviceMask = 0;
        };
    }
}
