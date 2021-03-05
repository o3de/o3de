/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

class CREMeshImpl
    : public CREMesh
{
#if !defined(NULL_RENDERER)
public:
    // Constant buffer used for tessellation. It has just one constant which tells the hull shader how it needs to offset iPrimitiveID that comes from HW.
    WrappedDX11Buffer m_tessCB;
#endif

public:
    virtual struct CRenderChunk* mfGetMatInfo();
    virtual TRenderChunkArray* mfGetMatInfoList();
    virtual int mfGetMatId();
    virtual bool mfPreDraw(SShaderPass* sl);
    virtual bool mfIsHWSkinned()
    {
        return (m_Flags & FCEF_SKINNED) != 0;
    }
    virtual void mfGetPlane(Plane& pl);
    virtual void mfPrepare(bool bCheckOverflow);
    virtual void mfReset();
    virtual void mfCenter(Vec3& Pos, CRenderObject* pObj);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
    virtual void* mfGetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags);
    virtual bool mfUpdate(int Flags, bool bTessellation = false);
    virtual void mfGetBBox(Vec3& vMins, Vec3& vMaxs);
    virtual void mfPrecache(const SShaderItem& SH);
    virtual int Size()
    {
        int nSize = sizeof(*this);
        return nSize;
    }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    bool BindRemappedSkinningData(uint32 guid);

    CREMeshImpl()
    {
    }

    virtual ~CREMeshImpl()
    {
    }

#if !defined(_RELEASE)
    inline bool ValidateDraw(EShaderType shaderType);
#endif

    virtual bool GetGeometryInfo(SGeometryInfo &geomInfo) override;
    virtual AZ::Vertex::Format GetVertexFormat() const override;

    //protected:
    //  CREMeshImpl(CREMeshImpl&);
    //  CREMeshImpl& operator=(CREMeshImpl&);
};
