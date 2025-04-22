/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/any.h>

namespace AzFramework
{
    // DeviceAttribute provides an interface for requesting information about a
    // single device attribute like model, GPU type, RAM amount etc.
    class DeviceAttribute
    {
    public:
        AZ_RTTI(DeviceAttribute, "{8B7AF778-DD8A-4AFA-8416-EA2D03080F29}");
        virtual ~DeviceAttribute() = default;

        //! Get the name of the device attribute e.g. gpuMemory, gpuVendor, customAttribute42
        virtual AZStd::string_view GetName() const = 0;

        //! Get a description of this device attribute, used for help text and eventual UI
        virtual AZStd::string_view GetDescription() const = 0;

        //! Evaluate a rule and return true if there is a match for this device attribute
        virtual bool Evaluate(AZStd::string_view rule) const = 0;

        //! Get the value of this attribute
        virtual AZStd::any GetValue() const = 0;
    };

    // DeviceAttributeRegistrarInterface provides an interface for registering and accessing
    // DeviceAttributes which describes a single device attribute.
    class DeviceAttributeRegistrarInterface
    {
    public:
        AZ_RTTI(DeviceAttributeRegistrarInterface, "{D6B65DF8-8275-42F8-B84D-4F9ACBECC7C2}");
        virtual ~DeviceAttributeRegistrarInterface() = default;

        //! Find a device attribute by name
        virtual DeviceAttribute* FindDeviceAttribute(AZStd::string_view deviceAttribute) const = 0;

        //! Register a device attribute interface, deviceAttribute must have a unique name, returns true on success
        virtual bool RegisterDeviceAttribute(AZStd::shared_ptr<DeviceAttribute> deviceAttributeInterface) = 0;

        //! Unregister an existing device attribute, returns true on success, false if not found
        virtual bool UnregisterDeviceAttribute(AZStd::string_view deviceAttribute) = 0;

        //! The callback function should return true to continue enumaration or false to stop
        using VisitInterfaceCallback = AZStd::function<bool(DeviceAttribute&)>;

        //! Visit device attribute interfaces with a callback function
        //! The visiting callback can be useful to enumerate over multiple attributes
        //! for display or rule evaluation.
        virtual void VisitDeviceAttributes(const VisitInterfaceCallback&) const = 0;
    };
    using DeviceAttributeRegistrar = AZ::Interface<DeviceAttributeRegistrarInterface>;

} // namespace AzFramework
