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

// Description : make physical representation

#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "IndexedMesh.h"
#include "3dEngine.h"
#include "CGFContent.h"
#include "ObjMan.h"
#define SMALL_MESH_NUM_INDEX 30

#include <CryPhysicsDeprecation.h>


//////////////////////////////////////////////////////////////////////////
///////////////////////// Breakable Geometry /////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace
{
    template<class T>
    struct SplitArray
    {
        T* ptr[2];
        T& operator[](int idx) { return ptr[idx >> 15][idx & ~(1 << 15)]; }
        T* operator+(int idx) { return ptr[idx >> 15] + (idx & ~(1 << 15)); }
        T* operator-(int idx) { return ptr[idx >> 15] - (idx & ~(1 << 15)); }
    };
}

static inline int mapiTri(int itri, int* pIdx2iTri)
{
    int mask = (itri - BOP_NEWIDX0) >> 31;
    return itri & mask | pIdx2iTri[itri - BOP_NEWIDX0 | mask];
}

static inline void swap(int* pSubsets, vtx_idx* pidx, int* pmap, int i1, int i2)
{
    int i = pSubsets[i1];
    pSubsets[i1] = pSubsets[i2];
    pSubsets[i2] = i;
    i = pmap[i1];
    pmap[i1] = pmap[i2];
    pmap[i2] = i;
    for (i = 0; i < 3; i++)
    {
        vtx_idx u = pidx[i1 * 3 + i];
        pidx[i1 * 3 + i] = pidx[i2 * 3 + i];
        pidx[i2 * 3 + i] = u;
    }
}
static void qsort(int* pSubsets, vtx_idx* pidx, int* pmap, int ileft, int iright, int iter = 0)
{
    if (ileft >= iright)
    {
        return;
    }
    int i, ilast, diff = 0;
    swap(pSubsets, pidx, pmap, ileft, (ileft + iright) >> 1);
    for (ilast = ileft, i = ileft + 1; i <= iright; i++)
    {
        diff |= pSubsets[i] - pSubsets[ileft];
        if (pSubsets[i] < pSubsets[ileft] + iter) // < when iter==0 and <= when iter==1
        {
            swap(pSubsets, pidx, pmap, ++ilast, i);
        }
    }
    swap(pSubsets, pidx, pmap, ileft, ilast);

    if (diff)
    {
        qsort(pSubsets, pidx, pmap, ileft, ilast - 1, iter ^ 1);
        qsort(pSubsets, pidx, pmap, ilast + 1, iright, iter ^ 1);
    }
}

static inline int check_mask(unsigned int* pMask, int i)
{
    return pMask[i >> 5] >> (i & 31) & 1;
}
static inline void set_mask(unsigned int* pMask, int i)
{
    pMask[i >> 5] |= 1u << (i & 31);
}
static inline void clear_mask(unsigned int* pMask, int i)
{
    pMask[i >> 5] &= ~(1u << (i & 31));
}

//////////////////////////////////////////////////////////////////////////
///////////////////////// Deformable Geometry ////////////////////////////
//////////////////////////////////////////////////////////////////////////



int CStatObj::SubobjHasDeformMorph(int iSubObj)
{
    int i;
    char nameDeformed[256];
    cry_strcpy(nameDeformed, m_subObjects[iSubObj].name);
    cry_strcat(nameDeformed, "_Destroyed");

    for (i = m_subObjects.size() - 1; i >= 0 && strcmp(m_subObjects[i].name, nameDeformed); i--)
    {
        ;
    }
    return i;
}

#define getBidx(islot) _getBidx(islot, pIdxABuf, pFaceToFace0A, pFace0ToFaceB, pIdxB)
static inline int _getBidx(int islot, int* pIdxABuf, uint16* pFaceToFace0A, int* pFace0ToFaceB, vtx_idx* pIdxB)
{
    int idx, mask;
    idx = pFace0ToFaceB[pFaceToFace0A[pIdxABuf[islot] >> 2]] * 3 + (pIdxABuf[islot] & 3);
    mask = idx >> 31;
    return (pIdxB[idx & ~mask] & ~mask) + mask;
}


int CStatObj::SetDeformationMorphTarget(IStatObj* pDeformed)
{
    int i, j, k, j0, it, ivtx, nVtxA, nVtxAnew, nVtxA0, nIdxA, nFacesA, nFacesB;
    vtx_idx* pIdxA, * pIdxB;
    int* pVtx2IdxA, * pIdxABuf, * pFace0ToFaceB;
    uint16* pFaceToFace0A, * pFaceToFace0B, maxFace0;
    _smart_ptr<IRenderMesh> pMeshA, pMeshB, pMeshBnew;
    strided_pointer<Vec3> pVtxA, pVtxB, pVtxBnew;
    strided_pointer<Vec2> pTexA, pTexB, pTexBnew;
    strided_pointer<SPipTangents> pTangentsA, pTangentsB, pTangentsBnew;
    if (!GetRenderMesh())
    {
        MakeRenderMesh();
    }
    if (!pDeformed->GetRenderMesh())
    {
        ((CStatObj*)pDeformed)->MakeRenderMesh();
    }
    if (!(pMeshA = GetRenderMesh()) || !(pMeshB = pDeformed->GetRenderMesh()) ||
        !(pFaceToFace0A = m_pMapFaceToFace0) || !(pFaceToFace0B = ((CStatObj*)pDeformed)->m_pMapFaceToFace0))
    {
        return 0;
    }
    if (pMeshA->GetMorphBuddy())
    {
        return 1;
    }

    if (pMeshA->GetVerticesCount() > 0xffff || pMeshB->GetVerticesCount() > 0xffff)
    {
        return 0;
    }

    nVtxA0 = nVtxA = pMeshA->GetVerticesCount();
    pVtxA.data = (Vec3*)pMeshA->GetPosPtr(pVtxA.iStride, FSL_READ);
    pTexA.data = (Vec2*)pMeshA->GetUVPtr(pTexA.iStride, FSL_READ);
    pTangentsA.data = (SPipTangents*)pMeshA->GetTangentPtr(pTangentsA.iStride, FSL_READ);

    pVtxB.data = (Vec3*)pMeshB->GetPosPtr(pVtxB.iStride, FSL_READ);
    pTexB.data = (Vec2*)pMeshB->GetUVPtr(pTexB.iStride, FSL_READ);
    pTangentsB.data = (SPipTangents*)pMeshB->GetTangentPtr(pTangentsB.iStride, FSL_READ);

    nFacesB = pMeshB->GetIndicesCount();
    nFacesB /= 3;
    pIdxB = pMeshB->GetIndexPtr(FSL_READ);
    nIdxA = pMeshA->GetIndicesCount();
    nFacesA = nIdxA /= 3;
    pIdxA = pMeshA->GetIndexPtr(FSL_READ);

    memset(pVtx2IdxA = new int[nVtxA + 1], 0, (nVtxA + 1) * sizeof(int));
    for (i = 0; i < nIdxA; i++)
    {
        pVtx2IdxA[pIdxA[i]]++;
    }
    for (i = maxFace0 = 0; i < nFacesA; i++)
    {
        maxFace0 = max(maxFace0, pFaceToFace0A[i]);
    }
    for (i = 0; i < nVtxA; i++)
    {
        pVtx2IdxA[i + 1] += pVtx2IdxA[i];
    }
    pIdxABuf = new int[nIdxA];
    for (i = nFacesA - 1; i >= 0; i--)
    {
        for (j = 2; j >= 0; j--)
        {
            pIdxABuf[--pVtx2IdxA[pIdxA[i * 3 + j]]] = i * 4 + j;
        }
    }

    for (i = nFacesB - 1; i >= 0; i--)
    {
        maxFace0 = max(maxFace0, pFaceToFace0B[i]);
    }
    memset(pFace0ToFaceB = new int[maxFace0 + 1], -1, (maxFace0 + 1) * sizeof(int));
    for (i = nFacesB - 1; i >= 0; i--)
    {
        pFace0ToFaceB[pFaceToFace0B[i]] = i;
    }

    for (i = nVtxAnew = k = 0; i < nVtxA; i++)
    {
        for (j = pVtx2IdxA[i]; j < pVtx2IdxA[i + 1] - 1; j++)
        {
            for (k = pVtx2IdxA[i + 1] - 1; k > j; k--)
            {
                if (getBidx(k - 1) > getBidx(k))
                {
                    it = pIdxABuf[k - 1], pIdxABuf[k - 1] = pIdxABuf[k], pIdxABuf[k] = it;
                }
            }
        }
        for (j = pVtx2IdxA[i] + 1; j < pVtx2IdxA[i + 1]; j++)
        {
            nVtxAnew += iszero(getBidx(j) - getBidx(j - 1)) ^ 1;
#ifdef _DEBUG
            if ((pVtxB[getBidx(j)] - pVtxB[getBidx(j - 1)]).len2() > sqr(0.01f))
            {
                k++;
            }
#endif
        }
    }

    pMeshBnew = GetRenderer()->CreateRenderMesh("StatObj_Deformed", GetFilePath());
    pMeshBnew->UpdateVertices(0, nVtxA0 + nVtxAnew, 0, VSF_GENERAL, 0u);
    if (nVtxAnew)
    {
        pMeshA = GetRenderer()->CreateRenderMesh("StatObj_MorphTarget", GetFilePath());
        m_pRenderMesh->CopyTo(pMeshA, nVtxAnew);
        pVtxA.data = (Vec3*)pMeshA->GetPosPtr(pVtxA.iStride, FSL_SYSTEM_UPDATE);
        pTexA.data = (Vec2*)pMeshA->GetUVPtr(pTexA.iStride, FSL_SYSTEM_UPDATE);
        pTangentsA.data = (SPipTangents*)pMeshA->GetTangentPtr(pTangentsA.iStride, FSL_SYSTEM_UPDATE);
        nIdxA = pMeshA->GetIndicesCount();
        pIdxA = pMeshA->GetIndexPtr(FSL_READ);
        m_pRenderMesh = pMeshA;
    }

    pVtxBnew.data = (Vec3*)pMeshBnew->GetPosPtr(pVtxBnew.iStride, FSL_SYSTEM_UPDATE);
    pTexBnew.data = (Vec2*)pMeshBnew->GetUVPtr(pTexBnew.iStride, FSL_SYSTEM_UPDATE);
    pTangentsBnew.data = (SPipTangents*)pMeshBnew->GetTangentPtr(pTangentsBnew.iStride, FSL_SYSTEM_UPDATE);

    for (i = 0; i < nVtxA0; i++)
    {
        for (j = j0 = pVtx2IdxA[i]; j < pVtx2IdxA[i + 1]; j++)
        {
            if (j == pVtx2IdxA[i + 1] - 1 || getBidx(j) != getBidx(j + 1))
            {
                if (j0 > pVtx2IdxA[i])
                {
                    ivtx = nVtxA++;
                    pVtxA[ivtx] = pVtxA[i];
                    pTangentsA[ivtx] = pTangentsA[i];
                    pTexA[ivtx] = pTexA[i];
                    for (k = j0; k <= j; k++)
                    {
                        pIdxA[(pIdxABuf[k] >> 2) * 3 + (pIdxABuf[k] & 3)] = ivtx;
                    }
                }
                else
                {
                    ivtx = i;
                }
                if ((it = getBidx(j)) >= 0)
                {
#ifdef _DEBUG
                    static float maxdist = 0.1f;
                    float dist = (pVtxB[it] - pVtxA[i]).len();
                    if (dist > maxdist)
                    {
                        k++;
                    }
#endif
                    pVtxBnew[ivtx] = pVtxB[it];
                    pTangentsBnew[ivtx] = pTangentsB[it];
                    pTexBnew[ivtx] = pTexB[it];
                }
                else
                {
                    pVtxBnew[ivtx] = pVtxA[i];
                    pTangentsBnew[ivtx] = pTangentsA[i];
                    pTexBnew[ivtx] = pTexA[i];
                }
                j0 = j + 1;
            }
        }
    }

    pMeshA->SetMorphBuddy(pMeshBnew);
    pDeformed->SetFlags(pDeformed->GetFlags() | STATIC_OBJECT_HIDDEN);

    pMeshBnew->UnlockStream(VSF_GENERAL);
    pMeshBnew->UnlockStream(VSF_TANGENTS);
    pMeshA->UnlockStream(VSF_GENERAL);
    pMeshA->UnlockStream(VSF_TANGENTS);

    delete [] pVtx2IdxA;
    delete [] pIdxABuf;
    delete [] pFace0ToFaceB;

    return 1;
}


