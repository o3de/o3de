/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "ViewportAlign.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! AlignToolbarSection is a part of the ModeToolbar that only shows up when in move mode.
//! It has buttons that, when clicked, perform different align operations.
class AlignToolbarSection
{
public: //methods

    explicit AlignToolbarSection();

    //! Used to grey out the buttons when the selection is not valid for align operations
    void SetIsEnabled(bool enabled);

    //! Used to hide the section when not in the correct mode
    void SetIsVisible(bool visible);

    //! Adds the buttons, called by the parent toolbar to add them in the right place as toolbar is built
    void AddButtons(QToolBar* parent);

private: // methods

    void AddButton(QToolBar* parent, ViewportAlign::AlignType alignType, const QString& iconName, const QString& toolTip);

private: // data

    QAction* m_separator = nullptr;
    AZStd::vector<QAction*> m_buttons;
};
