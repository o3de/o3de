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

#if !defined(AZ_RELEASE_BUILD)
#include <ThermalInfo.h>

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
