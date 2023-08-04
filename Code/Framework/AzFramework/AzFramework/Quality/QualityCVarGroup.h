/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Preprocessor/Enum.h> 

namespace AzFramework
{
    AZ_ENUM_CLASS(QualityLevels,
        (LevelFromDeviceRules, -1),
        (DefaultQualityLevel, 0)
    );
    AZ_DEFINE_ENUM_ARITHMETIC_OPERATORS(QualityLevels);
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(QualityLevels);

    // QualityCVarGroup wraps an integer CVar for a quality group that, when set,
    // notifies the QualitySystemBus to load all the settings for the group and the
    // requested level
    //
    // Example:
    // A q_shadows group CVar could be created with 4 levels (low=0,medium=1,high=2,veryhigh=3)
    // When the level of q_shadows is set to 2 the QualitySystem will be notified
    // and apply all console settings defined in the SettingsRegistry for that
    // group at level 2 (high).
    //
    class QualityCVarGroup
    {
    public:
        using ValueType = QualityLevels;

        QualityCVarGroup(AZStd::string_view name, AZStd::string_view path);
        ~QualityCVarGroup();

        explicit inline operator ValueType() const { return m_qualityLevel; }

        //! required to set the value
        inline void CvarFunctor(const AZ::ConsoleCommandContainer& arguments);

        //! required to set the value
        bool StringToValue(const AZ::ConsoleCommandContainer& arguments);

        //! required to get the current value
        void ValueToString(AZ::CVarFixedString& outString) const;

        //! load the requested quality level CVar settings
        void LoadQualityLevel(ValueType qualityLevel);

    private:
        AZ_DISABLE_COPY(QualityCVarGroup);

        AZ::PerformCommandResult PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key);

        inline static constexpr bool ReplicateCommand = true;
        using QualityCVarGroupFunctor = AZ::ConsoleFunctor<QualityCVarGroup, ReplicateCommand>;
        AZStd::unique_ptr<QualityCVarGroupFunctor> m_functor;
        AZStd::string m_name;
        AZStd::string m_path;
        ValueType m_qualityLevel = QualityLevels::DefaultQualityLevel;
    };
} // namespace AzFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::QualityLevels, "{9AABD1B2-D433-49FE-A89D-2BEF09A252C0}");
}
