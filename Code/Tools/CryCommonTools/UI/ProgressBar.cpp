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

#include "StdAfx.h"
#include "ProgressBar.h"
#include <Windows.h>
#include <CommCtrl.h>

ProgressBar::ProgressBar()
    : m_progressBar(0)
{
}

void ProgressBar::CreateUI(void* window, int left, int top, int width, int height)
{
    m_progressBar = CreateWindowEx(
            0,
            PROGRESS_CLASS,
            0,
            WS_CHILD | WS_VISIBLE,
            left,
            top,
            width,
            height,
            (HWND)window,
            (HMENU)0,
            GetModuleHandle(0),
            0);
    SendMessage((HWND)m_progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
    SendMessage((HWND)m_progressBar, PBM_SETSTEP, (WPARAM) 1, 0);
}

void ProgressBar::Resize(void* window, int left, int top, int width, int height)
{
    MoveWindow((HWND)m_progressBar, left, top, width, height, true);
}

void ProgressBar::DestroyUI(void* window)
{
    DestroyWindow((HWND)m_progressBar);
}

void ProgressBar::GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight)
{
    minWidth = 200;
    maxWidth = 2000;
    minHeight = 30;
    maxHeight = 30;
}

void ProgressBar::SetProgress(float progress)
{
    int newPos = int(progress * 1000.0f);
    SendMessage((HWND)m_progressBar, PBM_SETPOS, newPos, 0);
}