static inline float max_fast(float op1, float op2) { return (op1 + op2 + fabsf(op1 - op2)) * 0.5f; }
static inline float min_fast(float op1, float op2) { return (op1 + op2 - fabsf(op1 - op2)) * 0.5f; }

static void UpdateWeights(const Vec3& pt, float r, float strength, IRenderMesh* pMesh, IRenderMesh* pWeights)
{
    int i, nVtx = pMesh->GetVerticesCount();
    float r2 = r * r, rr = 1 / r;
    strided_pointer<Vec3> pVtx;
    strided_pointer<Vec2> pWeight;
    pVtx.data = (Vec3*)pMesh->GetPosPtr(pVtx.iStride, FSL_SYSTEM_UPDATE);
    pWeight.data = (Vec2*)pWeights->GetPosPtr(pWeight.iStride, FSL_SYSTEM_UPDATE);

    if (r > 0)
    {
        for (i = 0; i < nVtx; i++)
        {
            if ((pVtx[i] - pt).len2() < r2)
            {
                pWeight[i].x = max_fast(0.0f, min_fast(1.0f, pWeight[i].x + strength * (1 - (pVtx[i] - pt).len() * rr)));
            }
        }
    }
    else
    {
        for (i = 0; i < nVtx; i++)
        {
            pWeight[i].x = max_fast(0.0f, min_fast(1.0f, pWeight[i].x + strength));
        }
    }
}


IStatObj* CStatObj::DeformMorph(const Vec3& pt, float r, float strength, IRenderMesh* pWeights)
{
    int i, j;
    CStatObj* pObj = this;

    if (!GetCVars()->e_DeformableObjects)
    {
        return pObj;
    }

    if (m_bHasDeformationMorphs)
    {
        if (!(GetFlags() & STATIC_OBJECT_CLONE))
        {
            pObj = (CStatObj*)Clone(true, false, false);
            pObj->m_bUnmergable = 1;
            for (i = pObj->GetSubObjectCount() - 1; i >= 0; i--)
            {
                if ((j = pObj->SubobjHasDeformMorph(i)) >= 0)
                {
                    pObj->GetSubObject(i)->pWeights = pObj->GetSubObject(i)->pStatObj->GetRenderMesh()->GenerateMorphWeights();
                    pObj->GetSubObject(j)->pStatObj->SetFlags(pObj->GetSubObject(j)->pStatObj->GetFlags() | STATIC_OBJECT_HIDDEN);
                }
            }
            return pObj->DeformMorph(pt, r, strength, pWeights);
        }
        for (i = m_subObjects.size() - 1; i >= 0; i--)
        {
            if (m_subObjects[i].pWeights)
            {
                UpdateWeights(m_subObjects[i].tm.GetInverted() * pt,
                    r * (fabs_tpl(m_subObjects[i].tm.GetColumn(0).len2() - 1.0f) < 0.01f ? 1.0f : 1.0f / m_subObjects[i].tm.GetColumn(0).len()),
                    strength, m_subObjects[i].pStatObj->GetRenderMesh(), m_subObjects[i].pWeights);
            }
        }
    }
    else if (m_nSubObjectMeshCount == 0 && m_pRenderMesh && m_pRenderMesh->GetMorphBuddy())
    {
        if (!pWeights)
        {
            pObj = new CStatObj;
            pObj->m_pMaterial = m_pMaterial;
            pObj->m_fObjectRadius = m_fObjectRadius;
            pObj->m_vBoxMin = m_vBoxMin;
            pObj->m_vBoxMax = m_vBoxMax;
            pObj->m_vVegCenter = m_vVegCenter;
            pObj->m_fRadiusHors = m_fRadiusHors;
            pObj->m_fRadiusVert = m_fRadiusVert;
            pObj->m_nFlags = m_nFlags | STATIC_OBJECT_CLONE;
            pObj->m_bHasDeformationMorphs = true;
            pObj->m_nSubObjectMeshCount = 1;
            pObj->m_bSharesChildren = true;
            pObj->m_subObjects.resize(1);
            pObj->m_subObjects[0].nType = STATIC_SUB_OBJECT_MESH;
            pObj->m_subObjects[0].name = "";
            pObj->m_subObjects[0].properties = "";
            pObj->m_subObjects[0].bIdentityMatrix = true;
            pObj->m_subObjects[0].tm.SetIdentity();
            pObj->m_subObjects[0].localTM.SetIdentity();
            pObj->m_subObjects[0].pStatObj = this;
            pObj->m_subObjects[0].nParent = -1;
            pObj->m_subObjects[0].helperSize = Vec3(0, 0, 0);
            pObj->m_subObjects[0].pWeights = GetRenderMesh()->GenerateMorphWeights();
            return pObj->DeformMorph(pt, r, strength, pWeights);
        }
        UpdateWeights(pt, r, strength, m_pRenderMesh, pWeights);
    }

    return this;
}


IStatObj* CStatObj::HideFoliage()
{
    int i;//,j,idx0;
    CMesh* pMesh;
    if (!GetIndexedMesh())
    {
        return this;
    }

    pMesh = GetIndexedMesh()->GetMesh();
    for (i = pMesh->m_subsets.size() - 1; i >= 0; i--)
    {
        if (pMesh->m_subsets[i].nPhysicalizeType == PHYS_GEOM_TYPE_NONE)
        {
            //pMesh->m_subsets[i].nMatFlags |= MTL_FLAG_NODRAW;
            //idx0 = i>0 ? pMesh->m_subsets[i-1].nFirstIndexId+pMesh->m_subsets[i-1].nNumIndices : 0;
            pMesh->m_subsets.erase(pMesh->m_subsets.begin() + i);
            /*for(j=i;j<pMesh->m_subsets.size();j++)
            {
                memmove(pMesh->m_pIndices+idx0, pMesh->m_pIndices+pMesh->m_subsets[j].nFirstIndexId,
                    pMesh->m_subsets[j].nNumIndices*sizeof(pMesh->m_pIndices[0]));
                pMesh->m_subsets[j].nFirstIndexId = idx0;
                idx0 += pMesh->m_subsets[j].nNumIndices;
            }
            pMesh->m_nIndexCount = idx0;*/
        }
    }
    Invalidate();

    return this;
}


//////////////////////////////////////////////////////////////////////////
////////////////////////   SubObjects    /////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static inline int GetEdgeByBuddy(mesh_data* pmd, int itri, int itri_buddy)
{
    int iedge = 0, imask;
    imask = pmd->pTopology[itri].ibuddy[1] - itri_buddy;
    imask = (imask - 1) >> 31 ^ imask >> 31;
    iedge = 1 & imask;
    imask = pmd->pTopology[itri].ibuddy[2] - itri_buddy;
    imask = (imask - 1) >> 31 ^ imask >> 31;
    iedge = iedge & ~imask | 2 & imask;
    return iedge;
}
static inline float qmin(float op1, float op2) { return (op1 + op2 - fabsf(op1 - op2)) * 0.5f; }
static inline float qmax(float op1, float op2) { return (op1 + op2 + fabsf(op1 - op2)) * 0.5f; }

static int __s_pvtx_map_dummy = 0;

