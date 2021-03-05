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
#include "Spacer.h"

Spacer::Spacer(int minWidth, int minHeight, int maxWidth, int maxHeight)
    : m_minWidth(minWidth)
    , m_minHeight(minHeight)
    , m_maxWidth(maxWidth)
    , m_maxHeight(maxHeight)
{
}

void Spacer::CreateUI(void* window, int left, int top, int width, int height)
{
}

void Spacer::Resize(void* window, int left, int top, int width, int height)
{
}

void Spacer::DestroyUI(void* window)
{
}

void Spacer::GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight)
{
    minWidth = m_minWidth;
    maxWidth = m_maxWidth;
    minHeight = m_minHeight;
    maxHeight = m_maxHeight;
}
