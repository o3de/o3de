/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace ScriptCanvasIdentifiers
{
    // Main Window
    inline constexpr AZStd::string_view ScriptCanvasActionContextIdentifier = "o3de.context.editor.scriptcanvas";

    // Main Window Widgets
    inline constexpr AZStd::string_view ScriptCanvasVariablesActionContextIdentifier = "o3de.context.editor.scriptcanvas.variables";

} // namespace ScriptCanvasIdentifiers
