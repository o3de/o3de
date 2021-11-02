/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphCanvas.h"

namespace GraphCanvas
{

    TranslationDatabase::TranslationDatabase()
    {
        TranslationRequestBus::Handler::BusConnect();
    }

    TranslationDatabase::~TranslationDatabase()
    {
        TranslationRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void TranslationDatabase::Init()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void TranslationDatabase::Restore()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

        m_database.clear();

        AZStd::function<void()> reloadFn = []()
        {
            // Collects all script assets for reloading
            AZ::Data::AssetCatalogRequests::AssetEnumerationCB collectAssetsCb = [](const AZ::Data::AssetId, const AZ::Data::AssetInfo& info)
            {
                // Check asset type
                if (info.m_assetType == azrtti_typeid<TranslationAsset>())
                {
                    auto asset = AZ::Data::AssetManager::Instance().GetAsset<TranslationAsset>(info.m_assetId, AZ::Data::AssetLoadBehavior::Default);
                    if (asset && asset.IsReady())
                    {
                        // Reload the asset from it's current data
                        asset.Reload();
                    }
                }
            };
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, collectAssetsCb, nullptr);
        };

        AZ::TickBus::QueueFunction(reloadFn);
    }

    void TranslationDatabase::DumpDatabase(const AZStd::string& filename)
    {
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Value entries(rapidjson::kArrayType);

        for (const auto& entry : m_database)
        {
            rapidjson::Value key(rapidjson::kStringType);
            key.SetString(entry.first.c_str(), document.GetAllocator());

            rapidjson::Value value(rapidjson::kStringType);
            value.SetString(entry.second.c_str(), document.GetAllocator());

            rapidjson::Value item(rapidjson::kObjectType);
            item.AddMember(key, value, document.GetAllocator());

            entries.PushBack(item, document.GetAllocator());

        }

        document.AddMember("entries", entries, document.GetAllocator());

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(filename.c_str(),
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE |
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
            AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error("Translation", false, "Failed to create output file: %s", filename.c_str());
            return;
        }

        rapidjson::StringBuffer scratchBuffer;

        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);
        document.Accept(writer);

        outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
        outputFile.Close();

        scratchBuffer.Clear();
    }

    bool TranslationDatabase::HasKey(const AZStd::string& key)
    {
        return m_database.find(key) != m_database.end();
    }

    GraphCanvas::TranslationRequests::Details TranslationDatabase::GetDetails(const AZStd::string& key, const Details& fallbackDetails)
    {
        Details details;
        if (!Get(key + ".name", details.Name))
        {
            details.Name = fallbackDetails.Name;
        }

        if (!Get(key + ".tooltip", details.Tooltip))
        {
            details.Tooltip = fallbackDetails.Tooltip;
        }

        if (!Get(key + ".subtitle", details.Subtitle))
        {
            details.Subtitle = fallbackDetails.Subtitle;
        }

        if (!Get(key + ".category", details.Category))
        {
            details.Category = fallbackDetails.Category;
        }

        return details;
    }

    bool TranslationDatabase::Get(const AZStd::string& key, AZStd::string& value)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

        if (m_database.find(key) != m_database.end())
        {
            value = m_database[key].c_str();
            return true;
        }

        static bool s_traceMissingItems = true;
        if (s_traceMissingItems)
        {
            AZ_TracePrintf("GraphCanvas", AZStd::string::format("Value not found for key: %s", key.c_str()).c_str());
        }

        return false;
    }

    bool TranslationDatabase::Add(const TranslationFormat& format)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

        bool warnings = false;

        for (auto& entry : format.m_database)
        {
            if (m_database.find(entry.first) == m_database.end())
            {
                m_database[entry.first] = entry.second;
            }
            else
            {
                AZStd::string warning = AZStd::string::format("Unable to store key: %s with value: %s because that key already exists with value: %s", entry.first.c_str(), entry.second.c_str(), m_database[entry.first].c_str());
                AZ_Warning("TranslationSerializer", false, warning.c_str());
                warnings = true;
            }
        }

        return warnings;
    }

    bool TranslationDatabase::IsDuplicate(const AZStd::string& key)
    {
        return m_database.find(key) != m_database.end();
    }

}
