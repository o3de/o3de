/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/EditorAssetSystemComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <Editor/Framework/ScriptCanvasReporter.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>

#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/UnitTesting/UnitTestBusSender.h>

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;

    // The runtime context (appropriately) always assumes that EntityIds are overridden, this step copies the values from the runtime data
    // over to the override data to simulate build step that does this when building prefabs
    AZ_INLINE void CopyAssetEntityIdsToOverrides(RuntimeDataOverrides& runtimeDataOverrides)
    {
        runtimeDataOverrides.m_entityIds.reserve(runtimeDataOverrides.m_runtimeAsset->m_runtimeData.m_input.m_entityIds.size());
        for (auto& varEntityPar : runtimeDataOverrides.m_runtimeAsset->m_runtimeData.m_input.m_entityIds)
        {
            runtimeDataOverrides.m_entityIds.push_back(varEntityPar.second);
        }

        for (auto& dependency : runtimeDataOverrides.m_dependencies)
        {
            CopyAssetEntityIdsToOverrides(dependency);
        }
    }

    AZ_INLINE AZStd::vector<LoadedInterpretedDependency> LoadInterpretedDepencies(const ScriptCanvas::DependencySet& dependencySet)
    {
        AZStd::vector<LoadedInterpretedDependency> loadedAssets;

        if (!dependencySet.empty())
        {
            for (auto& namespacePath : dependencySet)
            {
                if (namespacePath.empty())
                {
                    continue;
                }

                AZ_Assert(namespacePath.size() >= 3, "This functions assumes unit test dependencies are in the ScriptCanvas gem unit test folder");
                AZStd::string originalPath = namespacePath[2].data();

                for (size_t index = 3; index < namespacePath.size(); ++index)
                {
                    originalPath += "/";
                    originalPath += namespacePath[index];
                }

                if (originalPath.ends_with(Grammar::k_internalRuntimeSuffix) || originalPath.ends_with(Grammar::k_internalRuntimeSuffixLC))
                {
                    originalPath.resize(originalPath.size() - AZStd::string_view(Grammar::k_internalRuntimeSuffix).size());
                }

                AZStd::string path = AZStd::string::format("%s/%s.scriptcanvas", k_unitTestDirPathRelative, originalPath.data());
                LoadTestGraphResult loadResult = LoadTestGraph(path);
                AZ_Assert(loadResult.m_runtimeAsset, "failed to load dependent asset");

                AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> luaAssetOutcome = AZ::Failure(AZStd::string("lua asset creation for function failed"));
                ScriptCanvasEditor::EditorAssetConversionBus::BroadcastResult(luaAssetOutcome, &ScriptCanvasEditor::EditorAssetConversionBusTraits::CreateLuaAsset, loadResult.m_editorAsset, loadResult.m_editorAsset.RelativePath().c_str());
                AZ_Assert(luaAssetOutcome.IsSuccess(), "failed to create Lua asset");

                AZStd::string modulePath = namespacePath[0].data();
                for (size_t index = 1; index < namespacePath.size(); ++index)
                {
                    modulePath += "/";
                    modulePath += namespacePath[index];
                }

                const ScriptCanvas::Translation::LuaAssetResult& luaAssetResult = luaAssetOutcome.GetValue();
                // #functions2_recursive_unit_tests
                loadedAssets.push_back({ modulePath, loadResult.m_runtimeAsset, luaAssetResult, {} });
            }
        }

        return loadedAssets;
    }

    AZ_INLINE DurationSpec DurationSpec::Seconds(float seconds)
    {
        DurationSpec spec;
        spec.m_spec = eDuration::Seconds;
        spec.m_seconds = seconds;
        return spec;
    }

    AZ_INLINE DurationSpec DurationSpec::Ticks(size_t ticks)
    {
        DurationSpec spec;
        spec.m_spec = eDuration::Ticks;
        spec.m_ticks = ticks;
        return spec;
    }

    AZ_INLINE LoadTestGraphResult LoadTestGraph(AZStd::string_view graphPath)
    {
        if (auto fileLoadResult = LoadFromFile(graphPath))
        {
            auto& source = fileLoadResult.m_handle;
            auto testableSource = SourceHandle::FromRelativePath(source, AZ::Uuid::CreateRandom(), source.RelativePath().c_str());

            AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> assetOutcome(AZ::Failure(AZStd::string("asset create failed")));
            ScriptCanvasEditor::EditorAssetConversionBus::BroadcastResult(assetOutcome
                , &ScriptCanvasEditor::EditorAssetConversionBusTraits::CreateRuntimeAsset, testableSource);

            if (assetOutcome.IsSuccess())
            {
                LoadTestGraphResult loadTestGraphResult;
                loadTestGraphResult.m_editorAsset = AZStd::move(testableSource);
                loadTestGraphResult.m_runtimeAsset = assetOutcome.GetValue();
                loadTestGraphResult.m_entity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
                return loadTestGraphResult;
            }
        }

        return {};
    }

    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, Reporters& reporters)
    {
        AZ_Assert(!reporters.empty(), "there must be at least one report");

        for (auto& reporter : reporters)
        {
            if (runGraphSpec.runSpec.expectRuntimeFailure)
            {
                reporter.MarkExpectRuntimeFailure();
            }

            reporter.SetExecutionMode(runGraphSpec.runSpec.execution);
            RunGraphImplementation(runGraphSpec, reporter);
        }
    }

    AZ_INLINE void RunEditorAsset(SourceHandle asset, Reporter& reporter, ScriptCanvas::ExecutionMode mode)
    {
        AZ::Data::AssetId assetId = asset.Id();
        AZ::Data::AssetId runtimeAssetId(assetId.m_guid, RuntimeDataSubId);
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        if (!runtimeAsset.Create(runtimeAssetId, true))
        {
            return;
        }

        reporter.SetExecutionMode(mode);

        LoadTestGraphResult loadResult;
        loadResult.m_editorAsset = SourceHandle::FromRelativePath(nullptr, assetId.m_guid, asset.RelativePath());
        AZ::EntityId scriptCanvasId;
        loadResult.m_entity = AZStd::make_unique<AZ::Entity>("Loaded test graph");
        loadResult.m_runtimeAsset = runtimeAsset;

        RunGraphSpec runGraphSpec;
        runGraphSpec.dirPath = "";
        runGraphSpec.graphPath = asset.RelativePath().c_str();
        runGraphSpec.runSpec.duration.m_spec = eDuration::Ticks;
        runGraphSpec.runSpec.duration.m_ticks = 10;
        runGraphSpec.runSpec.execution = mode;
        runGraphSpec.runSpec.release = true;
        runGraphSpec.runSpec.debug = runGraphSpec.runSpec.traced = false;
        RunGraphImplementation(runGraphSpec, loadResult, reporter);
    }

    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, Reporter& reporter)
    {
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, true);
        LoadTestGraphResult loadResult = LoadTestGraph(runGraphSpec.graphPath);
        TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressPrintf, false);
        RunGraphImplementation(runGraphSpec, loadResult, reporter);
    }

    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, LoadTestGraphResult& loadResult, Reporter& reporter)
    {
        ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::MarkScriptUnitTestBegin);

        if (loadResult.m_entity)
        {
            reporter.MarkGraphLoaded();

            RuntimeData runtimeDataBuffer;
            AZStd::vector<RuntimeData> dependencyDataBuffer;
            AZStd::vector<LoadedInterpretedDependency> dependencies;

            if (runGraphSpec.runSpec.execution == ExecutionMode::Interpreted)
            {
                ScopedOutputSuppression outputSuppressor;
                AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> luaAssetOutcome = AZ::Failure(AZStd::string("lua asset creation failed"));
                ScriptCanvasEditor::EditorAssetConversionBus::BroadcastResult(luaAssetOutcome
                    , &ScriptCanvasEditor::EditorAssetConversionBusTraits::CreateLuaAsset, loadResult.m_editorAsset, loadResult.m_editorAsset.RelativePath().c_str());
                reporter.MarkParseAttemptMade();

                if (luaAssetOutcome.IsSuccess())
                {
                    const ScriptCanvas::Translation::LuaAssetResult& luaAssetResult = luaAssetOutcome.GetValue();
                    reporter.SetEntity(loadResult.m_entity->GetId());
                    reporter.SetDurations(luaAssetResult.m_parseDuration, luaAssetResult.m_translationDuration);
                    reporter.MarkCompiled();

                    if (!reporter.IsProcessOnly())
                    {
                        RuntimeDataOverrides runtimeDataOverrides;
                        runtimeDataOverrides.m_runtimeAsset = loadResult.m_runtimeAsset;
                        runtimeDataOverrides.m_runtimeAsset.SetHint("original");
                        runtimeDataOverrides.m_runtimeAsset.Get()->m_runtimeData.m_script.SetHint("original");

#if defined(LINUX) //////////////////////////////////////////////////////////////////////////
                        if (!luaAssetResult.m_dependencies.source.userSubgraphs.empty())
                        {
                            reporter.MarkLinuxDependencyTestBypass();
                            ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::MarkScriptUnitTestEnd);
                            return;
                        }
#else ///////////////////////////////////////////////////////////////////////////////////////

                        dependencies = LoadInterpretedDepencies(luaAssetResult.m_dependencies.source.userSubgraphs);

                        if (!dependencies.empty())
                        {
                            // #functions2_recursive_unit_tests eventually, this will need to be recursive, or the full asset handling system will need to be integrated into the testing framework
                            // in order to test functionality with a dependency stack greater than 2

                            // load all script assets, and their dependencies, initialize statics on all those dependencies if it is the first time loaded
                            {
                                AZ::InMemoryScriptModules inMemoryModules;
                                inMemoryModules.reserve(dependencies.size());
                                dependencyDataBuffer.resize(dependencies.size());

                                for (auto& dependency : dependencies)
                                {
                                    inMemoryModules.emplace_back(dependency.path, dependency.luaAssetResult.m_scriptAsset);
                                }

                                AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::UseInMemoryRequireHook, inMemoryModules, AZ::ScriptContextIds::DefaultScriptContextId);
                            }

                            for (size_t index = 0; index < dependencies.size(); ++index)
                            {
                                auto& dependency = dependencies[index];
                                const ScriptCanvas::Translation::LuaAssetResult& depencyAssetResult = dependency.luaAssetResult;

                                RuntimeDataOverrides dependencyRuntimeDataOverrides;
                                dependencyRuntimeDataOverrides.m_runtimeAsset = dependency.runtimeAsset;
                                AZStd::string dependencyHint = AZStd::string::format("dependency_%zu", index);
                                dependencyRuntimeDataOverrides.m_runtimeAsset.SetHint(dependencyHint);
                                dependencyRuntimeDataOverrides.m_runtimeAsset.Get()->m_runtimeData.m_script.SetHint(dependencyHint);

                                runtimeDataOverrides.m_dependencies.push_back(dependencyRuntimeDataOverrides);

                                RuntimeData& dependencyData = dependencyDataBuffer[index];
                                dependencyData.m_input = depencyAssetResult.m_runtimeInputs;
                                dependencyData.m_debugMap = depencyAssetResult.m_debugMap;
                                dependencyData.m_script = depencyAssetResult.m_scriptAsset;
                                Execution::Context::InitializeStaticActivationData(dependencyData);
                                Execution::InitializeInterpretedStatics(dependencyData);
                            }
                        }
#endif //////////////////////////////////////////////////////////////////////////////////////

                        loadResult.m_scriptAsset = luaAssetResult.m_scriptAsset;
                        loadResult.m_runtimeAsset.Get()->m_runtimeData.m_script = loadResult.m_scriptAsset;
                        loadResult.m_runtimeAsset.Get()->m_runtimeData.m_input = luaAssetResult.m_runtimeInputs;
                        loadResult.m_runtimeAsset.Get()->m_runtimeData.m_debugMap = luaAssetResult.m_debugMap;
                        loadResult.m_runtimeComponent = loadResult.m_entity->CreateComponent<ScriptCanvas::RuntimeComponent>();
                        CopyAssetEntityIdsToOverrides(runtimeDataOverrides);
                        loadResult.m_runtimeComponent->TakeRuntimeDataOverrides(AZStd::move(runtimeDataOverrides));
                        Execution::Context::InitializeStaticActivationData(loadResult.m_runtimeAsset->m_runtimeData);
                        Execution::InitializeInterpretedStatics(loadResult.m_runtimeAsset->m_runtimeData);
                    }
                }
            }

            if (reporter.IsCompiled())
            {
                if (reporter.IsProcessOnly())
                {
                    reporter.FinishReport();
                }
                else
                {
                    loadResult.m_entity->Init();
                    reporter.SetGraph(loadResult.m_runtimeAsset.GetId());

                    {
                        ScopedOutputSuppression outputSuppressor;

                        if (runGraphSpec.runSpec.execution == ExecutionMode::Interpreted)
                        {
                            // make sure the functions have debug info, too
                            if (reporter.GetExecutionConfiguration() == ExecutionConfiguration::Release)
                            {
                                ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::SetInterpretedBuildConfiguration, ScriptCanvas::BuildConfiguration::Release);
                            }
                            else if (reporter.GetExecutionConfiguration() == ExecutionConfiguration::Performance)
                            {
                                ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::SetInterpretedBuildConfiguration, ScriptCanvas::BuildConfiguration::Performance);
                            }
                            else
                            {
                                ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::SetInterpretedBuildConfiguration, ScriptCanvas::BuildConfiguration::Debug);
                            }
                        }

                        loadResult.m_entity->Activate();
                        SimulateDuration(runGraphSpec.runSpec.duration);
                    }

                    if (runGraphSpec.runSpec.m_onPostSimulate)
                    {
                        AZStd::invoke(runGraphSpec.runSpec.m_onPostSimulate);
                    }

                    loadResult.m_entity->Deactivate();
                    reporter.CollectPerformanceTiming();
                    reporter.FinishReport();
                    loadResult.m_entity.reset();
                }
            }

            if (runGraphSpec.runSpec.execution == ExecutionMode::Interpreted)
            {
                AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::ClearAssetReferences, loadResult.m_scriptAsset.GetId());

                if (!dependencies.empty())
                {
                    AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::RestoreDefaultRequireHook, AZ::ScriptContextIds::DefaultScriptContextId);
                }

                AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);
            }
        }

        if (!reporter.IsReportFinished())
        {
            reporter.FinishReport();
        }

        ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::MarkScriptUnitTestEnd);
    }

    AZ_INLINE Reporters RunGraph(const RunGraphSpec& runGraphSpec)
    {
        Reporters reporters;

        if (runGraphSpec.runSpec.processOnly)
        {
            Reporter reporter;
            reporter.SetFilePath(runGraphSpec.graphPath);
            reporter.SetProcessOnly(runGraphSpec.runSpec.processOnly);
            reporters.push_back(reporter);
        }
        else
        {
            if (runGraphSpec.runSpec.release)
            {
                Reporter reporterRelease;
                reporterRelease.SetFilePath(runGraphSpec.graphPath);
                reporterRelease.SetExecutionConfiguration(ExecutionConfiguration::Release);
                reporters.push_back(reporterRelease);

                Reporter reporterPeformance;
                reporterPeformance.SetFilePath(runGraphSpec.graphPath);
                reporterPeformance.SetExecutionConfiguration(ExecutionConfiguration::Performance);
                reporters.push_back(reporterPeformance);
            }

            if (runGraphSpec.runSpec.debug)
            {
                Reporter reporterDebug;
                reporterDebug.SetFilePath(runGraphSpec.graphPath);
                reporterDebug.SetExecutionConfiguration(ExecutionConfiguration::Debug);
                reporters.push_back(reporterDebug);
            }

            if (runGraphSpec.runSpec.traced)
            {
                Reporter reporterTraced;
                reporterTraced.SetFilePath(runGraphSpec.graphPath);
                reporterTraced.SetExecutionConfiguration(ExecutionConfiguration::Traced);
                reporters.push_back(reporterTraced);
            }
        }

        RunGraphImplementation(runGraphSpec, reporters);
        return reporters;
    }

    AZ_INLINE void Simulate(const DurationSpec& duration)
    {
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
        AZ::SystemTickBus::ExecuteQueuedEvents();

        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, duration.m_timeStep, AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        AZ::TickBus::ExecuteQueuedEvents();
    }

    AZ_INLINE void SimulateDuration(const DurationSpec& duration)
    {
        switch (duration.m_spec)
        {
        case eDuration::InitialActivation:
            break;
        case eDuration::Seconds:
            SimulateSeconds(duration);
            break;
        case eDuration::Ticks:
            SimulateTicks(duration);
            break;
        default:
            break;
        }
    }

    AZ_INLINE void SimulateSeconds(const DurationSpec& duration)
    {
        float simulationDuration = duration.m_seconds;

        while (simulationDuration > 0.0f)
        {
            Simulate(duration);
            simulationDuration -= duration.m_timeStep;
        }
    }

    AZ_INLINE void SimulateTicks(const DurationSpec& duration)
    {
        size_t remainingTicks = duration.m_ticks;

        while (remainingTicks)
        {
            Simulate(duration);
            --remainingTicks;
        }
    }
}
