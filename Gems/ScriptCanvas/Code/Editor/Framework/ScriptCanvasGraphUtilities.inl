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

#include <Asset/EditorAssetSystemComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Editor/Framework/ScriptCanvasReporter.h>

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;

    inline void RunGraph(AZStd::string_view graphPath, [[maybe_unused]] ScriptCanvas::ExecutionMode execution, [[maybe_unused]] const DurationSpec& duration, Reporter& reporter)
    {
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
        LoadTestGraphResult loadResult = LoadTestGraph(graphPath);
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);

        if (loadResult.m_graph)
        {
            reporter.SetGraph(loadResult.m_graph->GetScriptCanvasId(), loadResult.m_graph->GetEntityId());

            loadResult.m_graphEntity->Init();
            TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
            loadResult.m_graphEntity->Activate();

            TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
            loadResult.m_graphEntity->Deactivate();
            reporter.FinishReport(loadResult.m_graph->IsInErrorState());
            loadResult.m_graphEntity.reset();
        }
        else
        {
            reporter.FinishReport();
        }
    }

    inline Reporter RunGraph(AZStd::string_view graphPath, ExecutionMode execution, const DurationSpec& duration)
    {
        Reporter reporter;
        RunGraph(graphPath, execution, duration, reporter);
        return reporter;
    }

    inline Reporter RunGraph(AZStd::string_view graphPath, const DurationSpec& duration)
    {
        return RunGraph(graphPath, ExecutionMode::Interpreted, duration);
    }

    inline Reporter RunGraph(AZStd::string_view graphPath, ExecutionMode execution)
    {
        return RunGraph(graphPath, execution, DurationSpec());
    }

    inline LoadTestGraphResult LoadTestGraph(AZStd::string_view graphPath)
    {
        AZ::Outcome< AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> assetOutcome = AZ::Failure(AZStd::string("asset creation failed"));
        ScriptCanvasEditor::EditorAssetConversionBus::BroadcastResult(assetOutcome, &ScriptCanvasEditor::EditorAssetConversionBusTraits::CreateRuntimeAsset, graphPath);

        if (assetOutcome.IsSuccess())
        {
            LoadTestGraphResult result;
            result.m_asset = assetOutcome.GetValue();
            result.m_graphEntity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
            result.m_graph = result.m_graphEntity->CreateComponent<ScriptCanvas::RuntimeComponent>(result.m_asset);
            return result;
        }

        return {};
    }

} // namespace ScriptCanvasEditor
