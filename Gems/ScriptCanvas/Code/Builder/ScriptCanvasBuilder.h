/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        struct ParsedRuntimeInputs;
    }
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;
}

namespace ScriptCanvasBuilder
{
    class BuildVariableOverrides
    {
    public:
        AZ_TYPE_INFO(BuildVariableOverrides, "{8336D44C-8EDC-4C28-AEB4-3420D5FD5AE2}");
        AZ_CLASS_ALLOCATOR(BuildVariableOverrides, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        void Clear();

        // use this to perserve old values that may have been overridden on the instance, and are still valid in the parsed graph
        void CopyValues(const BuildVariableOverrides& source);

        // use this to initialize the new data, and make sure they have a editor graph variable for proper editor display
        void PopulateFromParsedResults(const ScriptCanvas::Grammar::ParsedRuntimeInputs& inputs, ScriptCanvas::VariableData& variables);

        bool IsEmpty() const;

        // #functions2 provide an identifier for the node/variable in the source that caused the dependency. the root will not have one.
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> m_source;
        AZStd::vector<ScriptCanvas::GraphVariable> m_variables;
        AZStd::vector<ScriptCanvas::GraphVariable> m_entityIds;
        AZStd::vector<BuildVariableOverrides> m_dependencies;
    };

    class EditorAssetTree
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorAssetTree, AZ::SystemAllocator, 0);

        EditorAssetTree* m_parent = nullptr;
        AZStd::vector<EditorAssetTree> m_dependencies;
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> m_asset;

        EditorAssetTree* ModRoot();

        void SetParent(EditorAssetTree& parent);

        AZStd::string ToString(size_t depth = 0) const;
    };

    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(AZ::Data::AssetId editorAssetId, AZStd::string_view assetHint, EditorAssetTree* parent = nullptr);

    AZ::Outcome<BuildVariableOverrides, AZStd::string> ParseEditorAssetTree(const EditorAssetTree& editorAssetTree);

    ScriptCanvas::RuntimeDataOverrides ConvertToRuntime(const BuildVariableOverrides& overrides);
}