static void SyncToRenderMesh(SSyncToRenderMeshContext* ctx, volatile int* updateState)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngine);

    IGeometry* pPhysGeom = ctx->pObj->GetPhysGeom() ? ctx->pObj->GetPhysGeom()->pGeom : 0;
    if (pPhysGeom)
    {
        pPhysGeom->Lock(0);
        IStatObj* pObjSrc = (IStatObj*)pPhysGeom->GetForeignData(0);
        if (pPhysGeom->GetForeignData(DATA_MESHUPDATE) || !ctx->pObj->m_hasClothTangentsData || pObjSrc != ctx->pObj && pObjSrc != ctx->pObj->GetCloneSourceObject())
        {   // skip all updates if the mesh was altered
            if (updateState)
            {
                CryInterlockedDecrement(updateState);
            }
            pPhysGeom->Unlock(0);
            return;
        }
    }
    Vec3 n, edge, t;
    int i, j;
    Vec3* vmin = ctx->vmin, * vmax = ctx->vmax;
    int iVtx0 = ctx->iVtx0, nVtx = ctx->nVtx, mask = ctx->mask;
    strided_pointer<Vec3> pVtx = ctx->pVtx;
    int* pVtxMap = (mask == ~0) ? &__s_pvtx_map_dummy : ctx->pVtxMap;
    float rscale = ctx->rscale;
    SClothTangentVtx* ctd = ctx->ctd;
    strided_pointer<Vec3> pMeshVtx = ctx->pMeshVtx;
    strided_pointer<SPipTangents> pTangents = ctx->pTangents;
    strided_pointer<Vec3> pNormals = ctx->pNormals;

    AABB bbox;
    bbox.Reset();

    if (pMeshVtx)
    {
        for (i = iVtx0; i < nVtx; i++)
        {
            Vec3 v = pVtx[j = pVtxMap[i & ~mask] | i & mask] * rscale;
            bbox.Add(v);
            pMeshVtx[i] = v;
        }
        *vmin = bbox.min;
        *vmax = bbox.max;
    }

    if (pTangents)
    {
        for (i = iVtx0; i < nVtx; i++)
        {
            SMeshTangents tb(pTangents[i]);
            int16 nsg;
            tb.GetR(nsg);

            j = pVtxMap[i & ~mask] | i & mask;
            n = pNormals[j] * aznumeric_cast<float>(ctd[i].sgnNorm);
            edge = (pVtx[ctd[i].ivtxT] - pVtx[j]).normalized();
            Matrix33 M;
            crossproduct_matrix(pNormals[j] * ctd[i].edge.y, M) *= nsg;
            M += Matrix33(IDENTITY) * ctd[i].edge.x;
            t = M.GetInverted() * (edge - n * ctd[i].edge.z);
            (t -= n * (n * t)).Normalize();
            t.x = qmin(qmax(t.x, -0.9999f), 0.9999f);
            t.y = qmin(qmax(t.y, -0.9999f), 0.9999f);
            t.z = qmin(qmax(t.z, -0.9999f), 0.9999f);
            Vec3 b = n.Cross(t) * nsg;

            tb = SMeshTangents(t, b, nsg);
            tb.ExportTo(pTangents[i]);
        }
    }

    if (updateState)
    {
        CryInterlockedDecrement(updateState);
    }
    if (pPhysGeom)
    {
        pPhysGeom->Unlock(0);
    }
}

IStatObj* CStatObj::UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0, int nVtx, int* pVtxMap, float rscale)
{
    CStatObj* pObj = this;
    if (m_pRenderMesh)
    {
        strided_pointer<Vec3> pMeshVtx;
        strided_pointer<SPipTangents> pTangents;
        int i, j, mask = 0, dummy = 0, nVtxFull;
        if (!pVtxMap)
        {
            pVtxMap = &dummy, mask = ~0;
        }
        AABB bbox;
        bbox.Reset();
        SClothTangentVtx* ctd;

        if (!m_hasClothTangentsData && GetPhysGeom() && GetPhysGeom()->pGeom->GetType() == GEOM_TRIMESH && m_pRenderMesh)
        {
            if (GetPhysGeom()->pGeom->GetForeignData(DATA_MESHUPDATE))
            {
                return this;
            }
            ctd = m_pClothTangentsData = new SClothTangentVtx[nVtxFull = m_pRenderMesh->GetVerticesCount()];
            m_hasClothTangentsData = 1;
            memset(m_pClothTangentsData, 0, sizeof(SClothTangentVtx) * nVtxFull);
            mesh_data* pmd = (mesh_data*)GetPhysGeom()->pGeom->GetData();
            m_pRenderMesh->LockForThreadAccess();
            pTangents.data = (SPipTangents*)m_pRenderMesh->GetTangentPtr(pTangents.iStride, FSL_READ);
            for (i = 0; i < pmd->nTris; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    ctd[pmd->pIndices[i * 3 + j]].ivtxT = i, ctd[pmd->pIndices[i * 3 + j]].sgnNorm = j;
                }
            }
            if (pmd->pVtxMap)
            {
                for (i = 0; i < pmd->nVertices; i++)
                {
                    ctd[i].ivtxT = ctd[pmd->pVtxMap[i]].ivtxT, ctd[i].sgnNorm = ctd[pmd->pVtxMap[i]].sgnNorm;
                }
            }

            for (i = 0; i < nVtxFull && pTangents; i++)
            {
                j = pmd->pVtxMap ? pmd->pVtxMap[i] : i;

                SMeshTangents tb = SMeshTangents(pTangents[i]);
                Vec3 t, b, s;
                tb.GetTBN(t, b, s);

                float tedge = -1, tedgeDenom = 0;
                int itri = ctd[i].ivtxT, iedge = ctd[i].sgnNorm, itriT = 0, iedgeT = 0, itri1, loop;
                Vec3 n, edge, edge0;
                n.zero();

                for (int iter = 0; iter < 2; iter++)
                {
                    for (edge0.zero(), loop = 20;; ) // iter==0 - trace cw, 1 - ccw
                    {
                        edge = (pmd->pVertices[pmd->pIndices[itri * 3 + inc_mod3[iedge]]] - pmd->pVertices[pmd->pIndices[itri * 3 + iedge]]) * (1.f - iter * 2.f);
                        n += (edge0 ^ edge) * (iter * 2.f - 1.f);
                        edge0 = edge;
                        if (sqr(t * edge) * tedgeDenom > tedge * edge.len2())
                        {
                            tedge = sqr(t * edge), tedgeDenom = edge.len2(), itriT = itri, iedgeT = iedge;
                        }
                        itri1 = pmd->pTopology[itri].ibuddy[iedge];
                        if (itri1 == ctd[i].ivtxT || itri1 < 0 || --loop < 0)
                        {
                            break;
                        }
                        iedge = GetEdgeByBuddy(pmd, itri1, itri) + 1 + iter;
                        itri = itri1;
                        iedge -= 3 & (2 - iedge) >> 31;
                    }
                    if (itri1 >= 0 && iter == 0)
                    {
                        n.zero();
                    }
                    itri = ctd[i].ivtxT;
                    iedge = dec_mod3[ctd[i].sgnNorm];
                }
                n += pmd->pVertices[pmd->pIndices[ctd[i].ivtxT * 3 + 1]] - pmd->pVertices[pmd->pIndices[ctd[i].ivtxT * 3]] ^
                    pmd->pVertices[pmd->pIndices[ctd[i].ivtxT * 3 + 2]] - pmd->pVertices[pmd->pIndices[ctd[i].ivtxT * 3]];

                if ((ctd[i].ivtxT = pmd->pIndices[itriT * 3 + iedgeT]) == j)
                {
                    ctd[i].ivtxT = pmd->pIndices[itriT * 3 + inc_mod3[iedgeT]];
                }
                edge = (pmd->pVertices[ctd[i].ivtxT] - pmd->pVertices[j]).normalized();

                ctd[i].edge.Set(edge * t, edge * b, edge * s);
                ctd[i].sgnNorm = sgnnz(n * s);
            }
            if (pObj != this)
            {
                memcpy(pObj->m_pClothTangentsData = new SClothTangentVtx[nVtxFull], ctd, nVtxFull * sizeof(SClothTangentVtx));
                pObj->m_hasClothTangentsData = 1;
            }
            pObj->SetFlags(pObj->GetFlags() & ~STATIC_OBJECT_CANT_BREAK);
            m_pRenderMesh->UnLockForThreadAccess();
        }

        if (GetTetrLattice() || m_hasSkinInfo)
        {
            Vec3 sz = GetAABB().GetSize();
            float szmin = min(min(sz.x, sz.y), sz.z), szmax = max(max(sz.x, sz.y), sz.z), szmed = sz.x + sz.y + sz.y - szmin - szmax;
            if (!m_hasSkinInfo)
            {
                PrepareSkinData(Matrix34(IDENTITY), 0, min(szmin * 0.5f, szmed * 0.15f));
            }
            if (pVtx)
            {
                return SkinVertices(pVtx, Matrix34(IDENTITY));
            }
            return pObj;
        }

        if (!pVtx)
        {
            return pObj;
        }

        if (!(GetFlags() & STATIC_OBJECT_CLONE))
        {
            pObj = (CStatObj*)Clone(true, true, false);
            pObj->m_pRenderMesh->KeepSysMesh(true);
        }

        IRenderMesh* mesh = pObj->m_pRenderMesh;
        mesh->LockForThreadAccess();
        pMeshVtx.data = (Vec3*)((mesh = pObj->m_pRenderMesh)->GetPosPtr(pMeshVtx.iStride, FSL_SYSTEM_UPDATE));
        if (m_hasClothTangentsData && m_pClothTangentsData)
        {
            pTangents.data = (SPipTangents*)mesh->GetTangentPtr(pTangents.iStride, FSL_SYSTEM_UPDATE);
        }

        if (!m_pAsyncUpdateContext)
        {
            m_pAsyncUpdateContext = new SSyncToRenderMeshContext;
        }
        else
        {
            m_pAsyncUpdateContext->jobExecutor.WaitForCompletion();
        }
        m_pAsyncUpdateContext->Set(&pObj->m_vBoxMin,    &pObj->m_vBoxMax,   iVtx0, nVtx, pVtx, pVtxMap, mask, rscale
            , m_pClothTangentsData, pMeshVtx, pTangents, pNormals, pObj);

        if (GetCVars()->e_RenderMeshUpdateAsync)
        {
            SSyncToRenderMeshContext* pAsyncUpdateContext = m_pAsyncUpdateContext;
            volatile int* updateState = mesh->SetAsyncUpdateState();
            m_pAsyncUpdateContext->jobExecutor.StartJob([pAsyncUpdateContext, updateState]()
            {
                SyncToRenderMesh(pAsyncUpdateContext, updateState);
            });
        }
        else
        {
            SyncToRenderMesh(m_pAsyncUpdateContext, NULL);
            if (m_hasClothTangentsData && m_pClothTangentsData)
            {
                mesh->UnlockStream(VSF_TANGENTS);
            }
            mesh->UnlockStream(VSF_GENERAL);
        }
        mesh->UnLockForThreadAccess();
    }
    return pObj;
}


