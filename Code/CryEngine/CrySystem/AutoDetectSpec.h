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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSYSTEM_AUTODETECTSPEC_H
#define CRYINCLUDE_CRYSYSTEM_AUTODETECTSPEC_H
#pragma once



#if defined(WIN32) || defined(WIN64)

// exposed AutoDetectSpec() helper functions for reuse in CrySystem
namespace Win32SysInspect
{
    enum DXFeatureLevel
    {
        DXFL_Undefined,
        DXFL_9_1,
        DXFL_9_2,
        DXFL_9_3,
        DXFL_10_0,
        DXFL_10_1,
        DXFL_11_0
    };

    const char* GetFeatureLevelAsString(DXFeatureLevel featureLevel);

    void GetNumCPUCores(unsigned int& totAvailToSystem, unsigned int& totAvailToProcess);
    bool IsDX11Supported();
    bool GetGPUInfo(char* pName, size_t bufferSize, unsigned int& vendorID, unsigned int& deviceID, unsigned int& totLocalVidMem, DXFeatureLevel& featureLevel);
    int GetGPURating(unsigned int vendorId, unsigned int deviceId);
    void GetOS(SPlatformInfo::EWinVersion& ver, bool& is64Bit, char* pName, size_t bufferSize);
    bool IsVistaKB940105Required();

    inline size_t SafeMemoryThreshold(size_t memMB)
    {
        return (memMB * 8) / 10;
    }
}

#endif // #if defined(WIN32) || defined(WIN64)


#endif // CRYINCLUDE_CRYSYSTEM_AUTODETECTSPEC_H
