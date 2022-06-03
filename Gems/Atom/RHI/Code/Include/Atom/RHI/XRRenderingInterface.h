/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Interface/Interface.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ::RHI
{
    //! Base instance descriptor class used to communicate with base XR module
    class XRInstanceDescriptor : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(XRInstanceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRInstanceDescriptor, "{FE1EC82F-6265-4A67-84D2-D05D4229B598}");

        XRInstanceDescriptor() = default;
        virtual ~XRInstanceDescriptor() = default;
    };

    //! Base physical device descriptor class used to communicate with base XR module
    class XRPhysicalDeviceDescriptor : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(XRInstanceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRPhysicalDeviceDescriptor, "{94B9A6A2-AA80-4439-A51F-DBF20B4337BD}");

        XRPhysicalDeviceDescriptor() = default;
        virtual ~XRPhysicalDeviceDescriptor() = default;
    };

    //! Base device descriptor class used to communicate with base XR module
    class XRDeviceDescriptor : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(XRDeviceDescriptor, AZ::SystemAllocator, 0);
        AZ_RTTI(XRDeviceDescriptor, "{02118DCD-A081-4B1C-80CA-A8C5CD80D83B}");

        XRDeviceDescriptor() = default;
        virtual ~XRDeviceDescriptor() = default;
    };
       
    //! The class defines the XR specific RHI rendering interface. 
    class XRRenderingInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(XRRenderingInterface, AZ::SystemAllocator, 0);
        AZ_RTTI(XRRenderingInterface, "{D1D99CEF-30E5-4690-9D91-36C0029436FD}");

        XRRenderingInterface() = default;
        virtual ~XRRenderingInterface() = default;

        //! Rendering api to create a native instance
        virtual AZ::RHI::ResultCode InitNativeInstance(AZ::RHI::XRInstanceDescriptor* instanceDescriptor) = 0;

        //! Rendering api to get the number of physical devices
        virtual AZ::u32 GetNumPhysicalDevices() = 0;

        //! Rendering api to get the physical devices associated with a specific index
        virtual AZ::RHI::ResultCode GetXRPhysicalDevice(
            AZ::RHI::XRPhysicalDeviceDescriptor* physicalDeviceDescriptor, int32_t index) = 0;

        //! Rendering api to create a XR specific native object
        virtual AZ::RHI::ResultCode CreateDevice(AZ::RHI::XRDeviceDescriptor* instanceDescriptor) = 0;
    };
}
