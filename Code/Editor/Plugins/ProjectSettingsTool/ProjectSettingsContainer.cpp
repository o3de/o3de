/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsContainer.h"

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/XML/rapidxml_print.h>

#include <CryCommon/platform.h>
#include <Util/FileUtil.h>

namespace ProjectSettingsTool
{
    SettingsError::SettingsError(const AZStd::string& error, const AZStd::string& reason, bool shouldAbort)
        : m_error(error)
        , m_reason(reason)
        , m_shouldAbort(shouldAbort)
    {
    }

    AZ::Outcome<void, AZStd::string> WriteConfigFile(const AZStd::string& fileName, const AZStd::string& fileContents)
    {
        using AZ::IO::SystemFile;
        const char* filePath = fileName.c_str();

        // Attempt to make file writable or check it out in source control
        if (CFileUtil::OverwriteFile(filePath))
        {
            AZStd::string dir = filePath;
            AZ::StringFunc::Path::StripFullName(dir);
            if (!CFileUtil::CreateDirectory(dir.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Could not create the directory for file \"%s\".", dir.c_str()));
            }

            SystemFile settingsFile;
            if (!settingsFile.Open(filePath, SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE))
            {
                return AZ::Failure(AZStd::string::format("Failed to open settings file %s for write.", filePath));
            }

            if (settingsFile.Write(fileContents.c_str(), fileContents.size()) != fileContents.size())
            {
                return AZ::Failure(AZStd::string::format("Failed to write to file %s.", filePath));
            }


            settingsFile.Close();
            return AZ::Success();
        }

