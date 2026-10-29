/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework::EntityOutliner
{
    //! Flattened GUI only (AzQtComponents::StyleManager::stylesheetsDisabled): the elements that carry what EntityOutliner.qss gave
    //! the widgets of the Entity Outliner. EntityOutlinerWidget registers this function for the sheet
    //! (StyleManager::registerFlatSheet) before it sets the sheet, and the flattened GUI runs it when the sheet is first set;
    //! legacy mode never does.
    AZTF_API void RegisterFlatSheetElements();
} // namespace AzToolsFramework::EntityOutliner
