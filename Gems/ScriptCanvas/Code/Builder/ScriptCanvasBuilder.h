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

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;
}

namespace ScriptCanvasBuilder
{
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
}