        return AZ::Failure(AZStd::string::format("Could not check out or make file writable: \"%s\".", filePath));
    }

    AZ::Outcome<void, AZStd::string> ReadConfigFile(const AZStd::string& fileName, AZStd::string& fileContents)
    {
        using AZ::IO::SystemFile;
        const char* filePath = fileName.c_str();

        if (!SystemFile::Exists(filePath))
        {
            return AZ::Failure(AZStd::string::format("%s file doesn't exist.", filePath));
        }

        SystemFile settingsFile;
        if (!settingsFile.Open(filePath, SystemFile::SF_OPEN_READ_ONLY))
        {
            return AZ::Failure(AZStd::string::format("Failed to open settings file %s.", filePath));
        }

        fileContents = AZStd::string(settingsFile.Length(), '\0');
        settingsFile.Read(fileContents.size(), fileContents.data());

        settingsFile.Close();
        return AZ::Success();
    }


    ProjectSettingsContainer::ProjectSettingsContainer(const AZStd::string& projectJsonFileName, const PlatformResources& platformResources)
        : m_projectJson(JsonSettings{ projectJsonFileName, "", AZStd::make_unique<rapidjson::Document>() })
    {
        LoadJson(m_projectJson);

        for (const auto& [platformId, path] : platformResources)
        {
            if (AZ::IO::PathView(path).Extension() == ".json")
            {
                m_platformSettingsMap.insert(AZStd::pair<PlatformId, PlatformSettings>(
                    platformId,
                    JsonSettings{ path, "", AZStd::make_unique<rapidjson::Document>() }));

                LoadJson(AZStd::get<JsonSettings>(m_platformSettingsMap[platformId]));
            }
            else
            {
                m_platformSettingsMap.insert(AZStd::pair<PlatformId, PlatformSettings>(
                    platformId,
                    PlistSettings{ path, "", AZStd::make_unique<XmlDocument>() }));

                LoadPlist(AZStd::get<PlistSettings>(m_platformSettingsMap[platformId]));
            }
        }
    }

    ProjectSettingsContainer::~ProjectSettingsContainer()
    {
    }

    ProjectSettingsContainer::PlatformSettings* ProjectSettingsContainer::GetPlatformData(const Platform& plat)
    {
        PlatformSettings* result = nullptr;

        if (plat.m_type == PlatformDataType::PlatformResource)
        {
            auto iter = m_platformSettingsMap.find(plat.m_id);

            if (iter != m_platformSettingsMap.end())
            {
                result = &iter->second;
            }
            else
            {
                AZ_Assert(false, "Failed to find pList for platform.");
            }
        }

        return result;
    }

    bool ProjectSettingsContainer::HasPlatformData(const Platform& plat) const
    {
        if (plat.m_type == PlatformDataType::PlatformResource)
        {
            auto iter = m_platformSettingsMap.find(plat.m_id);

            if (iter != m_platformSettingsMap.end())
            {
                return true;
            }
        }
        return false;
    }

    AZ::Outcome<void, SettingsError> ProjectSettingsContainer::GetError()
    {
        if (!m_errors.empty())
        {
            SettingsError error = m_errors.front();
            m_errors.pop();

            return AZ::Failure(error);
        }
        return AZ::Success();
    }

    void ProjectSettingsContainer::SaveSettings(const Platform& plat)
    {
        const PlatformSettings* platformSettings = GetPlatformData(plat);
        if (platformSettings != nullptr)
        {
            if (AZStd::holds_alternative<JsonSettings>(*platformSettings))
            {
                SaveJson(AZStd::get<JsonSettings>(*platformSettings));
            }
            else
            {
                SavePlist(AZStd::get<PlistSettings>(*platformSettings));
            }
        }
        else
        {
            SaveJson(m_projectJson);
        }
    }

    void ProjectSettingsContainer::SaveProjectJsonData()
    {
        SaveJson(m_projectJson);
    }

    void ProjectSettingsContainer::ReloadProjectJsonData()
    {
        m_projectJson.m_document.reset(new rapidjson::Document);
        LoadJson(m_projectJson);
    }

    void ProjectSettingsContainer::SaveAllPlatformsData()
    {
        for (const auto& platformData : m_platformSettingsMap)
        {
            const PlatformSettings& platformSettings = platformData.second;

            if (AZStd::holds_alternative<JsonSettings>(platformSettings))
            {
                SaveJson(AZStd::get<JsonSettings>(platformSettings));
            }
            else
            {
                SavePlist(AZStd::get<PlistSettings>(platformSettings));
            }
        }
    }

    void ProjectSettingsContainer::SavePlatformData(const Platform& plat)
    {
        const PlatformSettings* platformSettings = GetPlatformData(plat);
        if (platformSettings != nullptr)
        {
            if (AZStd::holds_alternative<JsonSettings>(*platformSettings))
            {
                SaveJson(AZStd::get<JsonSettings>(*platformSettings));
            }
            else
            {
                SavePlist(AZStd::get<PlistSettings>(*platformSettings));
            }
        }
    }

    void ProjectSettingsContainer::ReloadAllPlatformsData()
    {
        for (AZStd::pair<PlatformId, PlatformSettings>& platformData : m_platformSettingsMap)
        {
            PlatformSettings& platformSettings = platformData.second;

            if (AZStd::holds_alternative<JsonSettings>(platformSettings))
            {
                auto& jsonSettings = AZStd::get<JsonSettings>(platformSettings);
                jsonSettings.m_document.reset(new rapidjson::Document);
                LoadJson(jsonSettings);
            }
            else
            {
                auto& plistSettings = AZStd::get<PlistSettings>(platformSettings);
                plistSettings.m_document.reset(new XmlDocument());
                LoadPlist(plistSettings);
            }
        }
    }

    rapidjson::Document& ProjectSettingsContainer::GetProjectJsonDocument()
    {
        return *m_projectJson.m_document;
    }

    AZ::Outcome<rapidjson::Value*, void> ProjectSettingsContainer::GetProjectJsonValue(const char* key)
    {
        return GetJsonValue(*m_projectJson.m_document, key);
    }

    AZ::Outcome<rapidjson::Value*, void> ProjectSettingsContainer::GetJsonValue(rapidjson::Document& settings, const char* key)
    {
        // Try to find member
        rapidjson::Value::MemberIterator memberIterator = settings.FindMember(key);
        if (memberIterator != settings.MemberEnd())
        {
            return AZ::Success(&memberIterator->value);
        }
        else
        {
            settings.AddMember(rapidjson::Value(key, settings.GetAllocator()),
                rapidjson::Value(rapidjson::kNullType), settings.GetAllocator());
            return AZ::Success(&settings.FindMember(key)->value);
        }
    }

    AZStd::unique_ptr<PlistDictionary> ProjectSettingsContainer::CreatePlistDictionary(const Platform& plat)
    {
        if (plat.m_type == PlatformDataType::PlatformResource)
        {
            const PlatformSettings* platformSettings = GetPlatformData(plat);
            if (platformSettings != nullptr)
            {
                if (!AZStd::holds_alternative<PlistSettings>(*platformSettings))
                {
                    AZ_Warning("ProjectSettingsContainer", false, "PlistDictionary can only be created from plist settings.");
                    return nullptr;
                }

                const auto& plistSettings = AZStd::get<PlistSettings>(*platformSettings);
                if (PlistDictionary::ContainsValidDict(plistSettings.m_document.get()))
                {
                    return AZStd::make_unique<PlistDictionary>(plistSettings.m_document.get());
                }
                else
                {
                    AZ_Error("ProjectSettingsContainer", false, "File %s contains an invalid PlistDictionary.", plistSettings.m_path.c_str());
                    return nullptr;
                }
            }
        }
        
        return nullptr;
    }

    rapidjson::Document::AllocatorType& ProjectSettingsContainer::GetProjectJsonAllocator()
    {
        return m_projectJson.m_document->GetAllocator();
    }

    void ProjectSettingsContainer::LoadJson(JsonSettings& jsonSettings)
    {
        AZ::Outcome<void, AZStd::string> outcome = ReadConfigFile(jsonSettings.m_path, jsonSettings.m_rawData);
        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError(AZStd::string::format("Failed to load %s", jsonSettings.m_path.c_str()), outcome.GetError(), true /*shouldAbort*/));
        }

        jsonSettings.m_document->Parse(jsonSettings.m_rawData.c_str());
    }

    void ProjectSettingsContainer::SaveJson(const JsonSettings& jsonSettings)
    {
        // Needed to write a document out to a string
        rapidjson::StringBuffer jsonDataBuffer;
        // Use pretty writer so it can be read easier
        rapidjson::PrettyWriter<rapidjson::StringBuffer> jsonDatawriter(jsonDataBuffer);

        jsonSettings.m_document->Accept(jsonDatawriter);
        const AZStd::string jsonDataString = jsonDataBuffer.GetString();

        AZ::Outcome<void, AZStd::string> outcome = WriteConfigFile(jsonSettings.m_path, jsonDataString);

        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError(AZStd::string::format("Failed to save %s", jsonSettings.m_path.c_str()), outcome.GetError()));
        }
    }

    void ProjectSettingsContainer::LoadPlist(PlistSettings& plistSettings)
    {
        AZ::Outcome<void, AZStd::string> outcome = ReadConfigFile(plistSettings.m_path, plistSettings.m_rawData);
        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError(AZStd::string::format("Failed to load %s", plistSettings.m_path.c_str()), outcome.GetError(), true /*shouldAbort*/));
        }

        const int xmlFlags = AZ::rapidxml::parse_doctype_node | AZ::rapidxml::parse_declaration_node | AZ::rapidxml::parse_no_data_nodes;

        plistSettings.m_document->parse<xmlFlags>(plistSettings.m_rawData.data());
    }

    void ProjectSettingsContainer::SavePlist(const PlistSettings& plistSettings)
    {
        // Needed to write a document out to a string
        AZStd::string xmlDocString;
        AZ::rapidxml::print(std::back_inserter(xmlDocString), *plistSettings.m_document);

        AZ::Outcome<void, AZStd::string> outcome = WriteConfigFile(plistSettings.m_path, xmlDocString);

        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError(AZStd::string::format("Failed to save %s", plistSettings.m_path.c_str()), outcome.GetError()));
        }
    }
} // namespace ProjectSettingsTool
