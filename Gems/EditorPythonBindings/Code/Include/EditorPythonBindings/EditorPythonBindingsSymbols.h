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
