/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace O3DE::ProjectManager
{
    //! Registers the elements that carry ProjectManager.qss in the flattened GUI (AzQtComponents' frame, box, text and tab
    //! elements). StyleManager runs it when the sheet is first set on a widget (StyleManager::registerFlatSheet); calling
    //! it again does nothing.
    void RegisterFlatElements();
} // namespace O3DE::ProjectManager
