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
#include <AzCore/std/string/string.h>

namespace Physics
{
    /// Constants for naming such as unit suffixes for physics properties.
    namespace NameConstants
    {
        const AZStd::string& GetSuperscriptMinus();
        const AZStd::string& GetSuperscriptOne();
        const AZStd::string& GetSuperscriptTwo();
        const AZStd::string& GetSuperscriptThree();
        const AZStd::string& GetInterpunct();
        const AZStd::string& GetSpeedUnit();
        const AZStd::string& GetAngularVelocityUnit();
        const AZStd::string& GetLengthUnit();
        const AZStd::string& GetVolumeUnit();
        const AZStd::string& GetMassUnit();
        const AZStd::string& GetInertiaUnit();
        const AZStd::string& GetSleepThresholdUnit();
        const AZStd::string& GetDensityUnit();
    } // namespace NameConstants
} // namespace Physics
