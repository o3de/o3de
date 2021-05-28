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

#include <SandboxAPI.h>

namespace SandboxEditor
{
    SANDBOX_API bool GridSnappingEnabled();

    SANDBOX_API float GridSnappingSize();

    SANDBOX_API bool AngleSnappingEnabled();

    SANDBOX_API float AngleSnappingSize();

    SANDBOX_API bool ShowingGrid();

    SANDBOX_API void SetGridSnapping(bool enabled);

    SANDBOX_API void SetGridSnappingSize(float size);

    SANDBOX_API void SetAngleSnapping(bool enabled);

    SANDBOX_API void SetAngleSnappingSize(float size);

    SANDBOX_API void SetShowingGrid(bool showing);

    //! Return if the new editor camera system is enabled or not.
    //! @note This is implemented in EditorViewportWidget.cpp
    SANDBOX_API bool UsingNewCameraSystem();
} // namespace SandboxEditor
