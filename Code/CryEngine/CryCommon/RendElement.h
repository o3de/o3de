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

#ifndef CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
#define CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
#pragma once


//=============================================================

#include "VertexFormats.h"

class CRendElementBase;
struct CRenderChunk;
struct PrimitiveGroup;
class CShader;
struct SShaderTechnique;
class CParserBin;
struct SParserFrame;

namespace AZ
{
    namespace Vertex
    {
        class Format;
    }
}

enum EDataType
{
    eDATA_Unknown = 0,
    eDATA_Sky,
    eDATA_Beam,
    eDATA_ClientPoly,
    eDATA_Flare,
    eDATA_Terrain,
    eDATA_SkyZone,
    eDATA_Mesh,
    eDATA_Imposter,
    eDATA_LensOptics,
    eDATA_FarTreeSprites_Deprecated,
    eDATA_OcclusionQuery,
    eDATA_Particle,
    eDATA_GPUParticle,
    eDATA_PostProcess,
    eDATA_HDRProcess,
    eDATA_Cloud,
    eDATA_HDRSky,
    eDATA_FogVolume,
    eDATA_WaterVolume,
    eDATA_WaterOcean,
    eDATA_VolumeObject,
    eDATA_PrismObject,              // normally this would be #if !defined(EXCLUDE_DOCUMENTATION_PURPOSE) but we keep it to get consistent numbers for serialization
    eDATA_DeferredShading,
    eDATA_GameEffect,
    eDATA_BreakableGlass,
    eDATA_GeomCache,
    eDATA_Gem,
};

#include <Cry_Color.h>

//=======================================================

#define FCEF_TRANSFORM 1
#define FCEF_DIRTY     2
#define FCEF_NODEL     4
#define FCEF_DELETED   8

#define FCEF_MODIF_TC   0x10
#define FCEF_MODIF_VERT 0x20
#define FCEF_MODIF_COL  0x40
#define FCEF_MODIF_MASK 0xf0

#define FCEF_UPDATEALWAYS 0x100
#define FCEF_ALLOC_CUST_FLOAT_DATA 0x200
#define FCEF_MERGABLE    0x400

#define FCEF_SKINNED    0x800
#define FCEF_PRE_DRAW_DONE  0x1000

#define FGP_NOCALC 1
#define FGP_SRC    2
#define FGP_REAL   4
#define FGP_WAIT   8

#define FGP_STAGE_SHIFT 0x10

#define MAX_CUSTOM_TEX_BINDS_NUM 2

struct SGeometryInfo;
class CRendElement;

struct IRenderElementDelegate
{
    virtual void mfPrepare(bool bCheckOverflow) = 0;
    virtual bool mfDraw(CShader* shader, SShaderPass* pass) = 0;
    virtual bool mfSetSampler([[maybe_unused]] int customId, [[maybe_unused]] int nTUnit, [[maybe_unused]] int nTState, [[maybe_unused]] int nTexMaterialSlot, [[maybe_unused]] int nSUnit) { return true; };
};

