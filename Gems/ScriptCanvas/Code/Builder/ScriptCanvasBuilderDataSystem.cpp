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
    }

    void DataSystem::CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle)
    {
        using namespace ScriptCanvasBuilder;

        BuildResult result;

        CompleteDescriptionInPlace(sourceHandle);

        auto assetTreeOutcome = LoadEditorAssetTree(sourceHandle);
        if (!assetTreeOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "EditorScriptCanvasComponent::BuildGameEntityData failed: %s", assetTreeOutcome.GetError().c_str());
            result.status = BuilderDataStatus::Unloadable;
            m_buildResultsByHandle[sourceHandle] = AZStd::move(result);
            return;
        }

        auto parseOutcome = ParseEditorAssetTree(assetTreeOutcome.GetValue());
        if (!parseOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false
                , "EditorScriptCanvasComponent::BuildGameEntityData failed: %s", parseOutcome.GetError().c_str());
            result.status = BuilderDataStatus::Failed;
            m_buildResultsByHandle[sourceHandle] = AZStd::move(result);
            return;
        }

        parseOutcome.GetValue().SetHandlesToDescription();
        result.data = parseOutcome.TakeValue();
        result.status = BuilderDataStatus::Good;
        m_buildResultsByHandle[sourceHandle] = AZStd::move(result);
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
            CompileBuilderData(*handle);
            // broadcast change and result of change
        }
    }

    void DataSystem::SourceFileRemoved([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        // check that the file asset id still works, and use that
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        AZ_TracePrintf("SourceFile Removed: %s", fileAssetId.ToString<AZStd::string>().c_str());

        // try and set status to removed
        // broadcast removal
    }

    void DataSystem::SourceFileFailed([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (!IsScriptCanvasFile(relativePath))
        {
            return;
        }

        // try and set status to error
        // broadcast error
    }

}
