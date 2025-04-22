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
#include <ScriptCanvas/Core/GraphSerialization.h>

// this class is for managing scriptcanvas data through file IO, it does not define, but it uses serialization utilities

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    class ScriptCanvasData;

    AZ::Outcome<SourceTree, AZStd::string> LoadEditorAssetTree(
        SourceHandle handle, MakeInternalGraphEntitiesUnique makeUniqueEntities);
    
    struct FileLoadResult
    {
        bool m_isSuccess;
        SourceHandle m_handle;
        DeserializeResult m_deserializeResult;
        AZStd::string m_fileReadErrors;

        operator bool() const
        {
            return m_isSuccess && m_deserializeResult.m_isSuccessful;
        }

        AZStd::string ToString() const;
    };

    /**
    * Loads the script canvas file at the given path.
    * @param path Path to the file to load
    * @param makeEntityIdsUnique controls if the entity IDs are re-generated for the graph to make them unique.
    *   Set to true if there's a chance the graph may be loaded multiple times, so that buses can be used safely with those IDs.
    *   Set to false when doing operations that rely on stable entity ID order between runs.
    * @param loadReferencedAssets controls whether referenced assets in the graph are loaded or not. In practice, this controls whether or not the
    * graph and underlying nodes are fully activated.
    * @return An FileLoadResult with either the handle to the data loaded and a string with deserialization issues, or a failure if the file did not load.
    */
    FileLoadResult LoadFromFile
        ( AZStd::string_view path
        , MakeInternalGraphEntitiesUnique makeEntityIdsUnique = MakeInternalGraphEntitiesUnique::Yes
        , LoadReferencedAssets loadReferencedAssets = LoadReferencedAssets::Yes);
}
