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

#include "RenderDll_precompiled.h"
#include "CREWaterOcean.h"
#include "I3DEngine.h"

CREWaterOcean::CREWaterOcean()
    : CRendElementBase()
{
    mfSetType(eDATA_WaterOcean);
    mfUpdateFlags(FCEF_TRANSFORM);

    m_nVerticesCount = 0;
    m_nIndicesCount = 0;

    m_pVertDecl = 0;
    m_pVertices = 0;
    m_pIndices = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

CREWaterOcean::~CREWaterOcean()
{
    ReleaseOcean();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::mfGetPlane([[maybe_unused]] Plane& pl)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;
    gRenDev->m_RP.m_CurVFormat = eVF_P3F_C4B_T2F;
    FrameUpdate();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

Vec3 CREWaterOcean ::GetPositionAt(float x, float y) const
{
    //assert( m_pWaterSim );
    if (WaterSimMgr())
    {
        return WaterSimMgr()->GetPositionAt((int)x, (int)y);
    }

    return Vec3(0, 0, 0);
}

Vec4* CREWaterOcean::GetDisplaceGrid() const
{
    //assert( m_pWaterSim );
    if (WaterSimMgr())
    {
        return WaterSimMgr()->GetDisplaceGrid();
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::UpdateFFT()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

