/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

namespace AzQtComponents
{
    //! The Global box elements of the property editor in the flattened GUI: the boxes ReflectedPropertyEditor.qss gave the
    //! labels of a property row, and Card.qss gave the parts of a card header (the expander, the icon, the warning icon, the
    //! help button and the context menu button; the two icons have another box inside a color picker). Each is a BoxElement
    //! with the values the legacy GUI finally rendered for the widgets it covers; PropertyEditorElements.cpp names the
    //! declaration that grants every one.
    //!
    //! The elements are registered with BoxElement as Global ones, so a module's own element for the same widget (the Editor
    //! preferences give the labels of a property row a padding) is checked first. Of the Global rows the first registered
    //! wins: register them before any Global row that covers every QLabel, QPushButton or QCheckBox.
    namespace PropertyEditorElements
    {
        //! Registers the elements. From Style::polish(QApplication*), in the flattened GUI only; a call in the legacy GUI
        //! does nothing, and so does every call after the first.
        AZ_QT_COMPONENTS_API void registerGlobal();
    } // namespace PropertyEditorElements
} // namespace AzQtComponents
