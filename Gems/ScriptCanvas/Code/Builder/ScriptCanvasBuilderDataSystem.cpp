/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <Builder/ScriptCanvasBuilderDataSystem.h>

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
        DataSystemRequestsBus::Handler::BusConnect();
    }

    void DataSystem::AddResult(const ScriptCanvasEditor::SourceHandle& handle, BuildResult&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[handle.Id()] = result;
    }

    void DataSystem::AddResult(AZ::Uuid&& id, BuildResult&& result)
    {
        MutexLock lock(m_mutex);
        m_buildResultsByHandle[id] = result;
    }

    BuildResult DataSystem::CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        MutexLock lock(m_mutex);

        if (!m_buildResultsByHandle.contains(sourceHandle.Id()))
        {
            CompileBuilderDataInternal(sourceHandle);
        }

        return m_buildResultsByHandle[sourceHandle.Id()];
    }

    void DataSystem::CompileBuilderDataInternal(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        using namespace ScriptCanvasBuilder;

        CompleteDescriptionInPlace(sourceHandle);
        BuildResult result;
        
        auto assetTreeOutcome = LoadEditorAssetTree(sourceHandle);
        if (!assetTreeOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "DataSystem::CompileBuilderDataInternal failed: %s", assetTreeOutcome.GetError().c_str());
            result.status = BuilderDataStatus::Unloadable;
            AddResult(sourceHandle, AZStd::move(result));
            return;
        }

        auto parseOutcome = ParseEditorAssetTree(assetTreeOutcome.GetValue());
        if (!parseOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "DataSystem::CompileBuilderDataInternal failed: %s", parseOutcome.GetError().c_str());
            result.status = BuilderDataStatus::Failed;
            AddResult(sourceHandle, AZStd::move(result));
            return;
        }

        parseOutcome.GetValue().SetHandlesToDescription();
        result.data = parseOutcome.TakeValue();
        result.status = BuilderDataStatus::Good;
        AddResult(sourceHandle, AZStd::move(result));
    }

    void DataSystem::SourceFileChanged([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        if (auto handle = ScriptCanvasEditor::CompleteDescription(ScriptCanvasEditor::SourceHandle(nullptr, fileAssetId, {})))
        {
            CompileBuilderDataInternal(*handle);
            DataSystemNotificationsBus::Event
                ( fileAssetId
                , &DataSystemNotifications::SourceFileChanged
                , m_buildResultsByHandle[fileAssetId]
                , relativePath
                , scanFolder);
        }
    }

    void DataSystem::SourceFileRemoved([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        BuildResult result;
        result.status = BuilderDataStatus::Removed;
        AddResult(AZStd::move(fileAssetId), AZStd::move(result));
        DataSystemNotificationsBus::Event
            ( fileAssetId
            , &DataSystemNotifications::SourceFileRemoved
            , relativePath
            , scanFolder);
    }

    void DataSystem::SourceFileFailed([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        BuildResult result;
        result.status = BuilderDataStatus::Failed;
        AddResult(AZStd::move(fileAssetId), AZStd::move(result));
        DataSystemNotificationsBus::Event
            ( fileAssetId
            , &DataSystemNotifications::SourceFileFailed
            , relativePath
            , scanFolder);
    }
}