//////////////////////////////////////////////////////////////////////////
namespace
{
    CryCriticalSection g_cPrepareSkinData;
}

void CStatObj::PrepareSkinData(const Matrix34& mtxSkelToMesh, IGeometry* pPhysSkel, float r)
{
    if (m_hasSkinInfo || pPhysSkel && pPhysSkel->GetType() != GEOM_TRIMESH)
    {
        return;
    }

    // protect again possible paralle calls, if streaming is here, but mainthread also reaches this function
    // before streaming thread has finished and wants to prepare data also
    AUTO_LOCK(g_cPrepareSkinData);

    // recheck again to guard again creating the object two times
    if (m_hasSkinInfo)
    {
        return;
    }

    m_nFlags |= STATIC_OBJECT_DYNAMIC;
    if (!m_pRenderMesh)
    {
        if (!m_pDelayedSkinParams)
        {
            m_pDelayedSkinParams = new SDelayedSkinParams;
            m_pDelayedSkinParams->mtxSkelToMesh = mtxSkelToMesh;
            m_pDelayedSkinParams->pPhysSkel = pPhysSkel;
            m_pDelayedSkinParams->r = r;
        }
        return;
    }
    m_pRenderMesh->KeepSysMesh(true);

    int i, j, nVtx;
    Vec3 vtx[4];
    geom_world_data gwd[2];
    geom_contact* pcontact;
    mesh_data* md;
    // two spheres for checking intersections against skeleton
    CRY_PHYSICS_REPLACEMENT_ASSERT();
    AZ_UNUSED(pcontact);
    AZ_UNUSED(j);
    strided_pointer<Vec3> pVtx;
    Matrix34 mtxMeshToSkel = mtxSkelToMesh.GetInverted();
    ITetrLattice* pLattice = GetTetrLattice();
    if (pLattice)
    {
        pPhysSkel = pLattice->CreateSkinMesh();
    }

    gwd[1].scale = mtxSkelToMesh.GetColumn0().len();
    gwd[1].offset = mtxSkelToMesh.GetTranslation();
    gwd[1].R = Matrix33(mtxSkelToMesh) * (1.0f / gwd[1].scale);
    m_pRenderMesh->GetBBox(vtx[0], vtx[1]);
    vtx[1] -= vtx[0];
    primitives::sphere sph;
    sph.center.zero();
    sph.r = r > 0.0f ? r : min(min(vtx[1].x, vtx[1].y), vtx[1].z);
    sph.r *= 3.0f;
    assert(pPhysSkel);
    PREFAST_ASSUME(pPhysSkel);
    md = (mesh_data*)pPhysSkel->GetData();
    IRenderMesh::ThreadAccessLock lockrm(m_pRenderMesh);
    pVtx.data = (Vec3*)m_pRenderMesh->GetPosPtr(pVtx.iStride, FSL_READ);
    SSkinVtx* pSkinInfo = m_pSkinInfo = new SSkinVtx[nVtx = m_pRenderMesh->GetVerticesCount()];
    m_hasSkinInfo = 1;

    for (i = 0; i < nVtx; i++)
    {
        Vec3 v = pVtx[i];
        if (pSkinInfo[i].bVolumetric = (pLattice && pLattice->CheckPoint(mtxMeshToSkel * v, pSkinInfo[i].idx, pSkinInfo[i].w)))
        {
            pSkinInfo[i].M = Matrix33(md->pVertices[pSkinInfo[i].idx[1]] - md->pVertices[pSkinInfo[i].idx[0]],
                    md->pVertices[pSkinInfo[i].idx[2]] - md->pVertices[pSkinInfo[i].idx[0]],
                    md->pVertices[pSkinInfo[i].idx[3]] - md->pVertices[pSkinInfo[i].idx[0]]).GetInverted();
        }
        else
        {
            gwd[0].offset = v;
        }
    }
    if (pLattice)
    {
        pPhysSkel->Release();
    }
}

IStatObj* CStatObj::SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34& mtxSkelToMesh)
{
    if (!m_hasSkinInfo && m_pDelayedSkinParams)
    {
        PrepareSkinData(m_pDelayedSkinParams->mtxSkelToMesh, m_pDelayedSkinParams->pPhysSkel, m_pDelayedSkinParams->r);
        if (m_hasSkinInfo)
        {
            delete m_pDelayedSkinParams, m_pDelayedSkinParams = 0;
        }
    }
    if (!m_pRenderMesh || !m_hasSkinInfo)
    {
        return this;
    }
    CStatObj* pObj = this;
    if (!(GetFlags() & STATIC_OBJECT_CLONE))
    {
        pObj = (CStatObj*)Clone(true, true, false);
    }
    if (!pObj->m_pClonedSourceObject || !pObj->m_pClonedSourceObject->m_pRenderMesh)
    {
        return pObj;
    }

    strided_pointer<Vec3> pVtx;
    strided_pointer<SPipTangents> pTangents, pTangents0;
    Vec3 vtx[4], t, b;
    Matrix33 M;
    AABB bbox;
    bbox.Reset();
    int i, j, nVtx;
    SSkinVtx* pSkinInfo = m_pSkinInfo;
    pObj->m_pRenderMesh->LockForThreadAccess();
    pObj->m_pClonedSourceObject->m_pRenderMesh->LockForThreadAccess();

    pVtx.data = (Vec3*)pObj->m_pRenderMesh->GetPosPtr(pVtx.iStride, FSL_SYSTEM_UPDATE);
    pTangents.data = (SPipTangents*)pObj->m_pRenderMesh->GetTangentPtr(pTangents.iStride, FSL_SYSTEM_UPDATE);
    pTangents0.data = (SPipTangents*)pObj->m_pClonedSourceObject->m_pRenderMesh->GetTangentPtr(pTangents0.iStride, FSL_READ);
    nVtx = pObj->m_pRenderMesh->GetVerticesCount();
    if (!pVtx.data)
    {
        nVtx = 0;
    }
    const bool canUseTangents = pTangents.data && pTangents0.data;
    for (i = 0; i < nVtx; i++)
    {
        Vec3 v3 = pVtx[i];
        if (pSkinInfo[i].idx[0] >= 0)
        {
            for (j = 0, v3.zero(); j < 3 + pSkinInfo[i].bVolumetric; j++)
            {
                v3 += pSkinInfo[i].w[j] * (vtx[j] = mtxSkelToMesh * pSkelVtx[pSkinInfo[i].idx[j]]);
            }
            if (!pSkinInfo[i].bVolumetric)
            {
                Vec3 n = (vtx[1] - vtx[0] ^ vtx[2] - vtx[0]).normalized();
                v3 += n * pSkinInfo[i].w[3];
                Vec3 edge = (vtx[1] + vtx[2] - vtx[0] * 2).normalized();
                M = Matrix33(edge, n ^ edge, n);
            }
            else
            {
                M = Matrix33(vtx[1] - vtx[0], vtx[2] - vtx[0], vtx[3] - vtx[0]);
            }
            M *= pSkinInfo[i].M;
            if (canUseTangents)
            {
                SMeshTangents tb(pTangents0[i]);

                tb.RotateBy(M);
                tb.ExportTo(pTangents0[i]);
            }
            pVtx[i] = v3;
        }
        bbox.Add(v3);
    }
    pObj->m_pRenderMesh->UnlockStream(VSF_GENERAL);
    pObj->m_pRenderMesh->UnlockStream(VSF_TANGENTS);
    pObj->m_pClonedSourceObject->m_pRenderMesh->UnlockStream(VSF_TANGENTS);
    pObj->m_pClonedSourceObject->m_pRenderMesh->UnLockForThreadAccess();
    pObj->m_pRenderMesh->UnLockForThreadAccess();
    pObj->m_vBoxMin = bbox.min;
    pObj->m_vBoxMax = bbox.max;
    return pObj;
}

//////////////////////////////////////////////////////////////////////////

