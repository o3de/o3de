/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SandboxAPI.h>

namespace AssetImporterFlatSheet
{
    //! Flattened GUI only (AzQtComponents::StyleManager::stylesheetsDisabled): the element that carries what AssetImporterWindow.qss
    //! gave the check boxes of the trees below the importer's window, the sheet the window sets on itself in the legacy GUI. The
    //! window registers it once before it builds its widgets; legacy mode never does. Declared here, not in AssetImporterWindow.h,
    //! so that the tests that call it do not need the scene settings headers of the window.
    SANDBOX_API void RegisterElements();
} // namespace AssetImporterFlatSheet
