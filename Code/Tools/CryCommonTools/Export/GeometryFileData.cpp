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
#include "GeometryFileData.h"

int GeometryFileData::AddGeometryFile(const void* handle, const char* name, const SProperties& properties)
{
    const int geometryFileIndex = int(m_geometryFiles.size());
    m_geometryFiles.push_back(GeometryFileEntry(handle, name, properties));
    return geometryFileIndex;
}

int GeometryFileData::GetGeometryFileCount() const
{
    return int(m_geometryFiles.size());
}

const void* GeometryFileData::GetGeometryFileHandle(int geometryFileIndex) const
{
    return m_geometryFiles[geometryFileIndex].handle;
}

const char* GeometryFileData::GetGeometryFileName(int geometryFileIndex) const
{
    return m_geometryFiles[geometryFileIndex].name.c_str();
}

//////////////////////////////////////////////////////////////////////////
const IGeometryFileData::SProperties& GeometryFileData::GetProperties(int geometryFileIndex) const
{
    if (size_t(geometryFileIndex) >= m_geometryFiles.size())
    {
        assert(0);
        static SProperties badValue;
        return badValue;
    }
    return m_geometryFiles[geometryFileIndex].properties;
}

void GeometryFileData::SetProperties(int geometryFileIndex, const IGeometryFileData::SProperties& properties)
{
    if (size_t(geometryFileIndex) >= m_geometryFiles.size())
    {
        assert(0);
        return;
    }
    m_geometryFiles[geometryFileIndex].properties = properties;
}
