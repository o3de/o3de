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
#include "ToggleButton.h"
#include "Win32GUI.h"

ToggleButton::~ToggleButton()
{
    m_callback->Release();
}

void ToggleButton::SetState(bool state)
{
    m_state = state;
    SendMessage((HWND)m_button, BM_SETCHECK, m_state, 0);
}

void ToggleButton::CreateUI(void* window, int left, int top, int width, int height)
{
    m_button = Win32GUI::CreateControl(_T("BUTTON"), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_NOTIFY | BS_PUSHLIKE, (HWND)window, left, top, 40, 20);
    m_font = Win32GUI::CreateFont();
    SendMessage((HWND)m_button, WM_SETFONT, (WPARAM)m_font, 0);
    SendMessage((HWND)m_button, WM_SETTEXT, 0, (LPARAM)m_text.c_str());
    SendMessage((HWND)m_button, BM_SETCHECK, m_state, 0);

    Win32GUI::SetCallback<Win32GUI::EventCallbacks::Checked, ToggleButton>((HWND)m_button, this, &ToggleButton::OnChecked);
}

void ToggleButton::Resize(void* window, int left, int top, int width, int height)
{
    MoveWindow((HWND)m_button, left, top, width, height, true);
}

void ToggleButton::DestroyUI(void* window)
{
    DestroyWindow((HWND)m_button);
    m_button = 0;
    DeleteObject(m_font);
    m_font = 0;
}

void ToggleButton::GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight)
{
    minWidth = 50;
    maxWidth = 50;
    minHeight = 20;
    maxHeight = 20;
}

void ToggleButton::OnChecked(bool checked)
{
    m_state = checked;
    m_callback->Call(checked);
}
