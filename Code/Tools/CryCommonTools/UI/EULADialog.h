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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_UI_EULADIALOG_H
#define CRYINCLUDE_CRYCOMMONTOOLS_UI_EULADIALOG_H
#pragma once


#include "FrameWindow.h"
#include "EditControl.h"
#include "Spacer.h"
#include "Layout.h"
#include "PushButton.h"

class EULADialog
{
public:
    enum UserResponse
    {
        UserResponseNone,
        UserResponseCancel,
        UserResponseAccept
    };

    static UserResponse Show(int width, int height, TCHAR* resourceID);

private:
    EULADialog();

    UserResponse Run(int width, int height, TCHAR* resourceID);

    void CancelPressed();
    void AcceptPressed();

    FrameWindow m_frameWindow;
    PushButton m_cancelButton;
    Spacer m_buttonSpacer;
    PushButton m_acceptButton;
    Layout m_buttonLayout;
    EditControl m_edit;

    UserResponse m_userResponse;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_UI_EULADIALOG_H
