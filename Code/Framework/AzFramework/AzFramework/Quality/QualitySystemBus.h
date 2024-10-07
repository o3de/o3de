/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    AZ_ENUM_CLASS(QualityLevel,
        (Invalid, -2),
        // When loading quality group settings using a level value of LevelFromDeviceRules
        // the device rules will be used to determine the quality level to use.
        // Quality levels are assumed to be ordered low to high with 0 being the lowest
        // quality.
        (LevelFromDeviceRules, -1),
        (DefaultQualityLevel, 0)
    );
    AZ_TYPE_INFO_SPECIALIZE(QualityLevel, "{9AABD1B2-D433-49FE-A89D-2BEF09A252C0}");
    AZ_DEFINE_ENUM_ARITHMETIC_OPERATORS(QualityLevel);
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(QualityLevel);

    // Device rule resolution 
    AZ_ENUM_CLASS(DeviceRuleResolution,
        (Invalid, -1),
        (Min, 0), // use the lowest setting for all matching rules
        (Max, 1), // use the maximum setting for all matching rules
        (First, 2), // use the setting for the first rule that matches
        (Last, 3)  // use the setting for the last rule that matches
    );
    AZ_TYPE_INFO_SPECIALIZE(DeviceRuleResolution, "{DB0E23FC-433A-4A95-BEFA-8E39E3A1E344}");
    AZ_DEFINE_ENUM_ARITHMETIC_OPERATORS(DeviceRuleResolution);
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(DeviceRuleResolution);

    class QualitySystemEvents
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<QualitySystemEvents>;

        // Loads the default quality group if one is defined in /O3DE/Quality/DefaultGroup
        // If the requested QualityLevel is 0 or higher, settings for that level will be loaded.
        // If the requested QualityLevel is LevelFromDeviceRules, device rules will be used to
        // determine the quality level, and if no device rule match is found the default
        // level for that group will be used if it has one.
        //
        // @param level Optional quality level to load. If LevelFromDeviceRules, the level will be
        //              determined from device rules. If there is no matching device rule, the
        //              default level for the group will be used.
        virtual void LoadDefaultQualityGroup(QualityLevel level = QualityLevel::LevelFromDeviceRules) = 0;
    };
} //AzFramework
