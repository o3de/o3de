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

#ifndef __CREOCCLUSIONQUERY_H__
#define __CREOCCLUSIONQUERY_H__

#define SUPP_HMAP_OCCL
#define SUPP_HWOBJ_OCCL

//=============================================================

class CRenderMesh;

class CREOcclusionQuery
    : public CRendElementBase
{
    friend class CRender3D;
    bool m_bSucceeded;
public:

    int m_nVisSamples;
    int m_nCheckFrame;
    int m_nDrawFrame;
    Vec3 m_vBoxMin;
    Vec3 m_vBoxMax;

    UINT_PTR m_nOcclusionID; // this will carry a pointer LPDIRECT3DQUERY9, so it needs to be 64-bit on Windows 64

    CRenderMesh* m_pRMBox;
    static uint32 m_nQueriesPerFrameCounter;
    static uint32 m_nReadResultNowCounter;
    static uint32 m_nReadResultTryCounter;

    CREOcclusionQuery()
    {
        m_nOcclusionID = 0;

        m_nVisSamples = 800 * 600;
        m_nCheckFrame = 0;
        m_nDrawFrame = 0;
        m_vBoxMin = Vec3(0, 0, 0);
        m_vBoxMax = Vec3(0, 0, 0);
        m_pRMBox = NULL;

        mfSetType(eDATA_OcclusionQuery);
        mfUpdateFlags(FCEF_TRANSFORM);
    }

    bool RT_ReadResult_Try(uint32 nDefaultNumSamples);

    ILINE bool HasSucceeded() const { return m_bSucceeded; }

    virtual ~CREOcclusionQuery();

    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
    virtual void mfReset();
    virtual bool mfReadResult_Try(uint32 nDefaultNumSamples = 1);
    virtual bool mfReadResult_Now();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

struct OcclusionTestClient
{
    OcclusionTestClient()
        : nLastOccludedMainFrameID(0)
        , nLastVisibleMainFrameID(0)
    {
#ifdef SUPP_HMAP_OCCL
        vLastVisPoint.Set(0, 0, 0);
        nTerrainOccLastFrame = 0;
#endif
#ifdef SUPP_HWOBJ_OCCL
        bOccluded = true;
        pREOcclusionQuery = 0;
#endif
        //nInstantTestRequested=0;
    }
#ifdef SUPP_HWOBJ_OCCL
    ~OcclusionTestClient()
    {
        if (pREOcclusionQuery)
        {
            pREOcclusionQuery->Release(false);
        }
    }
#endif
    uint32 nLastVisibleMainFrameID, nLastOccludedMainFrameID;
    uint32 nLastShadowCastMainFrameID, nLastNoShadowCastMainFrameID;
#ifdef SUPP_HMAP_OCCL
    Vec3 vLastVisPoint;
    int nTerrainOccLastFrame;
#endif
#ifdef SUPP_HWOBJ_OCCL
    CREOcclusionQuery* pREOcclusionQuery;
    uint8 bOccluded;
#endif
    //uint8 nInstantTestRequested;
};


#endif // CRYINCLUDE_CRYCOMMON_CREOCCLUSIONQUERY_H
