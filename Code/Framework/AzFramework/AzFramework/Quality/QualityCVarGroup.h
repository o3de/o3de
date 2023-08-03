/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class SettingsRegistryInterface;
}

namespace AzFramework
{
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
        typedef AZ::s64 ValueType;

        QualityCVarGroup(AZ::SettingsRegistryInterface* registry, AZ::IConsole* console, AZStd::string_view name, AZStd::string_view path);
        ~QualityCVarGroup();

        inline operator ValueType() const { return m_qualityLevel; }

        //! required to set the value
        inline void CvarFunctor(const AZ::ConsoleCommandContainer& arguments);

        //! get the quality level from a name, or -1 if not found
        ValueType GetQualityLevelFromName(AZStd::string_view levelName);

        //! required to set the value
        bool StringToValue(const AZ::ConsoleCommandContainer& arguments);

        //! required to get the current value
        void ValueToString(AZ::CVarFixedString& outString) const;

        //! load the requested quality level CVar settings
        void LoadQualityLevel(ValueType qualityLevel);

    private:
        AZ_DISABLE_COPY(QualityCVarGroup);

        ValueType GetHighestQualityForSetting(AZStd::string_view path);
        void PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key);

        AZ::IConsole* m_console;
        using QualityCVarGroupFunctor = AZ::ConsoleFunctor<QualityCVarGroup, /*replicate=*/true>;
        AZStd::unique_ptr<QualityCVarGroupFunctor> m_functor;
        AZ::u16 m_numQualityLevels;
        AZStd::string m_path;
        ValueType m_qualityLevel;
        AZ::SettingsRegistryInterface* m_settingsRegistry;
    };
} // namespace AzFramework