struct IRenderElement
{
    virtual int mfGetMatId() = 0;
    virtual uint16 mfGetFlags() = 0;
    virtual void mfSetFlags(uint16 fl) = 0;
    virtual void mfUpdateFlags(uint16 fl) = 0;
    virtual void mfClearFlags(uint16 fl) = 0;
    virtual void mfPrepare(bool bCheckOverflow) = 0;
    virtual void mfCenter(Vec3& centr, CRenderObject* pObj) = 0;
    virtual void mfGetBBox(Vec3& vMins, Vec3& vMaxs) = 0;
    virtual void mfReset() = 0;
    virtual void mfGetPlane(Plane_tpl<f32>& pl) = 0;
    virtual void mfExport(struct SShaderSerializeContext& SC) = 0;
    virtual void mfImport(struct SShaderSerializeContext& SC, uint32& offset) = 0;
    virtual void mfPrecache(const SShaderItem& SH) = 0;
    virtual bool mfIsHWSkinned() = 0;
    virtual bool mfCheckUpdate(int Flags, uint16 nFrame, bool bTessellation = false) = 0;
    virtual bool mfUpdate(int Flags, bool bTessellation = false) = 0;
    virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame) = 0;
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm) = 0;
    virtual bool mfPreDraw(SShaderPass* sl) = 0;
    virtual bool mfSetSampler(int customId, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit) = 0;
    virtual void mfSetDelegate(IRenderElementDelegate* pDelegate) = 0;
    virtual IRenderElementDelegate* mfGetDelegate() = 0;
    virtual CRenderChunk* mfGetMatInfo() = 0;
    virtual TRenderChunkArray* mfGetMatInfoList() = 0;
    virtual void* mfGetPointer(ESrcPointer ePT, int* Stride, EParamType Type, ESrcPointer Dst, int Flags) = 0;
    virtual AZ::Vertex::Format GetVertexFormat() const = 0;
    virtual void* GetCustomData() const = 0;
    virtual int GetCustomTexBind(int i) const = 0;
    virtual CRendElementBase* mfCopyConstruct() = 0;
    virtual EDataType mfGetType() = 0;
    virtual int Size() = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

};

class CRendElement
{
public:
    static CRendElement m_RootGlobal;
    static CRendElement m_RootRelease[];
    CRendElement* m_NextGlobal;
    CRendElement* m_PrevGlobal;

    EDataType m_Type;

protected:
    virtual void UnlinkGlobal()
    {
        if (!m_NextGlobal || !m_PrevGlobal)
        {
            return;
        }
        m_NextGlobal->m_PrevGlobal = m_PrevGlobal;
        m_PrevGlobal->m_NextGlobal = m_NextGlobal;
        m_NextGlobal = m_PrevGlobal = NULL;
    }

    virtual void LinkGlobal(CRendElement* Before)
    {
        if (m_NextGlobal || m_PrevGlobal)
        {
            return;
        }
        m_NextGlobal = Before->m_NextGlobal;
        Before->m_NextGlobal->m_PrevGlobal = this;
        Before->m_NextGlobal = this;
        m_PrevGlobal = Before;
    }

public:
    CRendElement();
    virtual ~CRendElement();
    virtual void Release(bool bForce = false);
    virtual const char* mfTypeString();
    virtual void mfSetType(EDataType t) { m_Type = t; }
    virtual void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}
    virtual int Size() { return 0; }

    static void ShutDown();
    static void Tick();
    static void Cleanup();
};

