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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKINNINGDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKINNINGDATA_H
#pragma once


#include "ISkinningData.h"

class SkinningData
    : public ISkinningData
{
public:
    virtual void SetVertexCount(int vertexCount);
    virtual void AddWeight(int vertexIndex, int boneIndex, float weight);

    int GetVertexCount() const;
    int GetBoneLinkCount(int vertexIndex) const;
    int GetBoneIndex(int vertexIndex, int linkIndex) const;
    float GetWeight(int vertexIndex, int linkIndex) const;

private:
    struct BoneWeight
    {
        BoneWeight(int boneIndex, float weight)
            : boneIndex(boneIndex)
            , weight(weight) {}
        int boneIndex;
        float weight;
    };

    std::vector<std::vector<BoneWeight> > m_weights;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_SKINNINGDATA_H
