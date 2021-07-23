/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Editor/EditorTypes.h>

namespace PrefabDependencyViewer
{
    static const GraphCanvas::EditorId PREFAB_DEPENDENCY_VIEWER_EDITOR_ID = AZ_CRC_CE("PrefabDependencyViewerEditor");
    static constexpr inline AZStd::string_view SYSTEM_NAME = "Prefab Dependency Viewer";
    static constexpr inline AZStd::string_view MODULE_FILE_EXTENSION = ".prefabdependencyviewer";
}
