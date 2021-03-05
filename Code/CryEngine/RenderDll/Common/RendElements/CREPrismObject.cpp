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

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

#include "CREPrismObject.h"
#include <IEntityRenderState.h> // <> required for Interfuscator

CREPrismObject::CREPrismObject()
    : CRendElementBase()
    , m_center(0, 0, 0)
{
    mfSetType(eDATA_PrismObject);
    mfUpdateFlags(FCEF_TRANSFORM);
}

void CREPrismObject::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;
}


#endif // EXCLUDE_DOCUMENTATION_PURPOSE