int CStatObj::Physicalize(IPhysicalEntity* pent, pe_geomparams* pgp, int id, const char* szPropsOverride)
{
    int res = -1;
    if (GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        Matrix34 mtxId(IDENTITY);
        res = PhysicalizeSubobjects(pent, pgp->pMtx3x4 ? pgp->pMtx3x4 : &mtxId, pgp->mass, pgp->density, id, 0, szPropsOverride);
    }

    {
        int i, nNoColl, iNoColl = 0;
        float V;
        if (pgp->mass < 0 && pgp->density < 0)
        {
            GetPhysicalProperties(pgp->mass, pgp->density);
        }
        for (i = m_arrPhysGeomInfo.GetGeomCount() - 1, nNoColl = 0, V = 0.0f; i >= 0; i--)
        {
            if (m_arrPhysGeomInfo.GetGeomType(i) == PHYS_GEOM_TYPE_DEFAULT)
            {
                V += m_arrPhysGeomInfo[i]->V;
            }
            else
            {
                iNoColl = i, ++nNoColl;
            }
        }
        int flags0 = pgp->flags;
        ISurfaceTypeManager* pSurfaceMan = Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager();
        if (phys_geometry* pSolid = m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
        {
            if (pSolid->surface_idx < pSolid->nMats)
            {
                if (ISurfaceType* pMat = pSurfaceMan->GetSurfaceType(pSolid->pMatMapping[pSolid->surface_idx]))
                {
                    if (pMat->GetPhyscalParams().collType >= 0)
                    {
                        (pgp->flags &= ~(geom_collides | geom_floats)) |= pMat->GetPhyscalParams().collType;
                    }
                }
            }
        }
        if (pgp->mass > pgp->density && V > 0.0f)   // mass is set instead of density and V is valid
        {
            pgp->density = pgp->mass / V, pgp->mass = -1.0f;
        }
        (pgp->flags &= ~geom_colltype_explosion) |= geom_colltype_explosion & ~-(int)m_bDontOccludeExplosions;
        (pgp->flags &= ~geom_manually_breakable) |= geom_manually_breakable &  - (int)m_bBreakableByGame;
        if (m_nFlags & STATIC_OBJECT_NO_PLAYER_COLLIDE)
        {
            pgp->flags &= ~geom_colltype_player;
        }
        if (m_nSpines && m_arrPhysGeomInfo.GetGeomCount() - nNoColl <= 1 &&
            (nNoColl == 1 || nNoColl == 2 && m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE] && m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT]))
        {
            pe_params_structural_joint psj;
            bool bHasJoints = false;
            if (m_arrPhysGeomInfo.GetGeomCount() > nNoColl)
            {
                if (nNoColl && m_pParentObject && m_pParentObject != this)
                {
                    CStatObj* pParent;
                    for (pParent = m_pParentObject; pParent->m_pParentObject; pParent = pParent->m_pParentObject)
                    {
                        ;
                    }
                    bHasJoints = pParent->FindSubObject_StrStr("$joint") != 0;
                    psj.partid[0] = id;
                    psj.pt = m_arrPhysGeomInfo[iNoColl]->origin;
                    psj.bBreakable = 0;
                }
                res = pent->AddGeometry(m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT], pgp, id);
                id += 1024;
            }
            pgp->minContactDist = 1.0f;
            pgp->density = 5.0f;
            if (nNoColl == 1)
            {
                pgp->flags = geom_log_interactions | geom_squashy;
                pgp->flags |= geom_colltype_foliage_proxy;
                res = pent->AddGeometry(m_arrPhysGeomInfo[iNoColl], pgp, psj.partid[1] = id);
                if (bHasJoints)
                {
                    pent->SetParams(&psj);
                }
            }
            else
            {
                pgp->flags = geom_squashy | geom_colltype_obstruct;
                pent->AddGeometry(m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT], pgp, psj.partid[1] = id);
                id += 1024;
                if (bHasJoints)
                {
                    pent->SetParams(&psj);
                }
                pgp->flags = geom_log_interactions | geom_colltype_foliage_proxy;
                int flagsCollider = pgp->flagsCollider;
                pgp->flagsCollider = 0;
                pent->AddGeometry(m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE], pgp, psj.partid[1] = id);
                pgp->flagsCollider = flagsCollider;
                if (bHasJoints)
                {
                    pent->SetParams(&psj);
                }
            }
        }
        else if (nNoColl == 1 && m_arrPhysGeomInfo.GetGeomCount() == 2)
        {   // one solid, one obstruct or nocoll proxy -> use single part with ray proxy
            res = pent->AddGeometry(m_arrPhysGeomInfo[iNoColl], pgp, id);
            pgp->flags |= geom_proxy;
            pent->AddGeometry(m_arrPhysGeomInfo[iNoColl ^ 1], pgp, res);
            pgp->flags &= ~geom_proxy;
        }
        else
        {   // add all solid and non-colliding geoms as individual parts
            for (i = 0; i < m_arrPhysGeomInfo.GetGeomCount(); i++)
            {
                if (m_arrPhysGeomInfo.GetGeomType(i) == PHYS_GEOM_TYPE_DEFAULT)
                {
                    res = pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id += 1024;
                }
            }
            pgp->idmatBreakable = -1;
            for (i = 0; i < m_arrPhysGeomInfo.GetGeomCount(); i++)
            {
                if (m_arrPhysGeomInfo.GetGeomType(i) == PHYS_GEOM_TYPE_NO_COLLIDE)
                {
                    pgp->flags = geom_colltype_ray, res = pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id += 1024;
                }
                else if (m_arrPhysGeomInfo.GetGeomType(i) == PHYS_GEOM_TYPE_OBSTRUCT)
                {
                    pgp->flags = geom_colltype_obstruct, res = pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id += 1024;
                }
            }
        }
        pgp->flags = flags0;

        if (m_arrPhysGeomInfo.GetGeomCount() >= 10 && pent->GetType() == PE_STATIC)
        {
            pe_params_flags pf;
            pf.flagsOR = pef_parts_traceable;
            pf.flagsAND = ~pef_traceable;
            pent->SetParams(&pf);
        }
    }
    return res;
}

#define isalpha(c) isalpha((unsigned char)c)
#define isdigit(c) isdigit((unsigned char)c)

