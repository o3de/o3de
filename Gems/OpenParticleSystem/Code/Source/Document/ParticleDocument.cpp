/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/ParticleGraphicsViewRequestsBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Document/ParticleDocument.h>
#include <Editor/EditorParticleSystemComponentRequestBus.h>

namespace OpenParticleSystemEditor
{
    template<typename ObjectType>
    bool LoadFromFile(const AZStd::string& path, ObjectType& objectData)
    {
        objectData = ObjectType();

        auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(path);
        if (!loadOutcome.IsSuccess())
        {
            AZ_Error("AZ::JsonSerializationUtils", false, "%s", loadOutcome.GetError().c_str());
            return false;
        }

        rapidjson::Document& document = loadOutcome.GetValue();

        AZ::JsonDeserializerSettings jsonSettings;
        AZ::RPI::JsonReportingHelper reportingHelper;
        reportingHelper.Attach(jsonSettings);

        AZ::JsonSerialization::Load(objectData, document, jsonSettings);

        if (reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::JsonSerializationUtils", false, "Failed to load object from JSON file: %s", path.c_str());
            return false;
        }
        return true;
    }

    template<typename ObjectType>
    bool SaveToFile(const AZStd::string& path, const ObjectType& objectData)
    {
        rapidjson::Document document;
        document.SetObject();

        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;
        AZ::RPI::JsonReportingHelper reportingHelper;
        reportingHelper.Attach(settings);

        AZ::JsonSerialization::Store(document, document.GetAllocator(), objectData, settings);
        if (reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write object data to JSON document: %s", path.c_str());
            return false;
        }

        AZ::JsonSerializationUtils::WriteJsonFile(document, path);
        if (reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write JSON document to file: %s", path.c_str());
            return false;
        }

        return true;
    }


    ParticleDocument::ParticleDocument()
    {
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();

        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext != nullptr, "invalid serialize context");

