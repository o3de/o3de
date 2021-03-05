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

#include "Platforms.h"
#include "PlistDictionary.h"

#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/StringFunc/StringFunc.h>


namespace ProjectSettingsTool
{

    struct SettingsError
    {
        SettingsError(const AZStd::string& error, const AZStd::string& reason);

        // The error that occurred
        AZStd::string m_error;
        // The reason the error occurred
        AZStd::string m_reason;
    };

    // Loads, saves, and provides access to all of the project settings files
    class ProjectSettingsContainer
    {
    public:
        typedef AZStd::pair<PlatformId, AZStd::string> PlatformAndPath;
        typedef AZStd::vector<PlatformAndPath> PlistInitVector;

        template<class DocType>
        struct PlatformSettings
        {
            // File path to document
            AZStd::string m_path;
            // Raw string loaded from file
            AZStd::string m_rawData;
            // The document itself
            AZStd::unique_ptr<DocType> m_document;

        };

        using JsonSettings = PlatformSettings<rapidjson::Document>;
        using PlistSettings = PlatformSettings<AZ::rapidxml::xml_document<char>>;

        // Constructs the main manager of a document
        ProjectSettingsContainer(const AZStd::string& projectJsonFileName, PlistInitVector& plistPaths);

        // Used to destroy valueDoesNotExist
        ~ProjectSettingsContainer();

        // Returns the PlistSettings for given platform
        PlistSettings* GetPlistSettingsForPlatform(const Platform& plat);
        // Returns true if PlistSettings are found for platform
        bool IsPlistPlatform(const Platform& plat);

        // Gets the earliest error not seen
        AZ::Outcome<void, SettingsError> GetError();
        // Save settings for given platform
        void SavePlatformData(const Platform& plat);
        // Saves project.json to disk
        void SaveProjectJsonData();
        // Reloads Project.json from disk
        void ReloadProjectJsonData();
        // Save all pLists back to disk
        void SavePlistsData();
        // Save platform's plist data back to disk
        void SavePlistData(const Platform& plat);
        // Reloads all plists from disk
        void ReloadPlistData();
        // Returns a reference to the project.json Document
        rapidjson::Document& GetProjectJsonDocument();
        // Gets reference to value in project.json
        // returns null type if not found
        AZ::Outcome<rapidjson::Value*, void> GetProjectJsonValue(const char* key);

        AZStd::unique_ptr<PlistDictionary> GetPlistDictionary(const Platform& plat);

        // Returns the allocator used by ProjectJson
        rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& GetProjectJsonAllocator();

        static const char* GetFailedLoadingPlistText();


    protected:
        // Loads project.json from disk
        void LoadProjectJsonData();
        // Loads info.plist from filePath into given document
        void LoadPlist(PlistSettings& plistSettings);
        void SavePlist(PlistSettings& plistSettings);

        // Errors that have occurred
        AZStd::queue<SettingsError> m_errors;
        // The project.json document
        JsonSettings m_projectJson;
        // A map to all of the loaded pLists
        AZStd::unordered_map<PlatformId, PlistSettings> m_pListsMap;


    private:
        AZ_DISABLE_COPY_MOVE(ProjectSettingsContainer);
    };
} // namespace ProjectSettingsTool
