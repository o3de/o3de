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

// Description : Render element that uses the IREGameEffect interface for its functionality

#include "RenderDll_precompiled.h"
#include "CREGameEffect.h"

//--------------------------------------------------------------------------------------------------
// Name: CREGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CREGameEffect::CREGameEffect()
{
    m_pImpl = NULL;
    mfSetType(eDATA_GameEffect);
    mfUpdateFlags(FCEF_TRANSFORM);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CREGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CREGameEffect::~CREGameEffect()
{
    SAFE_DELETE(m_pImpl);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: mfPrepare
// Desc: Prepares rendering
//--------------------------------------------------------------------------------------------------
void CREGameEffect::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE                         = this;
    gRenDev->m_RP.m_RendNumIndices  = 0;
    gRenDev->m_RP.m_RendNumVerts        = 0;

    if (m_pImpl)
    {
        m_pImpl->mfPrepare(false);
    }
}//-------------------------------------------------------------------------------------------------