        AZ::TypeId sysType;
        OpenParticle::EditorParticleSystemComponentRequestBus::BroadcastResult(sysType, &OpenParticle::EditorParticleSystemComponentRequests::GetParticleSystemConfigType);
        m_sourceData.m_config = m_serializeContext->CreateAny(sysType);
    }

    ParticleDocument::~ParticleDocument()
    {
        ParticleDocumentRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
    }

    void ParticleDocument::OpenParticle(const AZStd::string_view full_path)
    {
        Open(full_path);
    }

    bool ParticleDocument::UpdateAsset()
    {
        // Create ParticleAsset from ParticleSourceData.
        m_sourceData.ToEditor();
        m_sourceData.ToRuntime();

        if (!m_sourceData.CheckDistributionIndex())
        {
            AZ_Error("ParticleSourceData", false, "Distribution index doesn't match distribution.");
            return false;
        }

        auto createRst = m_sourceData.CreateParticleAsset(AZ::Uuid::CreateName(m_absolutePath.c_str()), m_absolutePath, true);
        if (!createRst)
        {
            AZ_Error("Particle Document", false, "Particle asset could not be created from source data: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_particleAsset = createRst.GetValue();
        if (!m_particleAsset.IsReady())
        {
            AZ_Error("Particle Document", false, "Particle asset is not ready: '%s'.", m_absolutePath.c_str());
            return false;
        }

        ParticleDocumentNotifyBus::Broadcast(&ParticleDocumentNotifyBus::Handler::OnDocumentOpened, m_particleAsset, m_absolutePath);

        return true;
    }

    bool ParticleDocument::Open(AZStd::string_view loadPath)
    {
        m_absolutePath = loadPath;
        AZ_Printf("Emitter", "ParticleDocument::Open|%s", m_absolutePath.c_str());

        AZStd::string fileName;
        if (!AzFramework::StringFunc::Path::GetFileName(m_absolutePath.c_str(), fileName))
        {
            AZ_Error("Particle Document", false, "Particle document path is not valid: '%s'.", m_absolutePath.c_str());
            return false;
        }
        ParticleDocumentRequestBus::Handler::BusConnect(fileName);

        if (!AzFramework::StringFunc::Path::Normalize(m_absolutePath))
        {
            AZ_Error("Particle Document", false, "Particle document path could not be normalized: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (AzFramework::StringFunc::Path::IsRelative(m_absolutePath.c_str()))
        {
            AZ_Error("Particle Document", false, "Particle document path must be absolute: '%s'.", m_absolutePath.c_str());
            return false;
        }

        bool result = false;
        AZ::Data::AssetInfo sourceAssetInfo;
        AZStd::string watchFolder;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
            m_absolutePath.c_str(), sourceAssetInfo, watchFolder);
        if (!result)
        {
            AZ_Error("Particle Document", false, "Could not find source particle: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_sourceAssetId = sourceAssetInfo.m_assetId;
        m_relativePath = sourceAssetInfo.m_relativePath;

        if (!AzFramework::StringFunc::Path::Normalize(m_relativePath))
        {
            AZ_Error("Particle Document", false, "Particle document path could not be normalized: '%s'.", m_relativePath.c_str());
            return false;
        }

        if (!AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), "particle"))
        {
            AZ_Error("Particle Document", false, "Particle document extension not supported: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // if changed data before. clear the caches data
        m_sourceData.m_distribution.ClearCaches();

        // Load ParticleSourceData.
        if (!LoadFromFile(m_absolutePath, m_sourceData))
        {
            return false;
        }

        m_sourceData.Normalize();
        m_sourceData.EmittersToDetails();

        // show in ParticleSystemEditor
        ParticleDocumentNotifyBus::Broadcast(&ParticleDocumentNotifyBus::Handler::OnParticleSourceDataLoaded, &m_sourceData, m_absolutePath);
        EBUS_EVENT(OpenParticle::EditorParticleDocumentBusRequestsBus, OnEmitterNameChanged, &m_sourceData);
        UpdateAsset();
        m_modified = false;

        return true;
    }

    bool ParticleDocument::Save()
    {
        AZStd::string fileName;
        if (!AzFramework::StringFunc::Path::GetFileName(m_absolutePath.c_str(), fileName))
        {
            AZ_Error("Particle Document", false, "Particle document path is not valid: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (!IsOpen())
        {
            AZ_Error("Particle Document", false, "Particle document is not open to be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        EBUS_EVENT_ID(fileName, ParticleGraphicsViewRequestsBus, CheckAllParticleItemWidgetWithoutBusNotification);
        if (!SaveToFile(m_absolutePath, m_sourceData))
        {
            AZ_Error("Particle Document", false, "Particle document could not be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        EBUS_EVENT_ID(fileName, ParticleGraphicsViewRequestsBus, RestoreAllParticleItemWidgetStatusAfterCheckAll);
        m_modified = false;
        AZ_TracePrintf("Particle Document", "Particle document saved: '%s'.\n", m_absolutePath.c_str());

        return true;
    }

    bool ParticleDocument::CreateParticle(AZStd::string_view absolutePath)
    {
        if (absolutePath.empty())
        {
            AZ_Error("Particle Document", false, "New particle document path is empty: '%s'.", absolutePath.data());
            return false;
        }

        AZ::TypeId sysType;

        OpenParticle::EditorParticleSystemComponentRequestBus::BroadcastResult(
            sysType, &OpenParticle::EditorParticleSystemComponentRequests::GetParticleSystemConfigType);
        OpenParticle::ParticleSourceData newSourceData;
        newSourceData.m_config = m_serializeContext->CreateAny(sysType);

        newSourceData.AddEmitter("Emitter");

        OpenParticle::EditorParticleSystemComponentRequestBus::BroadcastResult(
            sysType, &OpenParticle::EditorParticleSystemComponentRequests::GetDefaultEmitType);
        newSourceData.m_emitters[0]->AddEmitModule(m_serializeContext->CreateAny(sysType));

        OpenParticle::EditorParticleSystemComponentRequestBus::BroadcastResult(
            sysType, &OpenParticle::EditorParticleSystemComponentRequests::GetDefaultRenderType);
        newSourceData.m_emitters[0]->SetRender(m_serializeContext->CreateAny(sysType));

        AZStd::vector<AZ::TypeId> sysTypes;
        OpenParticle::EditorParticleSystemComponentRequestBus::BroadcastResult(
            sysTypes, &OpenParticle::EditorParticleSystemComponentRequests::GetDefaultSpawnTypes);
        for (const auto& type : sysTypes)
        {
            newSourceData.m_emitters[0]->AddSpawnModule(m_serializeContext->CreateAny(type));
        }
        
        if (!SaveToFile(absolutePath, newSourceData))
        {
            AZ_Error("Particle Document", false, "New particle document could not be saved: '%s'.", absolutePath.data());
            return false;
        }

        AZ_TracePrintf("Particle Document", "New particle document saved: '%s'.", absolutePath.data());
        return true;
    }

    bool ParticleDocument::Close()
    {
        m_modified = false;
        return true;
    }

    bool ParticleDocument::IsOpen() const
    {
        return !m_absolutePath.empty() && !m_relativePath.empty();
    }

    void ParticleDocument::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID)
    {
        (void)relativePath;
        (void)scanFolder;
        (void)sourceUUID;
    }

    void ParticleDocument::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        (void)asset;
    }

    OpenParticle::ParticleSourceData::DetailInfo* ParticleDocument::AddDetail(const AZStd::string& str)
    {
        m_modified = true;
        auto detailInfo = m_sourceData.AddDetail(str);
        UpdateAsset();
        return detailInfo;
    }

    void ParticleDocument::RemoveDetail(OpenParticle::ParticleSourceData::DetailInfo* info)
    {
        m_sourceData.RemoveDetail(info);
        UpdateAsset();
        m_modified = true;
    }

    OpenParticle::ParticleSourceData* ParticleDocument::GetParticleSourceData()
    {
        return &m_sourceData;
    }

    void ParticleDocument::NotifyParticleSourceDataModified()
    {
        m_modified = true;
        UpdateAsset();
    }

    bool ParticleDocument::IsModified() const
    {
        return m_modified;
    }

    AZStd::string_view ParticleDocument::GetAbsolutePath() const
    {
        return m_absolutePath;
    }

    OpenParticle::ParticleSourceData::DetailInfo* ParticleDocument::CopyDetail(AZStd::string& str)
    {
        AZStd::string destDetailName = AZStd::string::format("%s Copy", str.data());
        auto detailInfo = m_sourceData.CopyDetail(str, destDetailName);
        auto emitterInfo = m_sourceData.CopyEmitter();
        m_copyDetail = detailInfo;
        m_copyEmitter = emitterInfo;
        UpdateAsset();
        return detailInfo;
    }

    void ParticleDocument::SetCopyName(AZStd::string copyName)
    {
        m_copyWidgetTitleName = copyName;
    }

    OpenParticle::ParticleSourceData::DetailInfo* ParticleDocument::GetDetail()
    {
        return m_copyDetail;
    };

    OpenParticle::ParticleSourceData::EmitterInfo* ParticleDocument::GetEmitter()
    {
        return m_copyEmitter;
    }

    OpenParticle::ParticleSourceData::DetailInfo* ParticleDocument::SetEmitterAndDetail(OpenParticle::ParticleSourceData::EmitterInfo* destEmitter, OpenParticle::ParticleSourceData::DetailInfo* destDetail)
    {
        AZStd::string destDetailName = destEmitter->m_name;
        AZ::u8 n = 0;
        AZStd::vector<OpenParticle::ParticleSourceData::EmitterInfo*>::const_iterator iter;
        while (true)
        {
            iter = AZStd::find_if(m_sourceData.m_emitters.begin(), m_sourceData.m_emitters.end(), [destDetailName](const auto& emitter)
                {
                    return emitter->m_name == destDetailName;
                });
            if (iter == m_sourceData.m_emitters.end())
            {
                break;
            }
            destDetailName = AZStd::string::format("%s %d", destEmitter->m_name.c_str(), ++n);
        };
        return m_sourceData.SetDestItem(destEmitter, destDetail, destDetailName);
    }

    AZStd::string ParticleDocument::GetCopyWidgetName()
    {
        return m_copyWidgetTitleName;
    }

    void ParticleDocument::ClearCopyCache()
    {
        m_sourceData.ClearCopyCache();
    }
}
