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
#include "SkinningData.h"

void SkinningData::SetVertexCount(int vertexCount)
{
    m_weights.resize(vertexCount);
}

void SkinningData::AddWeight(int vertexIndex, int boneIndex, float weight)
{
    m_weights[vertexIndex].push_back(BoneWeight(boneIndex, weight));
}

int SkinningData::GetVertexCount() const
{
    return int(m_weights.size());
}

int SkinningData::GetBoneLinkCount(int vertexIndex) const
{
    return int(m_weights[vertexIndex].size());
}

int SkinningData::GetBoneIndex(int vertexIndex, int linkIndex) const
{
    return m_weights[vertexIndex][linkIndex].boneIndex;
}

float SkinningData::GetWeight(int vertexIndex, int linkIndex) const
{
    return m_weights[vertexIndex][linkIndex].weight;
}
