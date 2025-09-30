/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzFramework/AzFrameworkAPI.h>
#include <AzCore/std/string/string.h>

namespace Physics
{
    /// Constants for naming such as unit suffixes for physics properties.
    namespace NameConstants
    {
        AZF_API const AZStd::string& GetSuperscriptMinus();
        AZF_API const AZStd::string& GetSuperscriptOne();
        AZF_API const AZStd::string& GetSuperscriptTwo();
        AZF_API const AZStd::string& GetSuperscriptThree();
        AZF_API const AZStd::string& GetInterpunct();
        AZF_API const AZStd::string& GetBulletPoint();
        AZF_API const AZStd::string& GetSpeedUnit();
        AZF_API const AZStd::string& GetAngularVelocityUnit();
        AZF_API const AZStd::string& GetLengthUnit();
        AZF_API const AZStd::string& GetVolumeUnit();
        AZF_API const AZStd::string& GetMassUnit();
        AZF_API const AZStd::string& GetInertiaUnit();
        AZF_API const AZStd::string& GetSleepThresholdUnit();
        AZF_API const AZStd::string& GetDensityUnit();
    } // namespace NameConstants
} // namespace Physics
