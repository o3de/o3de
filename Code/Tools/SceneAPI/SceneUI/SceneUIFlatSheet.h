/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

class QFrame;
class QPushButton;
class QWidget;

//! The sheets that the widgets of the scene settings set on themselves, and the elements that carry them in the flattened GUI
//! (AzQtComponents::StyleManager::stylesheetsDisabled). Each Style function is the look of one site: in the legacy GUI the sheet the site
//! sets, in the flattened GUI none, but the state that the elements read. They are declared here, not in the headers of the widgets, so
//! that the tests that call them do not need the widgets, which the library does not export.
namespace SceneUIFlatSheet
{
    //! Flattened GUI only: registers the elements, once per process. The manifest widget registers them before it builds its pages;
    //! the legacy GUI never does.
    SCENE_UI_API void RegisterElements();

    //! How many times RegisterElements was called, whether or not that call registered. A test asks it to see that a site asked for the
    //! registration, whatever was registered before the site ran.
    SCENE_UI_API int RegisterElementsCalls();

    //! The header of a group in the "display divider" style (CategoryStyle), whose sheet reaches the labels below it. The flattened GUI
    //! tags the header, so that the labels below it are resolved again if they were polished already.
    SCENE_UI_API void StyleDisplayDividerHeader(QWidget* header);

    //! The frame of the buttons of a manifest page (ManifestWidgetPage.ui, `header`)
    SCENE_UI_API void StyleManifestPageHeader(QFrame* header);

    //! The "Add another" button of a manifest page (ManifestWidgetPage.ui, `m_addButton`)
    SCENE_UI_API void StyleAddButton(QPushButton* button);

    //! The "Add Object" button of a vector widget (ManifestVectorWidget.ui, `m_addObjectButton`)
    SCENE_UI_API void StyleAddObjectButton(QPushButton* button);
} // namespace SceneUIFlatSheet
