/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributeDeviceModel.h>

namespace AzFramework
{
    // Platform specific device model value retrieval can be found in the relevant platform's code folder
    // e.g. Code/Framework/AzFramework/Platform/Windows/AzFramework/Device/DeviceAttributesCommon_Win.cpp

    AZStd::string_view DeviceAttributeDeviceModel::GetName() const
    {
        return m_name;
    }

    AZStd::string_view DeviceAttributeDeviceModel::GetDescription() const
    {
        return m_description;
    }

    AZStd::any DeviceAttributeDeviceModel::GetValue() const
    {
        return AZStd::any(m_value);
    }

    bool DeviceAttributeDeviceModel::Evaluate([[maybe_unused]] AZStd::string_view rule) const
    {
        return false;
    }
} // namespace AzFramework

