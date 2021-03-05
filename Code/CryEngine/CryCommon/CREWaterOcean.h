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

#ifndef _CREWATEROCEAN_
#define _CREWATEROCEAN_

class CWater;

class CREWaterOcean
    : public CRendElementBase
{
public:
    CREWaterOcean();
    virtual ~CREWaterOcean();

    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
    virtual void mfGetPlane(Plane& pl);

    virtual void Create(uint32 nVerticesCount, SVF_P3F_C4B_T2F* pVertices, uint32 nIndicesCount, const void* pIndices, uint32 nIndexSizeof);
    void ReleaseOcean();

    virtual Vec3 GetPositionAt(float x, float y) const;
    virtual Vec4* GetDisplaceGrid() const;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:

    uint32 m_nVerticesCount;
    uint32 m_nIndicesCount;
    uint32 m_nIndexSizeof;

    void* m_pVertDecl;
    void* m_pVertices;
    void* m_pIndices;

private:

    void UpdateFFT();
    void FrameUpdate();
};


#endif