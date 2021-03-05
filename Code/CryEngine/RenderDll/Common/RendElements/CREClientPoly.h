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

#ifndef __CRECLIENTPOLY_H__
#define __CRECLIENTPOLY_H__

//=============================================================

struct SClientPolyStat
{
    int NumOccPolys;
    int NumRendPolys;
    int NumVerts;
    int NumIndices;
};

class CREClientPoly
    : public CRendElementBase
{
public:
    enum eFlags
    {
        efAfterWater = 1 << 0,
        efShadowGen = 1 << 1,
    };
    SShaderItem m_Shader;
    CRenderObject* m_pObject;
    short m_sNumVerts;
    short m_sNumIndices;
    byte m_nCPFlags;
    int m_nOffsVert;
    int m_nOffsTang;
    int m_nOffsInd;

    SRendItemSorter rendItemSorter;
    static SClientPolyStat mRS;
    static void mfPrintStat();

public:
    CREClientPoly()
    {
        mfSetType(eDATA_ClientPoly);
        m_sNumVerts = 0;
        m_nCPFlags = 0;
        mfUpdateFlags(FCEF_TRANSFORM);
    }

    virtual ~CREClientPoly() {};

    virtual void mfPrepare(bool bCheckOverflow);
    virtual CRendElementBase* mfCopyConstruct(void);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_PolysStorage);
    }

    static TArray<CREClientPoly*> m_PolysStorage[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];
};

#endif  // __CRECLIENTPOLY2D_H__
