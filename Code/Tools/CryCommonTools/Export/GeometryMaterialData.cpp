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
#include "GeometryMaterialData.h"

void GeometryMaterialData::AddUsedMaterialIndex(int materialIndex)
{
    std::map<int, int>::iterator usedMaterialPos = m_usedMaterialIndexIndexMap.find(materialIndex);
    if (usedMaterialPos == m_usedMaterialIndexIndexMap.end())
    {
        int materialIndexIndex = int(m_usedMaterialIndices.size());
        m_usedMaterialIndices.push_back(materialIndex);
        m_usedMaterialIndexIndexMap.insert(std::make_pair(materialIndex, materialIndexIndex));
    }
}

int GeometryMaterialData::GetUsedMaterialCount() const
{
    return int(m_usedMaterialIndices.size());
}

int GeometryMaterialData::GetUsedMaterialIndex(int usedMaterialIndex) const
{
    return m_usedMaterialIndices[usedMaterialIndex];
}
