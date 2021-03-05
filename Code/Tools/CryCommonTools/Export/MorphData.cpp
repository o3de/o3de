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
#include "MorphData.h"

MorphData::MorphData()
    : m_handle(0)
{
}

void MorphData::SetHandle(const void* handle)
{
    m_handle = handle;
}

void MorphData::AddMorph(const void* handle, const char* name, const char* fullname)
{
    m_morphs.push_back(Entry(handle, name, fullname ? fullname : ""));
}

const void* MorphData::GetHandle() const
{
    return m_handle;
}

int MorphData::GetMorphCount() const
{
    return int(m_morphs.size());
}

std::string MorphData::GetMorphName(int morphIndex) const
{
    return m_morphs[morphIndex].name;
}

std::string MorphData::GetMorphFullName(int morphIndex) const
{
    return m_morphs[morphIndex].fullname.length() > 0 ? m_morphs[morphIndex].fullname : m_morphs[morphIndex].name;
}

const void* MorphData::GetMorphHandle(int morphIndex) const
{
    return m_morphs[morphIndex].handle;
}
