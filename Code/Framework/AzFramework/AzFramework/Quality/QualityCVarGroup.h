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

#include <AzFramework/Quality/QualitySystemBus.h>

namespace AzFramework
{
    // QualityCVarGroup wraps a QualityLevel CVAR for a quality group that, when set,
    // iterates over all settings in the group and performs console commands to
    // apply the settings for the requested quality level.
    // The general format of a quality group entry in the Settings Registry is:
    // {
    //     "O3DE" : {
    //         "Quality" : {
    //             "Groups" : {
    //                 "<group CVAR>" : {
    //                      "Description" : "<optional description>",
    //                      "Levels" : [ "<quality level 0>", "<quality level n>" ],
    //                      "Default" : "<default quality level>",
    //                      "Settings" : {
    //                          "<setting CVAR>" : [<level 0 value>, <level n value> ]
    //                      }
    //                  }
    //             }
    //         }
    //     }
    // }
    //    
    // QualityCVarGroup only creates a CVAR for the quality group itself, it does not
    // create CVARs for any entries in the quality groups "Settings" object.
    // Quality levels are assumed to be ordered low to high as in the example below.
    //
    // Example:
    // A q_shadows group CVAR exists with 4 levels (low=0,medium=1,high=2,veryhigh=3)
    // When the level of q_shadows is set to 2 LoadQualityLevel will apply all
    // console settings defined in the SettingsRegistry for that
    // group at level 2 (high).
    //
    class QualityCVarGroup
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        QualityCVarGroup(AZStd::string_view name, AZStd::string_view path);
        ~QualityCVarGroup();

        explicit inline operator QualityLevel() const { return m_qualityLevel; }

        //! required to set the value
        inline void CvarFunctor(const AZ::ConsoleCommandContainer& arguments);

        //! required to set the value
        bool StringToValue(const AZ::ConsoleCommandContainer& arguments);

        //! required to get the current value
        void ValueToString(AZ::CVarFixedString& outString) const;

        //! load the requested quality level CVar settings
        void LoadQualityLevel(QualityLevel qualityLevel);

        QualityLevel GetQualityLevel(const AZ::CVarFixedString& value) const;

    private:
        AZ_DISABLE_COPY(QualityCVarGroup);

        QualityLevel FromNumber(const AZ::CVarFixedString& value) const;
        QualityLevel FromEnum(const AZ::CVarFixedString& value) const;
        QualityLevel FromName(const AZ::CVarFixedString& value) const;

        AZ::PerformCommandResult PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key);

        inline static constexpr bool ReplicateCommand = true;
        using QualityCVarGroupFunctor = AZ::ConsoleFunctor<QualityCVarGroup, ReplicateCommand>;
        AZStd::string m_description;
        AZStd::string m_name;
        AZStd::unique_ptr<QualityCVarGroupFunctor> m_functor;
        AZStd::string m_path;
        int32_t m_numQualityLevels{ 0 };
        QualityLevel m_qualityLevel = QualityLevel::DefaultQualityLevel;
    };
} // namespace AzFramework

