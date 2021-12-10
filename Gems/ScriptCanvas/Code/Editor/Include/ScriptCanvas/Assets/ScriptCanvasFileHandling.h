/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    class ScriptCanvasData;
}

namespace ScriptCanvasEditor
{
    class EditorAssetTree
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorAssetTree, AZ::SystemAllocator, 0);

        EditorAssetTree* m_parent = nullptr;
        AZStd::vector<EditorAssetTree> m_dependencies;
        SourceHandle m_asset;

        EditorAssetTree* ModRoot();

        void SetParent(EditorAssetTree& parent);

        AZStd::string ToString(size_t depth = 0) const;
    };

    AZ::Outcome<SourceHandle, AZStd::string> LoadFromFile(AZStd::string_view path);

    AZ::Outcome<void, AZStd::string> LoadDataFromJson
        ( ScriptCanvas::ScriptCanvasData& dataTarget
        , AZStd::string_view source
        , AZ::SerializeContext& serializeContext);

    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(SourceHandle handle, EditorAssetTree* parent = nullptr);
        
    AZ::Outcome<void, AZStd::string> SaveToStream(const SourceHandle& source, AZ::IO::GenericStream& stream);
}
