/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <EditorCoreAPI.h>

namespace Editor
{
    EDITOR_CORE_API bool GridSnappingEnabled();

    EDITOR_CORE_API float GridSnappingSize();

    EDITOR_CORE_API bool AngleSnappingEnabled();

    EDITOR_CORE_API float AngleSnappingSize();

    EDITOR_CORE_API bool ShowingGrid();

    EDITOR_CORE_API void SetGridSnapping(bool enabled);

    EDITOR_CORE_API void SetGridSnappingSize(float size);

    EDITOR_CORE_API void SetAngleSnapping(bool enabled);

    EDITOR_CORE_API void SetAngleSnappingSize(float size);

    EDITOR_CORE_API void SetShowingGrid(bool showing);
} // namespace Editor
