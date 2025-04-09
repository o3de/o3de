/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderDataSystem.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

namespace ScriptCanvasBuilderDataSystemCpp
{
    bool IsScriptCanvasFile(AZStd::string_view candidate)
    {
        AZ::IO::Path path(candidate);
        return path.HasExtension() && path.Extension() == ".scriptcanvas";
    }

    AZStd::optional<AZ::Uuid> GetUuid(AZStd::string_view candidate)
    {
        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool result = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult
            ( result
            , &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath
            , candidate.data()
            , assetInfo
            , watchFolder);

        return assetInfo.m_assetId.m_guid;
    }
}

namespace ScriptCanvasBuilder
{
    using namespace ScriptCanvasBuilderDataSystemCpp;

    DataSystem::DataSystem()
    {
        AzFramework::AssetSystemInfoBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        DataSystemAssetRequestsBus::Handler::BusConnect();
        DataSystemSourceRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
    }

    DataSystem::~DataSystem()
    {
        DataSystemAssetRequestsBus::Handler::BusDisconnect();
        DataSystemSourceRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void DataSystem::AddResult(const SourceHandle& handle, BuilderSourceStorage&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[handle.Id()] = result;
    }

    void DataSystem::AddResult(AZ::Uuid&& id, BuilderSourceStorage&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[id] = result;
    }

    BuilderSourceResult DataSystem::CompileBuilderData(SourceHandle sourceHandle)
    {
        MutexLock lock(m_mutex);

        if (!m_buildResultsByHandle.contains(sourceHandle.Id()))
        {
            CompileBuilderDataInternal(sourceHandle);
        }

        BuilderSourceStorage& storage = m_buildResultsByHandle[sourceHandle.Id()];
        return BuilderSourceResult{ storage.status, &storage.data };
    }

    void DataSystem::CompileBuilderDataInternal(SourceHandle sourceHandle)
    {
        using namespace ScriptCanvasBuilder;

        m_buildResultsByHandle.clear();

        BuilderSourceStorage result;

        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        ScriptCanvas::MakeInternalGraphEntitiesUnique makeUnique = ScriptCanvas::MakeInternalGraphEntitiesUnique::Yes;
        const bool isAssetProcessor = appType.IsValid() && (appType.IsTool() && !appType.IsEditor());
        if (isAssetProcessor)
        {
            // Allow to keep the same entity UIDs between the editable scriptCanvas and the compiled scriptCanvas files,
            // This is needed to support debug features such as breakpoints.
            // In editor we force the UIDs to be re-generated to prevent UIDs collision as entities are not unregistered on file reload.
            makeUnique = ScriptCanvas::MakeInternalGraphEntitiesUnique::No;
        }

        auto assetTreeOutcome = LoadEditorAssetTree(sourceHandle, makeUnique);
        if (!assetTreeOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "DataSystem::CompileBuilderDataInternal failed: %s", assetTreeOutcome.GetError().c_str());
            result.status = BuilderSourceStatus::Unloadable;
            AddResult(sourceHandle, AZStd::move(result));
            return;
        }

        auto parseOutcome = ParseEditorAssetTree(assetTreeOutcome.GetValue());
        if (!parseOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "DataSystem::CompileBuilderDataInternal failed: %s", parseOutcome.GetError().c_str());
            result.status = BuilderSourceStatus::Failed;
            AddResult(sourceHandle, AZStd::move(result));
            return;
        }

