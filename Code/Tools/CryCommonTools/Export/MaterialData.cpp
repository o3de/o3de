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
#include "MaterialData.h"

int MaterialData::AddMaterial(const char* name, int id, const void* handle, const char* properties)
{
    const int materialIndex = int(m_materials.size());
    m_materials.push_back(MaterialEntry(name, id, "submat", handle, properties));
    return materialIndex;
}

int MaterialData::AddMaterial(const char* name, int id, const char* subMatName, const void* handle, const char* properties)
{
    const int materialIndex = int(m_materials.size());
    m_materials.push_back(MaterialEntry(name, id, subMatName, handle, properties));
    return materialIndex;
}

int MaterialData::GetMaterialCount() const
{
    return int(m_materials.size());
}

const char* MaterialData::GetName(int materialIndex) const
{
    assert(materialIndex >= 0);
    assert(materialIndex < int(m_materials.size()));
    return m_materials[materialIndex].name.c_str();
}

int MaterialData::GetID(int materialIndex) const
{
    assert(materialIndex >= 0);
    assert(materialIndex < int(m_materials.size()));
    return m_materials[materialIndex].id;
}

const char* MaterialData::GetSubMatName(int materialIndex) const
{
    assert(materialIndex >= 0);
    assert(materialIndex < int(m_materials.size()));
    return m_materials[materialIndex].subMatName.c_str();
}

const void* MaterialData::GetHandle(int materialIndex) const
{
    assert(materialIndex >= 0);
    assert(materialIndex < int(m_materials.size()));
    return m_materials[materialIndex].handle;
}

const char* MaterialData::GetProperties(int materialIndex) const
{
    assert(materialIndex >= 0);
    assert(materialIndex < int(m_materials.size()));
    return m_materials[materialIndex].properties.c_str();
}
