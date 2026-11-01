/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AssetProcessor
{
    //! The flattened GUI (AzQtComponents::StyleManager::stylesheetsDisabled) carries AssetProcessor.qss and the sheets it imports
    //! (AssetsTab.qss, LogsTab.qss; NewLogTabDialog.qss, AzToolsFramework's, only says what '*' says) as elements of the
    //! AzQtComponents registries: text, frame and box elements, each the final look of the widgets its matcher picks.
    //! StyleManager::registerFlatSheet runs this once, the first time the sheet is set on a widget; the legacy GUI never calls it.
    void RegisterFlatElements();
} // namespace AssetProcessor
