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
#include "CREFogVolume.h"
#include <IEntityRenderState.h> // <> required for Interfuscator


CREFogVolume::CREFogVolume()
    : CRendElementBase()
    , m_center(0.0f, 0.0f, 0.0f)
    , m_viewerInsideVolume(0)
    , m_stencilRef(0)
    , m_reserved(0)
    , m_localAABB(Vec3(-1, -1, -1), Vec3(1, 1, 1))
    , m_matWSInv()
    , m_fogColor(1, 1, 1, 1)
    , m_globalDensity(1)
    , m_softEdgesLerp(1, 0)
    , m_heightFallOffDirScaled(0, 0, 1)
    , m_heightFallOffBasePoint(0, 0, 0)
    , m_eyePosInWS(0, 0, 0)
    , m_eyePosInOS(0, 0, 0)
    , m_rampParams(0, 0, 0)
    , m_windOffset(0, 0, 0)
    , m_noiseScale(0)
    , m_noiseFreq(1, 1, 1)
    , m_noiseOffset(0)
    , m_noiseElapsedTime(0)
{
    mfSetType(eDATA_FogVolume);
    mfUpdateFlags(FCEF_TRANSFORM);

    m_matWSInv.SetIdentity();
}


CREFogVolume::~CREFogVolume()
{
}


void CREFogVolume::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;
}

