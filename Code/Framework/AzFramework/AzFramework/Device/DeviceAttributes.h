/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Device/DeviceAttributeInterface.h>

namespace AzFramework
{
    class DeviceAttributeDeviceModel : public DeviceAttribute
    {
    public:
        DeviceAttributeDeviceModel();
        AZStd::string_view GetName() const override { return m_name; }
        AZStd::string_view GetDescription() const override { return m_description; }
        AZStd::any GetValue() const override { return AZStd::any(m_value); }
        bool Evaluate(AZStd::string_view rule) const override;
    protected:
        AZStd::string m_name = "DeviceModel";
        AZStd::string m_description = "Device Model";
        AZStd::string m_value;
    };

    class DeviceAttributeRAM : public DeviceAttribute
    {
    public:
        DeviceAttributeRAM();
        AZStd::string_view GetName() const override { return m_name; }
        AZStd::string_view GetDescription() const override { return m_description; }
        AZStd::any GetValue() const override { return AZStd::any(m_value); }
        bool Evaluate(AZStd::string_view rule) const override;
    protected:
        AZStd::string m_name = "RAM";
        AZStd::string m_description = "RAM available to OS in GB, usually less than the physically installed RAM.";
        float m_value;
    };
} // namespace AzFramework
