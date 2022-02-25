/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderDataSystemBus.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class EditorAssetTree;
}

namespace ScriptCanvasBuilder
{
    class DataSystem
        : public DataSystemRequestsBus::Handler
        , private AzToolsFramework::AssetSystemBus::Handler
    {
        struct BuildResult;

    public:
        AZ_TYPE_INFO(DataSystem, "{27B74209-319D-4A8C-B37D-F85EFA6D2FFA}");
        AZ_CLASS_ALLOCATOR(DataSystem, AZ::SystemAllocator, 0);

    protected:
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;

    private:
        struct BuildResult
        {
            BuilderDataStatus status;
            ScriptCanvasBuilder::BuildVariableOverrides data;
        };

        AZStd::unordered_map<ScriptCanvasEditor::SourceHandle, BuildResult> m_buildResultsByHandle;
    };
}