        parseOutcome.GetValue().SetHandlesToDescription();
        result.data = parseOutcome.TakeValue();
        result.status = BuilderSourceStatus::Good;
        AddResult(sourceHandle, AZStd::move(result));
    }

    void DataSystem::MarkAssetInError(AZ::Uuid assetIdGuid)
    {
        auto& buildResult = m_assets[assetIdGuid];
        buildResult.data = {};
        buildResult.status = BuilderAssetStatus::Error;
        DataSystemAssetNotificationsBus::Event
            ( assetIdGuid
            , &DataSystemAssetNotifications::OnAssetNotReady);

        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetError: %s"
            , assetIdGuid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
    }

    BuilderAssetResult& DataSystem::MonitorAsset(AZ::Uuid sourceId)
    {
        const auto assetId = AZ::Data::AssetId(sourceId, ScriptCanvas::RuntimeDataSubId);
        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        ScriptCanvas::RuntimeAssetPtr asset(assetId, azrtti_typeid<ScriptCanvas::RuntimeAsset>());
        asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        m_assets[sourceId] = BuilderAssetResult{ BuilderAssetStatus::Pending, asset };
        return m_assets[sourceId];
    }

    BuilderAssetResult DataSystem::LoadAsset(SourceHandle sourceHandle)
    {
        BuilderAssetResult* result = nullptr;;

        if (auto iter = m_assets.find(sourceHandle.Id()); iter != m_assets.end())
        {
            result = &iter->second;
        }
        else
        {
            result = &MonitorAsset(sourceHandle.Id());
        }

        result->data.QueueLoad();
        return *result;
    }

    void DataSystem::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        const auto assetIdGuid = asset.GetId().m_guid;
        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetError: %s : %s, marking asset in error"
            , asset.GetHint().c_str()
            , assetIdGuid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        MarkAssetInError(assetIdGuid);
    }

    void DataSystem::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetReady: %s : %s, reporting it ready"
            , asset.GetHint().c_str()
            , asset.GetId().m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        ReportReadyFilter(asset);     
    }

    void DataSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        if (assetId.m_subId != ScriptCanvas::RuntimeDataSubId)
        {
            return;
        }

        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnCatalogAssetAdded: %s, monitoring asset"
            , assetId.m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        MonitorAsset(assetId.m_guid).data.QueueLoad();
    }

    void DataSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        if (assetId.m_subId != ScriptCanvas::RuntimeDataSubId)
        {
            return;
        }

        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnCatalogAssetChanged: %s, monitoring asset"
                , assetId.m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        MonitorAsset(assetId.m_guid).data.QueueLoad();
    }

    void DataSystem::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
    {
        if (assetId.m_subId != ScriptCanvas::RuntimeDataSubId)
        {
            return;
        }

        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnCatalogAssetRemoved: %s, marking asset in error"
            , assetId.m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        MarkAssetInError(assetId.m_guid);
    }

    void DataSystem::ReportReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        using namespace ScriptCanvas;
        const auto assetIdGuid = asset.GetId().m_guid;

        auto& buildResult = m_assets[assetIdGuid];
        buildResult.data = asset;
        buildResult.data.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

        DATA_SYSTEM_STATUS("ScriptCanvas", "DataSystem::ReportReady received a runtime asset");

        if (auto result = IsPreloaded(buildResult.data); result != IsPreloadedResult::Yes)
        {
            AZ_Error("ScriptCanvas"
                , false
                , "DataSystem received ready for asset that was not loaded: %s-%s"
                , buildResult.data.GetHint().c_str()
                , assetIdGuid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str()
            );

            DATA_SYSTEM_STATUS("ScriptCanvas", "DataSystem::ReportReady received a runtime asset, but it was not pre-loaded");
            buildResult.status = BuilderAssetStatus::Error;
            DataSystemAssetNotificationsBus::Event
                ( assetIdGuid
                , &DataSystemAssetNotifications::OnAssetNotReady);
        }
        else
        {
            DATA_SYSTEM_STATUS("ScriptCanvas", "DataSystem::ReportReady received a runtime asset and it is ready");
            buildResult.status = BuilderAssetStatus::Ready;
            DataSystemAssetNotificationsBus::Event
                ( assetIdGuid
                , &DataSystemAssetNotifications::OnReady
                , buildResult.data);
        }
    }

    void DataSystem::ReportReadyFilter(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        using namespace ScriptCanvas;

        DATA_SYSTEM_STATUS("ScriptCanvas", "DataSystem::ReportReadyFilter received a runtime asset, queuing Lua script processing.");
        SCRIPT_SYSTEM_SCRIPT_STATUS("ScriptCanvas", "DataSystem::ReportReadyFilter received a runtime asset, queuing Lua script processing.");

        AZ::SystemTickBus::QueueFunction([this, asset]()
        {
            DATA_SYSTEM_STATUS("ScriptCanvas", "DataSystem::ReportReadyFilter executing Lua script processing.");
            SCRIPT_SYSTEM_SCRIPT_STATUS("ScriptCanvas", "DataSystem::ReportReadyFilter executing Lua script processing.");

            const auto assetIdGuid = asset.GetId().m_guid;
            
            auto& buildResult = m_assets[assetIdGuid];
            buildResult.data = asset;
            buildResult.data.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

            auto& luaAsset = buildResult.data.Get()->m_runtimeData.m_script;
            luaAsset
                = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(luaAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad, {});
            luaAsset.QueueLoad();
            luaAsset.BlockUntilLoadComplete();
            ReportReady(buildResult.data);
        });
    }

    void DataSystem::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetReloaded: %s : %s"
            , asset.GetHint().c_str()
            , asset.GetId().m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        ReportReadyFilter(asset);
    }

    void DataSystem::OnAssetUnloaded(const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType)
    {
        DataSystemAssetNotificationsBus::Event(assetId.m_guid, &DataSystemAssetNotifications::OnAssetNotReady);
        MonitorAsset(assetId.m_guid);
    }

    void DataSystem::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        SCRIPT_SYSTEM_SCRIPT_STATUS
            ( "ScriptCanvas"
            , "DataSystem received source file changed: %s : %s"
            , relativePath.c_str()
            , sourceId.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());

        DataSystemAssetNotificationsBus::Event(sourceId, &DataSystemAssetNotifications::OnAssetNotReady);
        MonitorAsset(sourceId);

        auto handle = SourceHandle::FromRelativePathAndScanFolder(relativePath, scanFolder, sourceId);
        CompileBuilderDataInternal(handle);
        auto& builderStorage = m_buildResultsByHandle[sourceId];
        DataSystemSourceNotificationsBus::Event
            ( sourceId
            , &DataSystemSourceNotifications::SourceFileChanged
            , BuilderSourceResult{ builderStorage.status, &builderStorage.data }
            , relativePath
            , scanFolder);
    }

    void DataSystem::SourceFileRemoved(AZStd::string relativePath, [[maybe_unused]] AZStd::string scanFolder, AZ::Uuid sourceId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        BuilderSourceStorage result;
        result.status = BuilderSourceStatus::Removed;
        AddResult(AZStd::move(sourceId), AZStd::move(result));
        DataSystemSourceNotificationsBus::Event
            ( sourceId
            , &DataSystemSourceNotifications::SourceFileRemoved
            , relativePath
            , scanFolder);
    }

    void DataSystem::SourceFileFailed(AZStd::string relativePath, [[maybe_unused]] AZStd::string scanFolder, AZ::Uuid sourceId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        BuilderSourceStorage result;
        result.status = BuilderSourceStatus::Failed;
        AddResult(AZStd::move(sourceId), AZStd::move(result));
        DataSystemSourceNotificationsBus::Event
            ( sourceId
            , &DataSystemSourceNotifications::SourceFileFailed
            , relativePath
            , scanFolder);
    }
}
