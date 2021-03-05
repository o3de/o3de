/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
