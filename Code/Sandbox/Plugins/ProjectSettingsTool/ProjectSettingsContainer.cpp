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

#include "ProjectSettingsTool_precompiled.h"
#include "ProjectSettingsContainer.h"

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/XML/rapidxml_print.h>

#include <Util/FileUtil.h>

namespace ProjectSettingsTool
{
    using namespace AZ;
    using StringOutcome = Outcome<void, AZStd::string>;
    using XmlDocument = rapidxml::xml_document<char>;
    using XmlNode = rapidxml::xml_node<char>;

    const int xmlFlags = rapidxml::parse_doctype_node | rapidxml::parse_declaration_node | rapidxml::parse_no_data_nodes;

    SettingsError::SettingsError(const AZStd::string& error, const AZStd::string& reason)
    {
        m_error = error;
        m_reason = reason;
    }

    StringOutcome WriteConfigFile(const AZStd::string& fileName, const AZStd::string& fileContents)
    {
        using AZ::IO::SystemFile;
        const char* filePath = fileName.c_str();

        // Attempt to make file writable or check it out in source control
        if (CFileUtil::OverwriteFile(filePath))
        {
            if (!CFileUtil::CreateDirectory(fileName.substr(0, fileName.find_last_of('/')).data()))
            {
                return AZ::Failure(AZStd::string::format("Could not create the directory for file \"%s\".", filePath));
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

    StringOutcome ReadConfigFile(const AZStd::string& fileName, AZStd::string& fileContents)
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


    ProjectSettingsContainer::ProjectSettingsContainer(const AZStd::string& projectJsonFileName, PlistInitVector& plistPaths)
        : m_projectJson(JsonSettings{ projectJsonFileName, "", AZStd::make_unique<rapidjson::Document>() })
    {
        LoadProjectJsonData();

        for (PlatformAndPath& plistInfo : plistPaths)
        {
            m_pListsMap.insert(AZStd::pair<PlatformId, PlistSettings>(
                plistInfo.first,
                PlistSettings{ plistInfo.second, "", AZStd::make_unique<XmlDocument>() }));

            // We will have to find it then because unique_ptrs are not copy constructible
            // And a move constructor would copy large strings
            LoadPlist(m_pListsMap.find(plistInfo.first)->second);
        }
    }

    ProjectSettingsContainer::~ProjectSettingsContainer()
    {
    }

    ProjectSettingsContainer::PlistSettings* ProjectSettingsContainer::GetPlistSettingsForPlatform(const Platform& plat)
    {
        PlistSettings* result = nullptr;

        if (plat.m_type == PlatformDataType::Plist)
        {
            auto iter = m_pListsMap.find(plat.m_id);

            if (iter != m_pListsMap.end())
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

    bool ProjectSettingsContainer::IsPlistPlatform(const Platform& plat)
    {
        if (plat.m_type == PlatformDataType::Plist)
        {
            auto iter = m_pListsMap.find(plat.m_id);

            if (iter != m_pListsMap.end())
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

    void ProjectSettingsContainer::SavePlatformData(const Platform& plat)
    {
        PlistSettings* plistSettings = GetPlistSettingsForPlatform(plat);
        if (plistSettings != nullptr)
        {
            SavePlist(*plistSettings);
        }
        else
        {
            SaveProjectJsonData();
        }
    }

    void ProjectSettingsContainer::SaveProjectJsonData()
    {
        // Needed to write a document out to a string
        rapidjson::StringBuffer jsonDataBuffer;
        // Use pretty writer so it can be read easier
        rapidjson::PrettyWriter<rapidjson::StringBuffer> jsonDatawriter(jsonDataBuffer);

        m_projectJson.m_document->Accept(jsonDatawriter);
        const AZStd::string jsonDataString = jsonDataBuffer.GetString();

        StringOutcome outcome = WriteConfigFile(m_projectJson.m_path, jsonDataString);

        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError("Failed to save project.json", outcome.GetError()));
        }
    }

    void ProjectSettingsContainer::ReloadProjectJsonData()
    {
        m_projectJson.m_document.reset(new rapidjson::Document);
        LoadProjectJsonData();
    }

    void ProjectSettingsContainer::SavePlistsData()
    {
        for (AZStd::pair<PlatformId, PlistSettings>& plist : m_pListsMap)
        {
            LoadPlist(plist.second);
        }
    }

    void ProjectSettingsContainer::SavePlistData(const Platform& plat)
    {
        PlistSettings* settings = GetPlistSettingsForPlatform(plat);
        if (settings != nullptr)
        {
            SavePlist(*settings);
        }
    }

    void ProjectSettingsContainer::ReloadPlistData()
    {
        for (AZStd::pair<PlatformId, PlistSettings>& plist : m_pListsMap)
        {
            PlistSettings& plistSettings = plist.second;

            plistSettings.m_document.reset(new XmlDocument());
            LoadPlist(plistSettings);
        }
    }

    rapidjson::Document& ProjectSettingsContainer::GetProjectJsonDocument()
    {
        return *m_projectJson.m_document.get();
    }

    AZ::Outcome<rapidjson::Value*, void> ProjectSettingsContainer::GetProjectJsonValue(const char* key)
    {
        // Try to find member
        rapidjson::Document& settings = *m_projectJson.m_document;
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

    AZStd::unique_ptr<PlistDictionary> ProjectSettingsContainer::GetPlistDictionary(const Platform& plat)
    {
        if (plat.m_type == PlatformDataType::Plist)
        {
            PlistSettings* settings = GetPlistSettingsForPlatform(plat);
            if (settings != nullptr)
            {

                if (PlistDictionary::ContainsValidDict(settings->m_document.get()))
                {
                    return AZStd::make_unique<PlistDictionary>(settings->m_document.get());
                }
                else
                {
                    //TODO: Query user if they would like to remake a valid plist then do it
                    AZStd::string platformName;
                    switch (plat.m_id)
                    {
                    case PlatformId::Ios:
                        platformName = "iOS";
                        break;
                    default:
                        platformName = "unknown";
                        break;
                    }
                    AZ_Assert(false, "%s pList is in invalid state.", platformName.c_str());
                    return nullptr;
                }
            }
        }
        
        AZ_Assert(false, "This platform does not use pLists to store data.");
        return nullptr;
    }

    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& ProjectSettingsContainer::GetProjectJsonAllocator()
    {
        return m_projectJson.m_document->GetAllocator();
    }

    const char* ProjectSettingsContainer::GetFailedLoadingPlistText()
    {   
        return "Failed to load info.plist"; 
    }

    void ProjectSettingsContainer::LoadProjectJsonData()
    {
        StringOutcome outcome = ReadConfigFile(m_projectJson.m_path, m_projectJson.m_rawData);
        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError("Failed to load project.json", outcome.GetError()));
        }

        m_projectJson.m_document->Parse(m_projectJson.m_rawData.c_str());
    }

    // Loads info.plist for iOS from disk
    void ProjectSettingsContainer::LoadPlist(PlistSettings& plistSettings)
    {
        StringOutcome outcome = ReadConfigFile(plistSettings.m_path, plistSettings.m_rawData);
        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError(GetFailedLoadingPlistText(), outcome.GetError()));
        }

        plistSettings.m_document->parse<xmlFlags>(plistSettings.m_rawData.data());
    }

    void ProjectSettingsContainer::SavePlist(PlistSettings& plistSettings)
    {
        // Needed to write a document out to a string
        AZStd::string xmlDocString;
        rapidxml::print(std::back_inserter(xmlDocString), *plistSettings.m_document);

        StringOutcome outcome = WriteConfigFile(plistSettings.m_path, xmlDocString);

        if (!outcome.IsSuccess())
        {
            m_errors.push(SettingsError("Failed to save info.pList", outcome.GetError()));
        }
    }
} // namespace ProjectSettingsTool
