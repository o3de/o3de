/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A variant of Object associated with a Device instance. It holds a strong reference to
         * the device and provides a simple accessor API.
         */
        class DeviceObject
            : public Object
        {
        public:
            AZ_RTTI(DeviceObject, "{17D34F71-944C-4AF5-9823-627474C4C0A6}", Object);
            virtual ~DeviceObject() = default;

            /// Returns whether the device object is initialized.
            bool IsInitialized() const;

            /**
             * Returns the device this object is associated with. It is only permitted to call
             * this method when the object is initialized.
             */
            Device& GetDevice() const;

        protected:
            DeviceObject() = default;

            /// The derived class should call this method to assign the device.
            void Init(Device& device);

            /// Clears the current bound device to null.
            void Shutdown() override;

        private:
            Ptr<Device> m_device;
        };
    }
}