class CRendElementBase
    : public CRendElement
    , public IRenderElement
{
public:
    uint16 m_Flags;
    uint16 m_nFrameUpdated;

public:
    void* m_CustomData;
    int m_CustomTexBind[MAX_CUSTOM_TEX_BINDS_NUM];

    struct SGeometryStreamInfo
    {
        const void* pStream;
        int nOffset;
        int nStride;
    };
    struct SGeometryInfo
    {
        uint32        bonesRemapGUID; // Input paremeter to fetch correct skinning stream.

        int           primitiveType; //!< \see eRenderPrimitiveType
        AZ::Vertex::Format vertexFormat;
        uint32        streamMask;

        int32  nFirstIndex;
        int32  nNumIndices;
        uint32 nFirstVertex;
        uint32 nNumVertices;

        uint32 nMaxVertexStreams;

        SGeometryStreamInfo indexStream;
        SGeometryStreamInfo vertexStream[VSF_NUM];

        void* pTessellationAdjacencyBuffer;
        void* pSkinningExtraBonesBuffer;
    };

public:
    CRendElementBase();
    virtual ~CRendElementBase();


    virtual void mfPrepare(bool bCheckOverflow) override 
    {
        if (m_delegate)
        {
            m_delegate->mfPrepare(bCheckOverflow);
        }
    }
    bool mfDraw(CShader* ef, SShaderPass* sfm) override { return m_delegate ? m_delegate->mfDraw(ef, sfm) : true; }
    bool mfSetSampler(int customId, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit) override { return m_delegate ? m_delegate->mfSetSampler(customId, nTUnit, nTState, nTexMaterialSlot, nSUnit) : false; }
    void mfSetDelegate(IRenderElementDelegate* pDelegate) override { m_delegate = pDelegate; }
    IRenderElementDelegate* mfGetDelegate() { return m_delegate; }   

    EDataType mfGetType() override { return m_Type; };

    CRenderChunk* mfGetMatInfo() override { return nullptr; }
    TRenderChunkArray* mfGetMatInfoList() override { return nullptr; }
    int mfGetMatId() override { return -1; }
    void mfReset() override {};
    CRendElementBase* mfCopyConstruct() override;
    void mfCenter(Vec3& centr, CRenderObject* pObj) override;

    bool mfCompile([[maybe_unused]] CParserBin& Parser, [[maybe_unused]] SParserFrame& Frame) override { return false; }
    bool mfPreDraw([[maybe_unused]] SShaderPass* sl) override { return true; }
    bool mfUpdate([[maybe_unused]] int Flags, [[maybe_unused]] bool bTessellation = false) override { return true; }
    void mfPrecache([[maybe_unused]] const SShaderItem& SH) override {}
    void mfExport([[maybe_unused]] struct SShaderSerializeContext& SC) override { CryFatalError("mfExport has not been implemented for this render element type"); }
    void mfImport([[maybe_unused]] struct SShaderSerializeContext& SC, [[maybe_unused]] uint32& offset) override { CryFatalError("mfImport has not been implemented for this render element type"); }
    void mfGetPlane(Plane_tpl<f32>& pl) override;
    void* mfGetPointer([[maybe_unused]] ESrcPointer ePT, [[maybe_unused]] int* Stride, [[maybe_unused]] EParamType Type, [[maybe_unused]] ESrcPointer Dst, [[maybe_unused]] int Flags) override { return nullptr; }
    
    uint16 mfGetFlags()  override { return m_Flags; }
    void mfSetFlags(uint16 fl)  override { m_Flags = fl; }
    void mfUpdateFlags(uint16 fl)  override { m_Flags |= fl; }
    void mfClearFlags(uint16 fl)  override { m_Flags &= ~fl; }
    bool mfCheckUpdate(int Flags, uint16 nFrame, bool bTessellation = false) override
    {
        if (nFrame != m_nFrameUpdated || (m_Flags & (FCEF_DIRTY | FCEF_SKINNED | FCEF_UPDATEALWAYS)))
        {
            m_nFrameUpdated = nFrame;
            return mfUpdate(Flags, bTessellation);
        }
        return true;
    }
    void mfGetBBox(Vec3& vMins, Vec3& vMaxs) override
    {
        vMins.Set(0, 0, 0);
        vMaxs.Set(0, 0, 0);
    }
    bool mfIsHWSkinned() override { return false; }
    int Size() override { return 0; }
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const  override {}
    AZ::Vertex::Format GetVertexFormat() const override { return AZ::Vertex::Format(eVF_Unknown); };
    virtual bool GetGeometryInfo([[maybe_unused]] SGeometryInfo& streams) { return false; }
    void Draw([[maybe_unused]] CRenderObject* pObj, [[maybe_unused]] const struct SGraphicsPiplinePassContext& ctx) {};
    void* GetCustomData() const { return m_CustomData; }
    int GetCustomTexBind(int index) const { return m_CustomTexBind[index]; }

protected:
    IRenderElementDelegate * m_delegate = nullptr;
};

#include "CREMesh.h"
#include "CRESky.h"
#include "CREOcclusionQuery.h"
#include "CREImposter.h"
#include "CREBaseCloud.h"
#include "CREPostProcess.h"
#include "CREFogVolume.h"
#include "CREWaterVolume.h"
#include "CREWaterOcean.h"
#include "CREVolumeObject.h"
#include "CREGameEffect.h"
#include "CREGeomCache.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "CREPrismObject.h"
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//==========================================================

#endif // CRYINCLUDE_CRYCOMMON_RENDELEMENT_H
