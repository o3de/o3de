/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>
#include <AzCore/std/optional.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/DOM/DomPrefixTree.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

namespace AZ
{
    class SettingsRegistryOriginTracker
    {
    public:
        AZ_TYPE_INFO(SettingsRegistryOriginTracker, "{F6E3B4E5-F8CF-43D8-8609-BC322D9D08C5}");

        explicit SettingsRegistryOriginTracker(AZ::SettingsRegistryInterface& registry);
        ~SettingsRegistryOriginTracker();

        //! TrackingFilterCallback will be invoked with the JSON settings key being updated
        //! It should return true if the key at that JSON path should be tracked
        using TrackingFilterCallback = AZStd::function<bool(AZStd::string_view keyPath)>;

        //! Sets functions which will be used to filter which settings should be tracked
        //! If empty, all settings within the associated registry will be tracked
        void SetTrackingFilter(TrackingFilterCallback filterCallback);

        //! Gets setting registry being tracked
        AZ::SettingsRegistryInterface& GetSettingsRegistry();
        const AZ::SettingsRegistryInterface& GetSettingsRegistry() const;

        //! Returns the last origin associated with the given key
        bool FindLastOrigin(AZ::IO::Path& originPath, AZStd::string_view key) const;

        struct SettingsRegistryOrigin
        {
            //! Path to the file where the settings is sourced from or a special value in angle brackets to indicate a non-file source(<C++>, <command-line>, etc...)
            AZ::IO::Path m_originFilePath;
            //! The value of the settings key associated with the origin
            AZStd::string m_settingsKey;

            //! Sutable for representing a setting as a JSON string or to be not engaged if the setting has been deleted.
            using ValueType = AZStd::optional<AZStd::string>;

            //! The value of the setting as a JSON string at the time the origin entry was created
            //! If the value is a nullopt, then the setting was removed
            ValueType m_settingsValue;

            bool operator==(const SettingsRegistryOrigin& origin) const;
        };

        using SettingsRegistryOriginStack = AZStd::deque<SettingsRegistryOrigin>;

        //! Origin Visitor is callback is invoked for each setting key found during the VisitOrigins method
        //! @param settingsRegistryOrigin entry being visited
        //! @returns True should be returned if the visitation should continue.
        using OriginVisitorCallback = AZStd::function<bool(const SettingsRegistryOrigin&)>;

        //! Visit all settings keys that are at specified key path and any children in the DomPrefixTree
        //! and invoke the supplied callback with the visit key and the SettingsRegistryOriginStack
        void VisitOrigins(AZStd::string_view key, const OriginVisitorCallback& visitCallback) const;

    private:

        struct SettingsNotificationHandler
        {
            SettingsNotificationHandler(SettingsRegistryOriginTracker& originTracker);
            ~SettingsNotificationHandler();

            void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs);

        private:
            SettingsRegistryOriginTracker& m_originTracker;
            AZ::JsonSerializationResult::JsonIssueCallback m_prevReportingCallback;
        };

        using SettingsRegistryOriginPrefixTree = AZ::Dom::DomPrefixTree<SettingsRegistryOriginStack>;

        //! Afterwards push a new entry into the Origin Stack PrefixTree for the origin path
        void AddOrigin(AZStd::string_view key, AZ::IO::PathView originPath);
        //! Looks up the settings key in the Origin Stack Prefix Tree dict and removes an entry with the origin path
        //! If after this opration the Origin Stack is empty, then prefix tree entry is removed
        //! @param recurseChildren, If true children entries from the key in the Prefix Tree will also be visited.
        void RemoveOrigin(AZStd::string_view key, AZ::IO::PathView originPath, bool recurseChildren = false);


    private: // member variables
        AZ::SettingsRegistryInterface& m_settingsRegistry;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_notifyHandler;

        //! Maps a DOM path for the setting to a stack of entries which contain the source file where the setting originates from and a patch that recreate the value within that file
        SettingsRegistryOriginPrefixTree m_settingsOriginPrefixTree;
        //! Stack containing vectors of keys being added or modified while a file is being merged
        TrackingFilterCallback m_trackingFilter;
    };
}
