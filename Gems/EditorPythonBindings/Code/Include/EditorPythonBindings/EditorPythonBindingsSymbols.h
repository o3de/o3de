/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

namespace EditorPythonBindings
{
    // components
    constexpr const char PythonMarshalComponentTypeId[] = "{C733E1AD-9FDD-484E-A8D9-3EAB944B7841}";
    constexpr const char PythonReflectionComponentTypeId[] = "{CBF32BE1-292C-4988-9E64-25127A8525A7}";
    constexpr const char PythonSystemComponentTypeId[] = "{97F88B0F-CF68-4623-9541-549E59EE5F0C}";

    // services
    constexpr AZ::Crc32 PythonMarshalingService = AZ_CRC_CE("PythonMarshalingService");
    constexpr AZ::Crc32 PythonReflectionService = AZ_CRC_CE("PythonReflectionService");
    constexpr AZ::Crc32 PythonEmbeddedService = AZ_CRC_CE("PythonEmbeddedService");
}
