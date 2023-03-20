/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Script/ScriptSystemBus.h>
#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Editor/Framework/ScriptCanvasReporter.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Translation/TranslationResult.h>

namespace ScriptCanvas
{
    class RuntimeAsset;
    class RuntimeComponent;
}

namespace ScriptCanvasEditor
{
    struct LoadedInterpretedDependency
    {
        AZStd::string path;
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
        ScriptCanvas::Translation::LuaAssetResult luaAssetResult;
        AZStd::vector<LoadedInterpretedDependency> dependencies;
    };

    AZ_INLINE AZStd::vector<LoadedInterpretedDependency> LoadInterpretedDepencies(const ScriptCanvas::DependencySet& dependencySet);

    AZ_INLINE LoadTestGraphResult LoadTestGraph(AZStd::string_view path);

    struct RunSpec
    {
        DurationSpec duration;
        ScriptCanvas::ExecutionMode execution = ScriptCanvas::ExecutionMode::Interpreted;
        bool expectRuntimeFailure = false;;
        bool processOnly = false;
        bool release = true;
        bool debug = true;
        bool traced = true;
        AZStd::function<void()> m_onPostSimulate;
    };

    struct RunGraphSpec
    {
        AZStd::string_view graphPath;
        AZStd::string_view dirPath;
        RunSpec runSpec;
    };

    AZ_INLINE Reporters RunGraph(const RunGraphSpec& runGraphSpec);
    AZ_INLINE void RunEditorAsset(AZ::Data::Asset<AZ::Data::AssetData> asset, Reporter& reporter, ScriptCanvas::ExecutionMode mode);

    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, Reporter& reporter);
    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, LoadTestGraphResult& loadGraphResult, Reporter& reporter);
    AZ_INLINE void RunGraphImplementation(const RunGraphSpec& runGraphSpec, Reporters& reporters);

    AZ_INLINE void Simulate(const DurationSpec& duration);
    AZ_INLINE void SimulateDuration(const DurationSpec& duration);
    AZ_INLINE void SimulateSeconds(const DurationSpec& duration);
    AZ_INLINE void SimulateTicks(const DurationSpec& duration);
} // ScriptCanvasEditor

#include <Editor/Framework/ScriptCanvasGraphUtilities.inl>
