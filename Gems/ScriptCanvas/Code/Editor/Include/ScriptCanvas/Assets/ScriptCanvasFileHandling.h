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
#include <ScriptCanvas/Core/Core.h>

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

    // both success and failure return JSON deserialization errors, regardless of parsing / loading success
    AZ::Outcome<AZStd::string, AZStd::string> LoadDataFromJson
        ( ScriptCanvas::ScriptCanvasData& dataTarget
        , AZStd::string_view source
        , AZ::SerializeContext& serializeContext);

    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(SourceHandle handle, EditorAssetTree* parent = nullptr);

    struct FileLoadSuccess
    {
        SourceHandle handle;
        AZStd::string deserializationErrors;
    };

    /**
    * Loads the script canvas file at the given path.
    * @param path Path to the file to load
    * @param makeEntityIdsUnique controls if the entity IDs are re-generated for the graph to make them unique.
    *   Set to true if there's a chance the graph may be loaded multiple times, so that buses can be used safely with those IDs.
    *   Set to false when doing operations that rely on stable entity ID order between runs.
    * @return An outcome with either the handle to the data loaded and a string with deserialization issues, or a failure if the file did not load.
    */
    AZ::Outcome<FileLoadSuccess, AZStd::string> LoadFromFile(AZStd::string_view path, bool makeEntityIdsUnique = true);

    AZ::Outcome<void, AZStd::string> SaveToStream(const SourceHandle& source, AZ::IO::GenericStream& stream);
}
