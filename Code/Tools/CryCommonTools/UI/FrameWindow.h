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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_UI_FRAMEWINDOW_H
#define CRYINCLUDE_CRYCOMMONTOOLS_UI_FRAMEWINDOW_H
#pragma once


#include <vector>
#include "Layout.h"

class IUIComponent;

class FrameWindow
{
public:
    FrameWindow();
    ~FrameWindow();
    void AddComponent(IUIComponent* component);
    void Show(bool show, int width, int height);
    void SetCaption(const TCHAR* caption);
    void* GetHWND();

private:
    void UpdateComponentUI(bool create);
    std::pair<int, int> InitializeSize();
    void CalculateExtremeDimensions(int& minWidth, int& maxWidth, int& minHeight, int& maxHeight);
    void OnSizeChanged(int width, int height);

    void* m_hwnd;
    Layout m_layout;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_UI_FRAMEWINDOW_H
