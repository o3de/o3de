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

// Description : Backend part of geometry cache rendering

#ifndef CRYINCLUDE_CRYCOMMON_CREGEOMCACHE_H
#define CRYINCLUDE_CRYCOMMON_CREGEOMCACHE_H
#pragma once

#if defined(USE_GEOM_CACHES)

#include <Vertex.h>
#include <CryCommon/StaticInstance.h>

class CREGeomCache
    : public CRendElementBase
{
public:
    struct SMeshInstance
    {
        AABB m_aabb;
        Matrix34 m_matrix;
        Matrix34 m_prevMatrix;
    };

    struct SMeshRenderData
    {
        DynArray<SMeshInstance> m_instances;
        _smart_ptr<IRenderMesh> m_pRenderMesh;
    };

    struct UpdateList
    {
        CryCriticalSection m_mutex;
        AZStd::vector<CREGeomCache*, AZ::AZStdAlloc<CryLegacySTLAllocator>> m_geoms;
    };

public:
    CREGeomCache();
    ~CREGeomCache();

    bool Update(const int flags, const bool bTesselation);
    static void UpdateModified();

    // CRendElementBase interface
    virtual bool mfUpdate(int Flags, bool bTessellation);
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    // CREGeomCache interface
    virtual void InitializeRenderElement(const uint numMeshes, _smart_ptr<IRenderMesh>* pMeshes, uint16 materialId);
    virtual void SetupMotionBlur(CRenderObject* pRenderObject, const SRenderingPassInfo& passInfo);

    virtual volatile int* SetAsyncUpdateState(int& threadId);
    virtual DynArray<SMeshRenderData>* GetMeshFillDataPtr();
    virtual DynArray<SMeshRenderData>* GetRenderDataPtr();
    virtual void DisplayFilledBuffer(const int threadId);


    AZ::Vertex::Format GetVertexFormat() const override;
    bool GetGeometryInfo(SGeometryInfo &streams) override;

private:
    uint16 m_materialId;
    volatile bool m_bUpdateFrame[2];
    volatile int m_transformUpdateState[2];

    // We use a double buffered m_meshFillData array for input from the main thread. When data
    // was successfully sent from the main thread it gets copied to m_meshRenderData
    // This simplifies the cases where frame data is missing, e.g. meshFillData is not updated for a frame
    // Note that meshFillData really needs to be double buffered because the copy occurs in render thread
    // so the next main thread could already be touching the data again
    //
    // Note: m_meshRenderData is directly accessed for ray intersections via GetRenderDataPtr.
    // This is safe, because it's only used in editor.
    DynArray<SMeshRenderData> m_meshFillData[2];
    DynArray<SMeshRenderData> m_meshRenderData;

    static StaticInstance<UpdateList> sm_updateList[2]; // double buffered update lists

    AZ::Vertex::Format m_geomCacheVertexFormat;
};

#endif
#endif // CRYINCLUDE_CRYCOMMON_CREGEOMCACHE_H