int CStatObj::PhysicalizeSubobjects(IPhysicalEntity* pent, const Matrix34* pMtx, float mass, float density, int id0, strided_pointer<int> pJointsIdMap, const char* szPropsOverride)
{
    int i, j, i0, i1, len = 0, len1, nObj = GetSubObjectCount(), ipart[2], nparts, bHasPlayerOnlyGeoms = 0, nGeoms = 0, id, idNext = 0;
    float V[2], M, scale, jointsz;
    const char* pval, * properties;
    bool bHasSkeletons = false, bAutoJoints = false;
    Matrix34 mtxLoc;
    Vec3 dist;
    primitives::box joint_bbox;
    IGeometry* pJointBox = 0;

    primitives::box bbox;
    IStatObj::SSubObject* pSubObj, * pSubObj1;
    pe_articgeomparams partpos;
    pe_params_structural_joint psj;
    pe_params_flags pf;
    geom_world_data gwd;
    intersection_params ip;
    geom_contact* pcontacts;
    scale = pMtx ? pMtx->GetColumn(0).len() : 1.0f;
    ip.bStopAtFirstTri = ip.bNoBorder = true;

    bbox.Basis.SetIdentity();
    bbox.size.Set(0.5f, 0.5f, 0.5f);
    bbox.center.zero();
    joint_bbox = bbox;
    pf.flagsOR = 0;

    for (i = 0; i < nObj; i++)
    {
        if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() &&
            pSubObj->bHidden &&
            (!strncmp(pSubObj->name, "childof_", 8) && (pSubObj1 = FindSubObject((const char*)pSubObj->name + 8)) ||
             (pSubObj1 = GetSubObject(pSubObj->nParent)) && strstr(pSubObj1->properties, "group")))
        {
            pSubObj->bHidden = pSubObj1->bHidden;
        }
    }

    for (i = 0, V[0] = V[1] = M = 0; i < nObj; i++)
    {
        if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden && strncmp(pSubObj->name, "skeleton_", 9))
        {
            float mi = -1.0f, dens = -1.0f, Vsubobj;
            for (j = 0, Vsubobj = 0.0f; pSubObj->pStatObj->GetPhysGeom(j); j++)
            {
                if (((CStatObj*)pSubObj->pStatObj)->m_arrPhysGeomInfo.GetGeomType(j) == PHYS_GEOM_TYPE_DEFAULT)
                {
                    Vsubobj += pSubObj->pStatObj->GetPhysGeom(j)->V;
                }
            }
            pSubObj->pStatObj->GetPhysicalProperties(mi, dens);
            if (dens > 0)
            {
                mi = Vsubobj * cube(scale) * dens; // Calc mass.
            }
            if (mi != 0.0f)
            {
                V[isneg(mi)] += Vsubobj * cube(scale);
            }
            M += max(0.0f, mi);

            if (((CStatObj*)pSubObj->pStatObj)->m_nRenderTrisCount <= 0 &&
                pSubObj->pStatObj->GetPhysGeom()->pGeom->GetForeignData() == pSubObj->pStatObj &&
                strstr(pSubObj->properties, "other_rendermesh"))
            {
                Vec3 center = pSubObj->localTM * (((CStatObj*)pSubObj->pStatObj)->m_vBoxMin + ((CStatObj*)pSubObj->pStatObj)->m_vBoxMax) * 0.5f;
                float mindist = 1e10f, curdist;
                for (j = 0, i0 = i; j < nObj; j++)
                {
                    if (j != i && (pSubObj1 = GetSubObject(j)) && pSubObj1->pStatObj && ((CStatObj*)pSubObj1->pStatObj)->m_nRenderTrisCount > 0 &&
                        (curdist = (pSubObj1->localTM * (((CStatObj*)pSubObj1->pStatObj)->m_vBoxMin +
                                                         ((CStatObj*)pSubObj1->pStatObj)->m_vBoxMax) * 0.5f - center).len2()) < mindist)
                    {
                        mindist = curdist;
                        i0 = j;
                    }
                }
                pSubObj->pStatObj->GetPhysGeom()->pGeom->SetForeignData(GetSubObject(i0)->pStatObj, 0);
            }
        }
    }
    for (i = 0; i < nObj; i++)
    {
        GetSubObject(i)->nBreakerJoints = 0;
    }
    if (mass <= 0)
    {
        mass = M * density;
    }
    if (density <= 0)
    {
        if ((V[0] + V[1]) != 0)
        {
            density = mass / (V[0] + V[1]);
        }
        else
        {
            density = 1000.0f; // Some default.
        }
    }
    partpos.flags = geom_collides | geom_floats;

    for (i = 0; i < nObj; i++)
    {
        if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() &&
            !pSubObj->bHidden && strcmp(pSubObj->name, "colltype_player") == 0)
        {
            bHasPlayerOnlyGeoms = 1;
        }
    }

    for (i = 0; i < nObj; i++)
    {
        if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && ((CStatObj*)pSubObj->pStatObj)->m_arrPhysGeomInfo.GetGeomCount() && !pSubObj->bHidden)
        {
            if (pJointsIdMap)
            {
                continue;
            }
            partpos.idbody = i + id0;
            partpos.pMtx3x4 = pMtx ? &(mtxLoc = *pMtx * pSubObj->tm) : &pSubObj->tm;

            float mi = 0, di = 0;
            if (pSubObj->pStatObj->GetPhysicalProperties(mi, di))
            {
                if (mi >= 0)
                {
                    partpos.mass = mi, partpos.density = 0;
                }
                else
                {
                    partpos.mass = 0, partpos.density = di;
                }
            }
            else
            {
                partpos.density = density;
            }
            if (GetCVars()->e_ObjQuality != CONFIG_LOW_SPEC)
            {
                partpos.idmatBreakable = ((CStatObj*)pSubObj->pStatObj)->m_idmatBreakable;
                if (((CStatObj*)pSubObj->pStatObj)->m_bVehicleOnlyPhysics)
                {
                    partpos.flags = geom_colltype6;
                }
                else
                {
                    partpos.flags = (geom_colltype_solid & ~(geom_colltype_player & - bHasPlayerOnlyGeoms)) | geom_colltype_ray | geom_floats | geom_colltype_explosion;
                    if (bHasPlayerOnlyGeoms && strcmp(pSubObj->name, "colltype_player") == 0)
                    {
                        partpos.flags = geom_colltype_player;
                    }
                }
            }
            else
            {
                partpos.idmatBreakable = -1;
                if (((CStatObj*)pSubObj->pStatObj)->m_bVehicleOnlyPhysics)
                {
                    partpos.flags = geom_colltype6;
                }
            }
            if (!strncmp(pSubObj->name, "skeleton_", 9))
            {
                if (!GetCVars()->e_DeformableObjects)
                {
                    continue;
                }
                bHasSkeletons = true;
                if (mi <= 0.0f)
                {
                    partpos.mass = 1.0f, partpos.density = 0.0f;
                }
            }
            partpos.flagsCollider &= ~geom_destroyed_on_break;
            if (strstr(pSubObj->properties, "pieces"))
            {
                partpos.flagsCollider |= geom_destroyed_on_break;
            }
            if (strstr(pSubObj->properties, "noselfcoll"))
            {
                partpos.flags &= ~(partpos.flagsCollider = geom_colltype_debris);
            }
            if ((id = pSubObj->pStatObj->Physicalize(pent, &partpos, i + id0)) >= 0)
            {
                nGeoms++, idNext = id + 1;
            }
            if (strstr(pSubObj->properties, "force_joint"))
            {
                pe_params_structural_joint psj1;
                psj1.id = 1024 + i;
                psj1.partid[0] = i + id0;
                psj1.partid[1] = pSubObj->nParent + id0;
                psj1.bBreakable = 0;
                psj1.pt = *partpos.pMtx3x4 * (pSubObj->pStatObj->GetBoxMin() + pSubObj->pStatObj->GetBoxMax()) * 0.5f;
                pent->SetParams(&psj1);
            }
        }
        else
        if (pSubObj->nType == STATIC_SUB_OBJECT_DUMMY && !strncmp(pSubObj->name, "$joint", 6))
        {
            properties = szPropsOverride ? szPropsOverride : (const char*)pSubObj->properties;
            psj.pt = pSubObj->tm.GetTranslation();
            psj.n = pSubObj->tm.GetColumn(2).normalized();
            psj.axisx = pSubObj->tm.GetColumn(0).normalized();
            float maxdim = max(pSubObj->helperSize.x, pSubObj->helperSize.y);
            maxdim = max(maxdim, pSubObj->helperSize.z);
            psj.szSensor = jointsz = maxdim * pSubObj->tm.GetColumn(0).len();
            psj.partidEpicenter = -1;
            psj.bBroken = 0;
            psj.id = i;
            psj.bReplaceExisting = 1;
            if (pSubObj->name[6] != ' ')
            {
                gwd.offset = psj.pt;
                ipart[1] = nObj;
                for (i0 = nparts = 0; i0 < nObj && nparts < 2; i0++)
                {
                    if ((pSubObj1 = GetSubObject(i0))->nType == STATIC_SUB_OBJECT_MESH && pSubObj1->pStatObj && pSubObj1->pStatObj->GetPhysGeom() &&
                        strncmp(pSubObj1->name, "skeleton_", 9) && !strstr(pSubObj1->properties, "group"))
                    {
                        gwd.offset = pSubObj1->tm.GetInverted() * psj.pt;
                        pSubObj1->pStatObj->GetPhysGeom()->pGeom->GetBBox(&bbox);
                        dist = bbox.Basis * (gwd.offset - bbox.center);
                        for (j = 0; j < 3; j++)
                        {
                            dist[j] = max(0.0f, fabs_tpl(dist[j]) - bbox.size[j]);
                        }
                        gwd.scale = jointsz;
                        if (fabs_tpl(pSubObj1->tm.GetColumn(0).len2() - 1.0f) > 0.01f)
                        {
                            gwd.scale /= pSubObj1->tm.GetColumn(0).len();
                        }

                        // Make a geometry box for intersection.
                        if (!pJointBox)
                        {
                            // Create box for joint
                            CRY_PHYSICS_REPLACEMENT_ASSERT();
                        }
                        {
                            WriteLockCond lockColl;
                            if (dist.len2() < sqr(gwd.scale * 0.5f) && pSubObj1->pStatObj->GetPhysGeom()->pGeom->IntersectLocked(pJointBox, 0, &gwd, &ip, pcontacts, lockColl))
                            {
                                ipart[nparts++] = i0;
                            }
                        }   // lock
                    }
                }
                if (nparts == 0)
                {
                    continue;
                }
                GetSubObject(ipart[0])->pStatObj->GetPhysGeom()->pGeom->GetBBox(&bbox);
                gwd.offset = GetSubObject(ipart[0])->tm * bbox.center;
                j = isneg((gwd.offset - psj.pt) * psj.n);
                i0 = ipart[j];
                i1 = ipart[1 ^ j];

                if (strlen((const char*)pSubObj->name) >= 7 && !strncmp((const char*)pSubObj->name + 7, "sample", 6))
                {
                    psj.bBroken = 2;
                }
                else if (strlen((const char*)pSubObj->name) >= 7 && !strncmp((const char*)pSubObj->name + 7, "impulse", 7))
                {
                    psj.bBroken = 2, psj.id = joint_impulse, psj.bReplaceExisting = 0, bAutoJoints = true;
                }
            }
            else
            {
                for (i0 = 0; i0 < nObj && ((pSubObj1 = GetSubObject(i0))->nType != STATIC_SUB_OBJECT_MESH ||
                                           (strlen((const char*)pSubObj->name) >= 7 && (strncmp((const char*)pSubObj->name + 7, pSubObj1->name, len = strlen(pSubObj1->name)) || isalpha(pSubObj->name[7 + len])))); i0++)
                {
                    ;
                }
                for (i1 = 0; i1 < nObj && ((pSubObj1 = GetSubObject(i1))->nType != STATIC_SUB_OBJECT_MESH ||
                                           (strlen((const char*)pSubObj->name) >= 8 && (strncmp((const char*)pSubObj->name + 8 + len, pSubObj1->name, len1 = strlen(pSubObj1->name)) || isalpha(pSubObj->name[8 + len + len1])))); i1++)
                {
                    ;
                }
                if (i0 >= nObj && i1 >= nObj)
                {
                    CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Error: cannot resolve part names in %s (%s)", (const char*)pSubObj->name, (const char*)m_szFileName);
                }
            }
            if (pJointsIdMap)
            {
                i0 = pJointsIdMap[i0], i1 = pJointsIdMap[i1];
            }
            psj.partid[0] = i0 + id0;
            psj.partid[1] = i1 + id0;
            psj.maxForcePush = psj.maxForcePull = psj.maxForceShift = psj.maxTorqueBend = psj.maxTorqueTwist = 1E20f;
            if (pMtx)
            {
                psj.pt = *pMtx * psj.pt;
                psj.n = pMtx->TransformVector(psj.n).normalized();
                psj.axisx = pMtx->TransformVector(psj.axisx).normalized();
            }

            if ((pval = strstr(properties, "limit")) && (pval - 11 < properties - 11 || strncmp(pval - 11, "constraint_", 11)))
            {
                for (pval += 5; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxTorqueBend = aznumeric_caster(atof(pval));
                }
                //psj.maxTorqueTwist = psj.maxTorqueBend*5;
                psj.maxForcePull = psj.maxTorqueBend;
                psj.maxForceShift = psj.maxTorqueBend;
                //              psj.maxForcePush = psj.maxForcePull*2.5f;
                psj.bBreakable = 1;
            }
            if (pval = strstr(properties, "twist"))
            {
                for (pval += 5; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxTorqueTwist = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "bend"))
            {
                for (pval += 4; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxTorqueBend = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "push"))
            {
                for (pval += 4; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxForcePush = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "pull"))
            {
                for (pval += 4; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxForcePull = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "shift"))
            {
                for (pval += 5; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.maxForceShift = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "damage_accum"))
            {
                for (pval += 5; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.damageAccum = aznumeric_caster(atof(pval));
                }
            }
            if (pval = strstr(properties, "damage_accum_threshold"))
            {
                for (pval += 5; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.damageAccumThresh = aznumeric_caster(atof(pval));
                }
            }
            if (psj.maxForcePush + psj.maxForcePull + psj.maxForceShift + psj.maxTorqueBend + psj.maxTorqueTwist > 4.9E20f)
            {
                if (azsscanf(properties, "%f %f %f %f %f",
                        &psj.maxForcePush, &psj.maxForcePull, &psj.maxForceShift, &psj.maxTorqueBend, &psj.maxTorqueTwist) == 5)
                {
                    psj.maxForcePush *= density;
                    psj.maxForcePull *= density;
                    psj.maxForceShift *= density;
                    psj.maxTorqueBend *= density;
                    psj.maxTorqueTwist *= density;
                    psj.bBreakable = 1;
                }
                else
                {
                    psj.bBreakable = 0;
                }
            }
            psj.bDirectBreaksOnly = strstr(properties, "hits_only") != 0;
            psj.limitConstraint.zero();
            psj.bConstraintWillIgnoreCollisions = 1;
            if ((len = 16, pval = strstr(properties, "constraint_limit")) || (len = 5, pval = strstr(properties, "C_lmt")))
            {
                for (pval += len; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.limitConstraint.z = aznumeric_caster(atof(pval));
                }
            }
            if ((len = 17, pval = strstr(properties, "constraint_minang")) || (len = 5, pval = strstr(properties, "C_min")))
            {
                for (pval += len; *pval && !isdigit(*pval) && *pval != '-'; pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.limitConstraint.x = aznumeric_caster(DEG2RAD(atof(pval)));
                }
            }
            if ((len = 17, pval = strstr(properties, "constraint_maxang")) || (len = 5, pval = strstr(properties, "C_max")))
            {
                for (pval += len; *pval && !isdigit(*pval) && *pval != '-'; pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.limitConstraint.y = aznumeric_caster(DEG2RAD(atof(pval)));
                }
            }
            if ((len = 18, pval = strstr(properties, "constraint_damping")) || (len = 5, pval = strstr(properties, "C_dmp")))
            {
                for (pval += len; *pval && !isdigit(*pval); pval++)
                {
                    ;
                }
                if (pval)
                {
                    psj.dampingConstraint = aznumeric_caster(atof(pval));
                }
            }
            if (strstr(properties, "constraint_collides") || strstr(properties, "C_coll"))
            {
                psj.bConstraintWillIgnoreCollisions = 0;
            }
            scale = GetFloatCVar(e_JointStrengthScale);
            psj.maxForcePush *= scale;
            psj.maxForcePull *= scale;
            psj.maxForceShift *= scale;
            psj.maxTorqueBend *= scale;
            psj.maxTorqueTwist *= scale;
            psj.limitConstraint.z *= scale;
            pent->SetParams(&psj);
            if (!gEnv->bMultiplayer && strstr(properties, "gameplay_critical"))
            {
                pf.flagsOR |= pef_override_impulse_scale;
            }
            if (gEnv->bMultiplayer && strstr(properties, "mp_break_always"))
            {
                pf.flagsOR |= pef_override_impulse_scale;
            }
            if (strstr(properties, "player_can_break"))
            {
                pf.flagsOR |= pef_players_can_break;
            }

            if (strstr(pSubObj->properties, "breaker"))
            {
                if (GetSubObject(i0))
                {
                    GetSubObject(i0)->nBreakerJoints++;
                }
                if (GetSubObject(i1))
                {
                    GetSubObject(i1)->nBreakerJoints++;
                }
            }
        }
    }

    if (bAutoJoints)
    {
        psj.idx = -2; // tels the physics to try and generate joints
        pent->SetParams(&psj);
    }

    pe_params_part pp;
    if (bHasSkeletons)
    {
        for (i = 0; i < nObj; i++)
        {
            if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden &&
                !strncmp(pSubObj->name, "skeleton_", 9) && (pSubObj1 = FindSubObject((const char*)pSubObj->name + 9)))
            {
                pe_params_skeleton ps;
                properties = szPropsOverride ? szPropsOverride : (const char*)pSubObj->properties;
                if (pval = strstr(properties, "stiffness"))
                {
                    for (pval += 9; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.stiffness = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "thickness"))
                {
                    for (pval += 9; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.thickness = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "max_stretch"))
                {
                    for (pval += 11; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.maxStretch = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "max_impulse"))
                {
                    for (pval += 11; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.maxImpulse = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "skin_dist"))
                {
                    for (pval += 9; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        pp.minContactDist = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "hardness"))
                {
                    for (pval += 8; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.hardness = aznumeric_caster(atof(pval));
                    }
                }
                if (pval = strstr(properties, "explosion_scale"))
                {
                    for (pval += 15; *pval && !isdigit(*pval); pval++)
                    {
                        ;
                    }
                    if (pval)
                    {
                        ps.explosionScale = aznumeric_caster(atof(pval));
                    }
                }

                pp.partid = ps.partid = aznumeric_cast<int>((pSubObj1 - &m_subObjects[0])) + id0;
                pp.idSkeleton = i + id0;
                pent->SetParams(&pp);
                pent->SetParams(&ps);
                ((CStatObj*)pSubObj1->pStatObj)->PrepareSkinData(pSubObj1->localTM.GetInverted() * pSubObj->localTM, pSubObj->pStatObj->GetPhysGeom()->pGeom,
                    is_unused(pp.minContactDist) ? 0.0f : pp.minContactDist);
            }
        }
    }

    new(&pp)pe_params_part;
    for (i = 0; i < nObj; i++)
    {
        if ((pSubObj = GetSubObject(i))->nType == STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden &&
            (!strncmp(pSubObj->name, "childof_", 8) && (pSubObj1 = FindSubObject((const char*)pSubObj->name + 8)) ||
             (pSubObj1 = GetSubObject(pSubObj->nParent)) && strstr(pSubObj1->properties, "group")))
        {
            pp.partid = i + id0;
            pp.idParent = aznumeric_caster(pSubObj1 - &m_subObjects[0]);
            pent->SetParams(&pp);
            pSubObj->bHidden = true;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Iterate through sub objects and update hide mask.
    //////////////////////////////////////////////////////////////////////////
    m_nInitialSubObjHideMask = 0;
    for (size_t a = 0, numsub = m_subObjects.size(); a < numsub; a++)
    {
        SSubObject& subObj = m_subObjects[a];
        if (subObj.pStatObj && subObj.nType == STATIC_SUB_OBJECT_MESH && subObj.bHidden)
        {
            m_nInitialSubObjHideMask |= ((uint64)1 << a);
        }
    }

    if (nGeoms >= 10 && pent->GetType() == PE_STATIC)
    {
        pf.flagsOR |= pef_parts_traceable, pf.flagsAND = ~pef_traceable;
    }

    if (pf.flagsOR)
    {
        pent->SetParams(&pf);
    }
    if (pJointBox)
    {
        pJointBox->Release();
    }
    return idNext - 1;
}

//static const int g_nRanges = 5;
static int g_rngb2a[] = { 0, 'A', 26, 'a', 52, '0', 62, '+', 63, '/' };
static int g_rnga2b[] = { '+', 62, '/', 63, '0', 52, 'A', 0, 'a', 26 };
static inline int mapsymb(int symb, int* pmap, int n)
{
    int i, j;
    for (i = j = 0; j < n; j++)
    {
        i += isneg(symb - pmap[j * 2]);
    }
    i = n - 1 - i;
    return symb - pmap[i * 2] + pmap[i * 2 + 1];
}

static int Bin2ascii(const unsigned char* pin, int sz, unsigned char* pout)
{
    int a0, a1, a2, i, j, nout, chr[4];
    for (i = nout = 0; i < sz; i += 3, nout += 4)
    {
        a0 = pin[i];
        j = isneg(i + 1 - sz);
        a1 = pin[i + j] & - j;
        j = isneg(i + 2 - sz);
        a2 = pin[i + j * 2] & - j;
        chr[0] = a0 >> 2;
        chr[1] = a0 << 4 & 0x30 | (a1 >> 4) & 0x0F;
        chr[2] = a1 << 2 & 0x3C | a2 >> 6 & 0x03;
        chr[3] = a2 & 0x03F;
        for (j = 0; j < 4; j++)
        {
            *pout++ = mapsymb(chr[j], g_rngb2a, 5);
        }
    }
    return nout;
}
static int Ascii2bin(const unsigned char* pin, int sz, unsigned char* pout, int szout)
{
    int a0, a1, a2, a3, i, nout;
    for (i = nout = 0; i < sz - 4; i += 4, nout += 3)
    {
        a0 = mapsymb(pin[i + 0], g_rnga2b, 5);
        a1 = mapsymb(pin[i + 1], g_rnga2b, 5);
        a2 = mapsymb(pin[i + 2], g_rnga2b, 5);
        a3 = mapsymb(pin[i + 3], g_rnga2b, 5);
        *pout++ = a0 << 2 | a1 >> 4;
        *pout++ = a1 << 4 & 0xF0 | a2 >> 2 & 0x0F;
        *pout++ = a2 << 6 & 0xC0 | a3;
    }
    a0 = mapsymb(pin[i + 0], g_rnga2b, 5);
    a1 = mapsymb(pin[i + 1], g_rnga2b, 5);
    a2 = mapsymb(pin[i + 2], g_rnga2b, 5);
    a3 = mapsymb(pin[i + 3], g_rnga2b, 5);
    if (nout < szout)
    {
        *pout++ = a0 << 2 | a1 >> 4, nout++;
    }
    if (nout < szout)
    {
        *pout++ = a1 << 4 & 0xF0 | a2 >> 2 & 0x0F, nout++;
    }
    if (nout < szout)
    {
        *pout++ = a2 << 6 & 0xC0 | a3, nout++;
    }
    return nout;
}


static void SerializeData(TSerialize ser, const char* name, void* pdata, int size)
{
    /*static std::vector<int> arr;
    if (ser.IsReading())
    {
        ser.Value(name, arr);
        memcpy(pdata, &arr[0], size);
    } else
    {
        arr.resize(((size-1)>>2)+1);
        memcpy(&arr[0], pdata, size);
        ser.Value(name, arr);
    }*/
    static string str;
    if (!size)
    {
        return;
    }
    if (ser.IsReading())
    {
        ser.Value(name, str);
        int n = Ascii2bin((const unsigned char*)(const char*)str, str.length(), (unsigned char*)pdata, size);
        assert(n == size);
        (void)n;
    }
    else
    {
        str.resize(((size - 1) / 3 + 1) * 4);
        int n = Bin2ascii((const unsigned char*)pdata, size, (unsigned char*)(const char*)str);
        assert(n == str.length());
        (void)n;
        ser.Value(name, str);
    }
}


int CStatObj::Serialize(TSerialize ser)
{
    ser.BeginGroup("StatObj");
    ser.Value("Flags", m_nFlags);
    if (GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        int i, nParts = m_subObjects.size();
        bool bVal;
        string srcObjName;
        ser.Value("nParts", nParts);
        if (m_pClonedSourceObject)
        {
            SetSubObjectCount(nParts);
            for (i = 0; i < nParts; i++)
            {
                ser.BeginGroup("part");
                ser.Value("bGenerated", bVal = !ser.IsReading() && m_subObjects[i].pStatObj &&
                        m_subObjects[i].pStatObj->GetFlags() & STATIC_OBJECT_GENERATED && !m_subObjects[i].bHidden);
                if (bVal)
                {
                    if (ser.IsReading())
                    {
                        (m_subObjects[i].pStatObj = gEnv->p3DEngine->CreateStatObj())->AddRef();
                    }
                    m_subObjects[i].pStatObj->Serialize(ser);
                }
                else
                {
                    ser.Value("subobj", m_subObjects[i].name);
                    if (ser.IsReading())
                    {
                        IStatObj::SSubObject* pSrcSubObj = m_pClonedSourceObject->FindSubObject(m_subObjects[i].name);
                        if (pSrcSubObj)
                        {
                            m_subObjects[i] = *pSrcSubObj;
                            if (pSrcSubObj->pStatObj)
                            {
                                pSrcSubObj->pStatObj->AddRef();
                            }
                        }
                    }
                }
                bVal = m_subObjects[i].bHidden;
                ser.Value("hidden", bVal);
                m_subObjects[i].bHidden = bVal;
                ser.EndGroup();
            }
        }
    }
    else
    {
#if defined(CONSOLE)
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Error: full geometry serialization should never happen on consoles. file: '%s' Geom: '%s'", m_szFileName.c_str(), m_szGeomName.c_str());
#else
        CMesh* pMesh;
        int i, nVtx, nTris, nSubsets;
        string matName;
        _smart_ptr<IMaterial> pMat;

        if (ser.IsReading())
        {
            ser.Value("nvtx", nVtx = 0);
            if (nVtx)
            {
                m_pIndexedMesh = new CIndexedMesh();
                pMesh = m_pIndexedMesh->GetMesh();
                assert(pMesh->m_pPositionsF16 == 0);
                ser.Value("ntris", nTris);
                ser.Value("nsubsets", nSubsets);
                pMesh->SetVertexCount(nVtx);
                pMesh->ReallocStream(CMesh::TEXCOORDS, 0, nVtx);
                pMesh->ReallocStream(CMesh::TANGENTS, 0, nVtx);
                pMesh->SetIndexCount(nTris * 3);

                for (i = 0; i < nSubsets; i++)
                {
                    SMeshSubset mss;
                    ser.BeginGroup("subset");
                    ser.Value("matid", mss.nMatID);
                    ser.Value("matflg", mss.nMatFlags);
                    ser.Value("vtx0", mss.nFirstVertId);
                    ser.Value("nvtx", mss.nNumVerts);
                    ser.Value("idx0", mss.nFirstIndexId);
                    ser.Value("nidx", mss.nNumIndices);
                    ser.Value("center", mss.vCenter);
                    ser.Value("radius", mss.fRadius);
                    pMesh->m_subsets.push_back(mss);
                    ser.EndGroup();
                }

                SerializeData(ser, "Positions", pMesh->m_pPositions, nVtx * sizeof(pMesh->m_pPositions[0]));
                SerializeData(ser, "Normals", pMesh->m_pNorms, nVtx * sizeof(pMesh->m_pNorms[0]));
                SerializeData(ser, "TexCoord", pMesh->m_pTexCoord, nVtx * sizeof(pMesh->m_pTexCoord[0]));
                SerializeData(ser, "Tangents", pMesh->m_pTangents, nVtx * sizeof(pMesh->m_pTangents[0]));
                SerializeData(ser, "Indices", pMesh->m_pIndices, nTris * 3 * sizeof(pMesh->m_pIndices[0]));

                ser.Value("Material", matName);
                SetMaterial(gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName));
                ser.Value("MaterialAux", matName);
                if (m_pMaterial && (pMat = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName)))
                {
                    if (pMat->GetSubMtlCount() > 0)
                    {
                        pMat = pMat->GetSubMtl(0);
                    }
                    for (i = m_pMaterial->GetSubMtlCount() - 1; i >= 0 && strcmp(m_pMaterial->GetSubMtl(i)->GetName(), matName); i--)
                    {
                        ;
                    }
                    if (i < 0)
                    {
                        i = m_pMaterial->GetSubMtlCount();
                        m_pMaterial->SetSubMtlCount(i + 1);
                        m_pMaterial->SetSubMtl(i, pMat);
                    }
                }

                int surfaceTypesId[MAX_SUB_MATERIALS];
                memset(surfaceTypesId, 0, sizeof(surfaceTypesId));
                int numIds = 0;
                if (m_pMaterial)
                {
                    numIds = m_pMaterial->FillSurfaceTypeIds(surfaceTypesId);
                }


                char* pIds = new char[nTris];
                memset(pIds, 0, nTris);
                int j, itri;
                for (i = 0; i < pMesh->m_subsets.size(); i++)
                {
                    for (itri = (j = pMesh->m_subsets[i].nFirstIndexId) / 3; j < pMesh->m_subsets[i].nFirstIndexId + pMesh->m_subsets[i].nNumIndices; j += 3, itri++)
                    {
                        pIds[itri] = pMesh->m_subsets[i].nMatID;
                    }
                }

                ser.Value("PhysSz", i);
                if (i)
                {
                    char* pbuf = new char[i];
                    CMemStream stm(pbuf, i, false);
                    SerializeData(ser, "PhysMeshData", pbuf, i);
                    CRY_PHYSICS_REPLACEMENT_ASSERT();
                    delete[] pbuf;
                }
                delete[] pIds;

                Invalidate();
                SetFlags(STATIC_OBJECT_GENERATED);
            }
        }
        else
        {
            if (GetIndexedMesh(true))
            {
                pMesh = GetIndexedMesh(true)->GetMesh();
                assert(pMesh->m_pPositionsF16 == 0);
                ser.Value("nvtx", nVtx = pMesh->GetVertexCount());
                ser.Value("ntris", nTris = pMesh->GetIndexCount() / 3);
                ser.Value("nsubsets", nSubsets = pMesh->m_subsets.size());

                for (i = 0; i < nSubsets; i++)
                {
                    ser.BeginGroup("subset");
                    ser.Value("matid", pMesh->m_subsets[i].nMatID);
                    ser.Value("matflg", pMesh->m_subsets[i].nMatFlags);
                    ser.Value("vtx0", pMesh->m_subsets[i].nFirstVertId);
                    ser.Value("nvtx", pMesh->m_subsets[i].nNumVerts);
                    ser.Value("idx0", pMesh->m_subsets[i].nFirstIndexId);
                    ser.Value("nidx", pMesh->m_subsets[i].nNumIndices);
                    ser.Value("center", pMesh->m_subsets[i].vCenter);
                    ser.Value("radius", pMesh->m_subsets[i].fRadius);
                    ser.EndGroup();
                }

                if (m_pMaterial && m_pMaterial->GetSubMtlCount() > 0)
                {
                    ser.Value("auxmatname", m_pMaterial->GetSubMtl(m_pMaterial->GetSubMtlCount() - 1)->GetName());
                }

                SerializeData(ser, "Positions", pMesh->m_pPositions, nVtx * sizeof(pMesh->m_pPositions[0]));
                SerializeData(ser, "Normals", pMesh->m_pNorms, nVtx * sizeof(pMesh->m_pNorms[0]));
                SerializeData(ser, "TexCoord", pMesh->m_pTexCoord, nVtx * sizeof(pMesh->m_pTexCoord[0]));
                SerializeData(ser, "Tangents", pMesh->m_pTangents, nVtx * sizeof(pMesh->m_pTangents[0]));
                SerializeData(ser, "Indices", pMesh->m_pIndices, nTris * 3 * sizeof(pMesh->m_pIndices[0]));

                ser.Value("Material", GetMaterial()->GetName());
                if (m_pMaterial && m_pMaterial->GetSubMtlCount() > 0)
                {
                    ser.Value("MaterialAux", m_pMaterial->GetSubMtl(m_pMaterial->GetSubMtlCount() - 1)->GetName());
                }
                else
                {
                    ser.Value("MaterialAux", matName = "");
                }

                if (GetPhysGeom())
                {
                    CMemStream stm(false);
                    ser.Value("PhysSz", i = stm.GetUsedSize());
                    SerializeData(ser, "PhysMeshData", stm.GetBuf(), i);
                }
                else
                {
                    ser.Value("PhysSz", i = 0);
                }
            }
            else
            {
                ser.Value("nvtx", nVtx = 0);
            }
        }
#endif // !defined(CONSOLE)
    }

    ser.EndGroup(); //StatObj

    return 1;
}


