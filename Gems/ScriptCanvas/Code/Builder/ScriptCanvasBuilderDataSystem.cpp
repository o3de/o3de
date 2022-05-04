/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderDataSystem.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
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
}

namespace ScriptCanvasBuilder
{
    using namespace ScriptCanvasBuilderDataSystemCpp;

    DataSystem::DataSystem()
    {
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
        DataSystemSourceRequestsBus::Handler::BusConnect();
        DataSystemAssetRequestsBus::Handler::BusConnect();
    }

    DataSystem::~DataSystem()
    {
        DataSystemSourceRequestsBus::Handler::BusDisconnect();
        DataSystemAssetRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void DataSystem::AddResult(const ScriptCanvasEditor::SourceHandle& handle, BuilderSourceStorage&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[handle.Id()] = result;
    }

    void DataSystem::AddResult(AZ::Uuid&& id, BuilderSourceStorage&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[id] = result;
    }

    BuilderSourceResult DataSystem::CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        MutexLock lock(m_mutex);

        if (!m_buildResultsByHandle.contains(sourceHandle.Id()))
        {
            CompileBuilderDataInternal(sourceHandle);
        }

        BuilderSourceStorage& storage = m_buildResultsByHandle[sourceHandle.Id()];
        return BuilderSourceResult{ storage.status, &storage.data };
    }

    void DataSystem::CompileBuilderDataInternal(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        using namespace ScriptCanvasBuilder;

        CompleteDescriptionInPlace(sourceHandle);
        BuilderSourceStorage result;
        
        auto assetTreeOutcome = LoadEditorAssetTree(sourceHandle);
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

    BuilderAssetResult& DataSystem::MonitorAsset(AZ::Uuid sourceId)
    {
        const auto assetId = AZ::Data::AssetId(sourceId, ScriptCanvas::RuntimeDataSubId);
        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        ScriptCanvas::RuntimeAssetPtr asset(assetId, azrtti_typeid<ScriptCanvas::RuntimeAsset>());
        asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        m_assets[sourceId] = BuilderAssetResult{ BuilderAssetStatus::Pending, asset };
        BuilderAssetResult& result = m_assets[sourceId];
        result.data.QueueLoad();
        return result;
    }

    BuilderAssetResult DataSystem::LoadAsset(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        if (auto iter = m_assets.find(sourceHandle.Id()); iter != m_assets.end())
        {
            return iter->second;
        }
        else
        {
            return MonitorAsset(sourceHandle.Id());
        }
    }

    void DataSystem::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        const auto assetIdGuid = asset.GetId().m_guid;
        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetError: %s : %s"
            , asset.GetHint().c_str()
            , assetIdGuid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());

        auto& buildResult = m_assets[assetIdGuid];
        buildResult.data = asset;
        buildResult.status = BuilderAssetStatus::Error;
        DataSystemAssetNotificationsBus::Event
            ( assetIdGuid
            , &DataSystemAssetNotifications::OnAssetNotReady);
    }

    void DataSystem::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DATA_SYSTEM_STATUS
            ( "ScriptCanvas"
            , "DataSystem received OnAssetReady: %s : %s"
            , asset.GetHint().c_str()
            , asset.GetId().m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());
        ReportReadyFilter(asset);     
    }

    void DataSystem::ReportReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        using namespace ScriptCanvas;
        const auto assetIdGuid = asset.GetId().m_guid;

        auto& buildResult = m_assets[assetIdGuid];
        buildResult.data = asset;
        buildResult.data.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        
        if (auto result = IsPreloaded(buildResult.data); result != IsPreloadedResult::Yes)
        {
            AZ_Error("ScriptCanvas"
                , false
                , "DataSystem received ready for asset that was not loaded: %s-%s"
                , buildResult.data.GetHint().c_str()
                , assetIdGuid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str()
            );

            buildResult.status = BuilderAssetStatus::Error;
            DataSystemAssetNotificationsBus::Event
                ( assetIdGuid
                , &DataSystemAssetNotifications::OnAssetNotReady);
        }
        else
        {
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

        SCRIPT_SYSTEM_SCRIPT_STATUS("ScriptCanvas", "DataSystem::ReportReadyFilter received a runtime asset, queuing Lua script processing.");
        AZ::SystemTickBus::QueueFunction([this, asset]()
        {
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

    void DataSystem::SourceFileChanged(AZStd::string relativePath, [[maybe_unused]] AZStd::string scanFolder, AZ::Uuid sourceId)
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

        if (auto handle = ScriptCanvasEditor::CompleteDescription(ScriptCanvasEditor::SourceHandle(nullptr, sourceId, {})))
        {
            CompileBuilderDataInternal(*handle);
            auto& builderStorage = m_buildResultsByHandle[sourceId];
            DataSystemSourceNotificationsBus::Event
                ( sourceId
                , &DataSystemSourceNotifications::SourceFileChanged
                , BuilderSourceResult{ builderStorage.status, &builderStorage.data }
                , relativePath
                , scanFolder);
        }
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
