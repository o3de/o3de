/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
