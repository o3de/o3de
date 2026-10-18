/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SandboxAPI.h>

namespace EditorFlatSheet
{
    //! Flattened GUI only (AzQtComponents::StyleManager::stylesheetsDisabled): the elements that carry what Editor.qss gave the
    //! widgets of the Editor. CryEdit.cpp registers this function for the sheet (StyleManager::registerFlatSheet) before it sets
    //! the sheet on the main window, and the flattened GUI runs it when the sheet is first set; legacy mode never does.
    SANDBOX_API void RegisterElements();
} // namespace EditorFlatSheet
