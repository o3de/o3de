/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    class QualitySystemEvents
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<QualitySystemEvents>;

        // When loading quality group settings using a level value of LevelFromDeviceRules
        // the device rules will be used to determine the quality level to use.
        static constexpr int LevelFromDeviceRules = -1;

        // Loads the default quality group level.
        // @param level Optional quality level.  If LevelFromDeviceRules, the level will be
        //              determined from existing device rules.
        virtual void LoadDefaultQualityGroup(int level = LevelFromDeviceRules) = 0;
    };
} //AzFramework
