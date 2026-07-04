/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/EditorOnlyEntityHandler.h>

namespace Shine
{
    //! Converts a list of editor entities to runtime entities by calling BuildGameEntity
    //! on each editor component or cloning runtime-ready components. This replaces the
    //! engine's CompileEditorSlice utility without requiring any Slice types.
    //!
    //! Ported from AzToolsFramework::Prefab::PrefabConversionUtils::EditorInfoRemover.
    //!
    //! @param sourceEntities The editor entities to convert. Not modified.
    //! @param platformTags Platform tags for conditional component export.
    //! @param serializeContext Serialize context for cloning and class lookup.
    //! @param editorOnlyEntityHandlers Optional handlers for editor-only entity removal.
    //! @return On success, a vector of newly-created runtime entities (caller owns).
    //!         On failure, an error string.
    using EditorOnlyEntityHandlers = AZStd::vector<AzToolsFramework::Prefab::PrefabConversionUtils::EditorOnlyEntityHandler*>;

    AZ::Outcome<AZStd::vector<AZ::Entity*>, AZStd::string> CompileEditorEntities(
        const AZStd::vector<AZ::Entity*>& sourceEntities,
        const AZ::PlatformTagSet& platformTags,
        AZ::SerializeContext& serializeContext,
        const EditorOnlyEntityHandlers& editorOnlyEntityHandlers = EditorOnlyEntityHandlers());

} // namespace Shine
