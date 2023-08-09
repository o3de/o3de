/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributes.h>

namespace AzFramework
{
    bool DeviceAttributeDeviceModel::Evaluate([[maybe_unused]] AZStd::string_view rule) const
    {
        return false;
    }

    bool DeviceAttributeRAM::Evaluate([[maybe_unused]] AZStd::string_view rule) const
    {
        return false;
    }
} // namespace AzFramework

