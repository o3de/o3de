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
    //! Device attribute for getting device model name e.g. "Pixel 3 XL" 
    class DeviceAttributeDeviceModel : public DeviceAttribute
    {
    public:
        DeviceAttributeDeviceModel();
        AZStd::string_view GetName() const override;
        AZStd::string_view GetDescription() const override;
        AZStd::any GetValue() const override;
        bool Evaluate(AZStd::string_view rule) const override;
    protected:
        AZStd::string m_name = "DeviceModel";
        AZStd::string m_description = "Device Model";
        AZStd::string m_value = "";
    };
} // namespace AzFramework
