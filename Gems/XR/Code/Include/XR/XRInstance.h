/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <XR/XRBase.h>
#include <XR/XRObject.h>

namespace XR
{
    class Instance
        : public XR::Object
    {
    public:
        AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator);
        AZ_RTTI(Instance, "{1C457924-56A4-444F-BC72-4D31A097BA70}");

        Instance() = default;
        ~Instance() override = default;

        //! Init the back-end instance. It is responsible for figuring out supported layers and extensions
        //! and based on that a xr instance is created. It also has logging support based on validation mode.
        AZ::RHI::ResultCode Init(AZ::RHI::ValidationMode validationMode);

        //! Get number of physical devices for XR.
        virtual AZ::u32 GetNumPhysicalDevices() const = 0;

        //! API to retrieve the native physical device for a specific index.
        virtual AZ::RHI::ResultCode GetXRPhysicalDevice(AZ::RHI::XRPhysicalDeviceDescriptor* physicalDeviceDescriptor, int32_t index) = 0;

    protected:
        //! API to allow backend object to initialize native xr instance.
        virtual AZ::RHI::ResultCode InitInstanceInternal() = 0;

        //! Called when the XR instance is being shutdown.
        virtual void ShutdownInternal() = 0;

        //Cache validation mode in case the backend object needs to use it.
        AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;

    private:
        ///////////////////////////////////////////////////////////////////
        // XR::Object
        void Shutdown() override;
        ///////////////////////////////////////////////////////////////////
    };

} // namespace XR
