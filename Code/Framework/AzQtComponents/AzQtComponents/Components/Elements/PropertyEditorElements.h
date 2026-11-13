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
    //! help button and the context menu button; the two icons have another box inside a color picker) and to the notifications
    //! of a card (the block, the pill with the icon and the message, and their labels). Each is a BoxElement
    //! with the values the legacy GUI finally rendered for the widgets it covers; PropertyEditorElements.cpp names the
    //! declaration that grants every one.
    //!
    //! Two more elements follow the rules of a card header that reach widgets no rule of their own names. The frame below a header
    //! that names none (the background frame of a header that clears the name, Card.qss:86-89) is a BoxElement of its fill alone.
    //! The tip shown for a header that has modified content, or for a widget in it (Card.qss:172-175), is a ToolTipElement with a
    //! TextElement, which gives the tip the orange text of the rule and restates the font size it keeps from ToolTip.qss.
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
