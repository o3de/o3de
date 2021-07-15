/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Variable/VariableCore.h>

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

        // use this to preserve old values that may have been overridden on the instance, and are still valid in the parsed graph
        void CopyPreviousOverriddenValues(const BuildVariableOverrides& source);

        bool IsEmpty() const;

        // use this to initialize the new data, and make sure they have a editor graph variable for proper editor display
        void PopulateFromParsedResults(const ScriptCanvas::Grammar::ParsedRuntimeInputs& inputs, const ScriptCanvas::VariableData& variables);

        // #functions2 provide an identifier for the node/variable in the source that caused the dependency. the root will not have one.
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> m_source;

        // all of the variables here are overrides
        AZStd::vector<ScriptCanvas::GraphVariable> m_variables;
        // the values here may or may not be overrides
        AZStd::vector<AZStd::pair<ScriptCanvas::VariableId, AZ::EntityId>> m_entityIds;
        // this is all that gets exposed to the edit context
        AZStd::vector<ScriptCanvas::GraphVariable> m_overrides;
        // AZStd::vector<size_t> m_entityIdRuntimeInputIndices; since all of the entity ids need to go in, they may not need indices
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

    // copy the variables overridden during editor / prefab build time back to runtime data
    ScriptCanvas::RuntimeDataOverrides ConvertToRuntime(const BuildVariableOverrides& overrides);

    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(AZ::Data::AssetId editorAssetId, AZStd::string_view assetHint, EditorAssetTree* parent = nullptr);

    AZ::Outcome<BuildVariableOverrides, AZStd::string> ParseEditorAssetTree(const EditorAssetTree& editorAssetTree);
}
