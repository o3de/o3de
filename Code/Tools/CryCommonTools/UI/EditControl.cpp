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
#include "EditControl.h"
#include "Win32GUI.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <Richedit.h>

EditControl::EditControl()
    : m_edit(0)
{
}

void EditControl::CreateUI(void* window, int left, int top, int width, int height)
{
    m_edit = Win32GUI::CreateControl(RICHEDIT_CLASS, ES_MULTILINE /*| ES_READONLY*/, (HWND)window, left, top, width, height);
}

void EditControl::Resize(void* window, int left, int top, int width, int height)
{
    MoveWindow((HWND)m_edit, left, top, width, height, true);
}

void EditControl::DestroyUI(void* window)
{
    DestroyWindow((HWND)m_edit);
    m_edit = 0;
}

void EditControl::GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight)
{
    minWidth = 20;
    maxWidth = 2000;
    minHeight = 20;
    maxHeight = 2000;
}
