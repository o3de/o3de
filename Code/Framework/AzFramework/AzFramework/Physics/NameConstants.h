/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        const AZStd::string& GetBulletPoint();
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
