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
#include "AbstractMeshElement.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"
#include "../../../CryCommon/VertexFormats.h"

void AbstractMeshElement::ApplyVert()
{
    if (!GetVertCount())
    {
        return;
    }

    TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(GetVertBufData(), GetVertCount(), 0);
    gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
}

void AbstractMeshElement::ApplyIndices()
{
    if (!GetIndexCount())
    {
        return;
    }

    TempDynIB16::CreateFillAndBind(GetIndexBufData(), GetIndexCount());
}

void AbstractMeshElement::ApplyMesh()
{
    ApplyVert();
    ApplyIndices();
}

void AbstractMeshElement::DrawMeshTriList()
{
    int nVertexBufferCount = GetVertCount();
    int nIndexBufferCount = GetIndexCount();
    if (nVertexBufferCount <= 0 || nIndexBufferCount <= 0)
    {
        return;
    }
    gcpRendD3D->FX_Commit();
    gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nVertexBufferCount, 0, nIndexBufferCount);
}

void AbstractMeshElement::DrawMeshWireframe()
{
    int nVertexBufferCount = GetVertCount();
    int nIndexBufferCount = GetIndexCount();
    if (nVertexBufferCount <= 0 || nIndexBufferCount <= 0)
    {
        return;
    }

    const int32 nState = gRenDev->m_RP.m_CurState;
    gcpRendD3D->FX_SetState(nState | GS_WIREFRAME);

    gcpRendD3D->FX_Commit();
    gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nVertexBufferCount, 0, nIndexBufferCount);

    gcpRendD3D->FX_SetState(nState);
}