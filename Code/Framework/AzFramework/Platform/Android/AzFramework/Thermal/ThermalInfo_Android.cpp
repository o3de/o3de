/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_RELEASE_BUILD) 
#include "ThermalInfo_Android.h"
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <cstdio>
#include <sys/types.h>
#include <dirent.h>

ThermalInfoAndroidHandler::ThermalInfoAndroidHandler()
{
    static_assert(AZ_ARRAY_SIZE(m_temperatureFiles) == static_cast<int>(ThermalSensorType::Count), "Thermal count does not match temperature array size");
    ThermalInfoRequestsBus::Handler::BusConnect();
    memset(m_temperatureFiles, 0, sizeof(m_temperatureFiles));

    const int sensorCount = static_cast<int>(ThermalSensorType::Count);
    const char* sensorTypes[sensorCount] = { "cpu", "gpu", "battery" };
    const int maxStringLen = 128;
    char tempString[maxStringLen];
    const char* thermalPath = "/sys/class/thermal";
    // List the elements from the thermal folder to get the thermal_zones available on the device
    DIR* directory = opendir(thermalPath);
    if (directory)
    {
        struct dirent* item;
        const char* thermalPrefix = "thermal_zone";
        // List all items of the directory and find the one that start with thermal_zone (thermal_zone0, thermal_zone1, etc)
        while ((item = readdir(directory)) != nullptr)
        {
            if (strncmp(item->d_name, thermalPrefix, strlen(thermalPrefix)) == 0)
            {
                // Try to deduce the type of sensor. For this we read the "type" file of the thermal zone.
                // This "type" is a string set by the manufacturer, so it can be anything.
                AZStd::string path = AZStd::string::format("%s/%s/type", thermalPath, item->d_name);
                FILE* sensorTypeFile = fopen(path.c_str(), "r");
                if (sensorTypeFile)
                {
                    if (fscanf(sensorTypeFile, "%s", tempString))
                    {
                        for (int i = 0; i < sensorCount; ++i)
                        {
                            if (m_temperatureFiles[i])
                            {
                                continue;
                            }

                            size_t foundPos = AzFramework::StringFunc::Find(tempString, sensorTypes[i]);
                            if (foundPos != AZStd::string::npos)
                            {
                                path = AZStd::string::format("%s/%s/temp", thermalPath, item->d_name);
                                m_temperatureFiles[i] = fopen(path.c_str(), "r");
                                break;
                            }
                        }
                    }
                    fclose(sensorTypeFile);
                }
            }
        }
        closedir(directory);
    }

    int cpuSensorIndex = static_cast<int>(ThermalSensorType::CPU);
    if (!m_temperatureFiles[cpuSensorIndex])
    {
        // If we didn't find the CPU sensor just assume it's the first one.
        AZStd::string path = AZStd::string::format("%s/thermal_zone0/temp", thermalPath);
        m_temperatureFiles[cpuSensorIndex] = fopen(path.c_str(), "r");
    }
}

ThermalInfoAndroidHandler::~ThermalInfoAndroidHandler()
{
    ThermalInfoRequestsBus::Handler::BusDisconnect();
    for (int i = 0; i < static_cast<int>(ThermalSensorType::Count); ++i)
    {
        if (m_temperatureFiles[i])
        {
            fclose(m_temperatureFiles[i]);
        }
    }
}

float ThermalInfoAndroidHandler::GetSensorTemp(ThermalSensorType sensor)
{    
    FILE* tempFile = m_temperatureFiles[static_cast<int>(sensor)];
    if (!tempFile)
    {
        return 0.f;
    }

    fseek(tempFile, 0, SEEK_SET);
    float temperature = 0.f;
    fscanf(tempFile, "%f", &temperature);
    temperature /= 1000.0f;
    return temperature;
}

float ThermalInfoAndroidHandler::GetSensorOverheatingTemp(ThermalSensorType sensor)
{
    const int overheatingTemperatures[static_cast<int>(ThermalSensorType::Count)] = 
    {
        70, // CPU
        70, // GPU
        40  // Battery
    };

    return overheatingTemperatures[static_cast<int>(sensor)];
}

#endif // !defined(AZ_RELEASE_BUILD)

