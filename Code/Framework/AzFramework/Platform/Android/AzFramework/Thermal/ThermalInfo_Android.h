/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(AZ_RELEASE_BUILD)
#include <AzFramework/Thermal/ThermalInfo.h>

class ThermalInfoAndroidHandler : public ThermalInfoRequestsBus::Handler
{
public:
    ThermalInfoAndroidHandler();
    ~ThermalInfoAndroidHandler() override;

    float GetSensorTemp(ThermalSensorType sensor) override;
    float GetSensorOverheatingTemp(ThermalSensorType sensor) override;

private:
    FILE* m_temperatureFiles[static_cast<int>(ThermalSensorType::Count)];
};
#endif // !defined(AZ_RELEASE_BUILD)
