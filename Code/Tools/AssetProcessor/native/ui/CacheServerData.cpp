/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/CacheServerData.h>
#include <native/utilities/AssetServerHandler.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/pointer.h>

#include <QFile>
#include <QTextStream>

namespace AssetProcessor
{
    void CacheServerData::Reset()
    {
        auto* recognizerConfiguration = AZ::Interface<AssetProcessor::RecognizerConfiguration>::Get();
        if (recognizerConfiguration)
        {
            m_patternContainer = recognizerConfiguration->GetAssetCacheRecognizerContainer();
        }

        using namespace AssetProcessor;
        AssetServerBus::BroadcastResult(m_cachingMode, &AssetServerBus::Events::GetRemoteCachingMode);
        AssetServerBus::BroadcastResult(m_serverAddress, &AssetServerBus::Events::GetServerAddress);
        m_dirty = false;
    }

    bool CacheServerData::Save(const AZ::IO::Path& projectPath)
    {
        // construct JSON for writing out a .setreg for current settings
        rapidjson::Document doc;
        doc.SetObject();

        // this builds up the JSON doc for each key inside AssetProcessorSettingsKey
        AZStd::string path;
        AZ::StringFunc::TokenizeVisitor(
            AssetProcessor::AssetProcessorServerKey,
            [&doc, &path](AZStd::string_view elem)
            {
                auto key = rapidjson::StringRef(elem.data(), elem.size());
                rapidjson::Value value(rapidjson::kObjectType);
                if (path.empty())
                {
                    doc.AddMember(key, value, doc.GetAllocator());
                }
                else
                {
                    auto* node = rapidjson::Pointer(path.c_str(), path.size()).Get(doc);
                    node->AddMember(key, value, doc.GetAllocator());
                }
                path += "/";
                path += elem;
            },
            '/');

        // creates a rapidjson doc to hold the shared cache server settings
        rapidjson::Value value(rapidjson::kObjectType);
        auto server = rapidjson::Pointer(AssetProcessor::AssetProcessorServerKey).Get(doc);

        server->AddMember(
            rapidjson::StringRef(AssetProcessor::CacheServerAddressKey),
            rapidjson::StringRef(m_serverAddress.c_str()),
            doc.GetAllocator());

        server->AddMember(
            rapidjson::StringRef(AssetProcessor::AssetCacheServerModeKey),
            rapidjson::StringRef(AssetProcessor::AssetServerHandler::GetAssetServerModeText(m_cachingMode)),
            doc.GetAllocator());

        // add the cache patterns
        AZStd::string jsonText;
        AssetProcessor::PlatformConfiguration::ConvertToJson(m_patternContainer, jsonText);
        if (!jsonText.empty())
        {
            rapidjson::Document recognizerDoc;
            recognizerDoc.Parse(jsonText.c_str());
            for (auto member = recognizerDoc.MemberBegin(); member != recognizerDoc.MemberEnd(); ++member)
            {
                rapidjson::Value valuePattern;
                valuePattern.CopyFrom(member->value, doc.GetAllocator(), true);
                rapidjson::Value valueKey;
                valueKey.CopyFrom(member->name, doc.GetAllocator(), true);
                server->AddMember(AZStd::move(valueKey), AZStd::move(valuePattern), doc.GetAllocator());
            }
        }

        // get project folder and construct Registry project folder
        const char* assetCacheServerSettings = "asset_cache_server_settings.setreg";
        AZ::IO::Path fullpath( projectPath );
        fullpath /= "Registry";
        fullpath /= assetCacheServerSettings;

        QFile file(fullpath.c_str());
        bool result = file.open(QIODevice::ReadWrite);
        if (result)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            // rewrite the file contents
            file.resize(0);
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream << buffer.GetString();

            m_statusLevel = StatusLevel::Notice;
            m_statusMessage = AZStd::string::format("Updated settings file (%s)", fullpath.c_str());
            m_updateStatus = true;
        }
        else
        {
            m_statusLevel = StatusLevel::Error;
            m_statusMessage = AZStd::string::format("**Error**: Could not write settings file (%s)", fullpath.c_str());
            m_updateStatus = false;
        }
        file.close();
        return result;
    }
}
