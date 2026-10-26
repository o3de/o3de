/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Platforms.h"
#include "PlistDictionary.h"

#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/variant.h>

#include <AzFramework/StringFunc/StringFunc.h>


namespace ProjectSettingsTool
{

    struct SettingsError
    {
        SettingsError(const AZStd::string& error, const AZStd::string& reason, bool shouldAbort = false);

        // The error that occurred
        AZStd::string m_error;

        // The reason the error occurred
        AZStd::string m_reason;

        bool m_shouldAbort = false;
    };

    // Loads, saves, and provides access to all of the project settings files of all platforms.
    // It handles base settings and platform settings (android, ios) separately.
    // For base settings it uses json document and for platform settings it uses different documents
    // depending on the type, it supports json and plist formats.
    class ProjectSettingsContainer
    {
    public:
        using PlatformAndPath = AZStd::pair<PlatformId, AZStd::string>;
        using PlatformResources = AZStd::vector<PlatformAndPath>;

        template<class DocType>
        struct Settings
        {
            // File path to document
            AZStd::string m_path;

            // Raw string loaded from file
            AZStd::string m_rawData;

            // The document itself
            AZStd::unique_ptr<DocType> m_document;

        };

        using JsonSettings = Settings<rapidjson::Document>;
        using PlistSettings = Settings<XmlDocument>;
        using PlatformSettings = AZStd::variant<JsonSettings, PlistSettings>; // Platform data (Android, ios) can be either json or plist

        // Constructs the main manager of a document
        ProjectSettingsContainer(const AZStd::string& projectJsonFileName, const PlatformResources& platformResources);

        // Used to destroy valueDoesNotExist
        ~ProjectSettingsContainer();

        // Returns the PlatformSettings for given platform
        PlatformSettings* GetPlatformData(const Platform& plat);

        // Returns true if PlatformSettings are found for platform
        bool HasPlatformData(const Platform& plat) const;

        // Gets the earliest error not seen
        AZ::Outcome<void, SettingsError> GetError();

        // Save settings of platform data or project json data.
        void SaveSettings(const Platform& plat);

        // Saves project.json to disk
        void SaveProjectJsonData();

        // Reloads Project.json from disk
        void ReloadProjectJsonData();

        // Save all pLists back to disk
        void SaveAllPlatformsData();

        // Save platform's data back to disk
        void SavePlatformData(const Platform& plat);

        // Reloads all plists from disk
        void ReloadAllPlatformsData();

        // Returns a reference to the project.json Document
        rapidjson::Document& GetProjectJsonDocument();

        // Gets reference to value in project.json
        // returns null type if not found
        AZ::Outcome<rapidjson::Value*, void> GetProjectJsonValue(const char* key);

        AZStd::unique_ptr<PlistDictionary> CreatePlistDictionary(const Platform& plat);

        // Returns the allocator used by ProjectJson
        rapidjson::Document::AllocatorType& GetProjectJsonAllocator();

        static AZ::Outcome<rapidjson::Value*, void> GetJsonValue(rapidjson::Document& settings, const char* key);

    protected:
        void LoadJson(JsonSettings& jsonSettings);
        void SaveJson(const JsonSettings& jsonSettings);
        void LoadPlist(PlistSettings& plistSettings);
        void SavePlist(const PlistSettings& plistSettings);

        // Errors that have occurred
        AZStd::queue<SettingsError> m_errors;

        // The settings from project.json (base)
        JsonSettings m_projectJson;

        // The settings from platform resources (android, ios)
        AZStd::unordered_map<PlatformId, PlatformSettings> m_platformSettingsMap;

    private:
        AZ_DISABLE_COPY_MOVE(ProjectSettingsContainer);
    };
} // namespace ProjectSettingsTool
