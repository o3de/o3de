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

// Description : Create and draw ocean water geometry (screen space grid, cycle buffers)


#include "Cry3DEngine_precompiled.h"

#include "Ocean.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include "MatMan.h"
#include "VisAreas.h"

#include "Environment/OceanEnvironmentBus.h"

ITimer* COcean::m_pTimer = 0;
CREWaterOcean* COcean::m_pOceanRE = 0;
uint32 COcean::m_nVisiblePixelsCount = ~0;
float COcean::m_fWaterLevelInfo = WATER_LEVEL_UNKNOWN;

// defined in CryEngine\Cry3DEngine\3dEngine.cpp
namespace OceanGlobals
{
    extern float g_oceanStep;
}

COcean::COcean(_smart_ptr<IMaterial> pMat, float fWaterLevel)
{
    m_pRenderMesh = 0;
    m_pBottomCapRenderMesh = 0;
    m_swathWidth = 0;
    m_bUsingFFT = false;
    m_bUseTessHW = false;
    m_fWaterLevel = fWaterLevel;

    memset(m_fRECustomData, 0, sizeof(m_fRECustomData));
    memset(m_fREOceanBottomCustomData, 0, sizeof(m_fREOceanBottomCustomData));
    m_windUvTransform = AZ::Vector2(0.0f, 0.0f);

    SetMaterial(pMat);
    m_fLastFov = 0;
    m_fLastVisibleFrameTime = 0.0f;

    m_pShaderOcclusionQuery = GetRenderer()->EF_LoadShader("OcclusionTest", 0);

    memset(m_pREOcclusionQueries, 0, sizeof(m_pREOcclusionQueries));

    m_nLastVisibleFrameId = 0;

    m_pBottomCapMaterial = GetMatMan()->LoadMaterial("EngineAssets/Materials/Water/WaterOceanBottom", false);
    m_pFogIntoMat = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/OceanInto", false);
    m_pFogOutofMat = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/OceanOutof", false);
    m_pFogIntoMatLowSpec = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/OceanIntoLowSpec", false);
    m_pFogOutofMatLowSpec = GetMatMan()->LoadMaterial("EngineAssets/Materials/Fog/OceanOutofLowSpec", false);

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
    {
        m_pWVRE[i] = static_cast<CREWaterVolume*>(GetRenderer()->EF_CreateRE(eDATA_WaterVolume));
        if (m_pWVRE[i])
        {
            m_pWVRE[i]->m_drawWaterSurface = false;
            m_pWVRE[i]->m_pParams = &m_wvParams[i];
            m_pWVRE[i]->m_pOceanParams = &m_wvoParams[i];
        }
    }

    m_pOceanRE = static_cast<CREWaterOcean*>(GetRenderer()->EF_CreateRE(eDATA_WaterOcean));

    m_nVertsCount = 0;
    m_nIndicesCount = 0;

    m_bOceanFFT = false;
}


COcean::~COcean()
{
    for (int32 x = 0; x < CYCLE_BUFFERS_NUM; ++x)
    {
        if (m_pREOcclusionQueries[x])
        {
            m_pREOcclusionQueries[x]->Release(true);
        }
    }

    m_pRenderMesh = NULL;
    m_pBottomCapRenderMesh = NULL;

    SAFE_RELEASE(m_pOceanRE);
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
    {
        SAFE_RELEASE(m_pWVRE[i]);
    }
}

int32 COcean::GetMemoryUsage()
{
    int32 nSize = 0;

    nSize += sizeofVector(m_pMeshIndices);
    nSize += sizeofVector(m_pMeshVerts);
    nSize += sizeofVector(m_pBottomCapVerts);
    nSize += sizeofVector(m_pBottomCapIndices);

    return nSize;
}

void COcean::Update(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    C3DEngine* p3DEngine = (C3DEngine*)Get3DEngine();
    IRenderer* pRenderer = GetRenderer();
    if (passInfo.IsRecursivePass() ||  !passInfo.RenderWaterOcean() || !m_pMaterial)
    {
        return;
    }

    const CCamera& rCamera = passInfo.GetCamera();
    int32 nFillThreadID = passInfo.ThreadID();
    uint32 nBufID = passInfo.GetFrameID() % CYCLE_BUFFERS_NUM;

    Vec3 vCamPos = rCamera.GetPosition();
    float fWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : p3DEngine->GetWaterLevel();

    // No hardware FFT support
    m_bOceanFFT = false;
    if ((pRenderer->GetFeatures() & (RFT_HW_VERTEXTEXTURES) && GetCVars()->e_WaterOceanFFT) && pRenderer->EF_GetShaderQuality(eST_Water) >= eSQ_High)
    {
        m_bOceanFFT = true;
    }

    if (vCamPos.z < fWaterLevel)
    {
        // if camera is in indoors and lower than ocean level
        // and exit portals are higher than ocean level - skip ocean rendering
        CVisArea* pVisArea = (CVisArea*)p3DEngine->GetVisAreaFromPos(vCamPos);
        if (pVisArea && !pVisArea->IsPortal())
        {
            for (int32 i = 0; i < pVisArea->m_lstConnections.Count(); i++)
            {
                if (pVisArea->m_lstConnections[i]->IsConnectedToOutdoor() && pVisArea->m_lstConnections[i]->m_boxArea.min.z < fWaterLevel)
                {
                    break; // there is portal making ocean visible
                }
                if (i == pVisArea->m_lstConnections.Count())
                {
                    return; // ocean surface is not visible
                }
            }
        }
    }

    bool bWaterVisible = IsVisible(passInfo);
    float _fWaterPlaneSize = rCamera.GetFarPlane();

    // Check if water surface occluded
    if (fabs(m_fLastFov - rCamera.GetFov()) < 0.01f && GetCVars()->e_HwOcclusionCullingWater && passInfo.IsGeneralPass())
    {
        AABB boxOcean(Vec3(vCamPos.x - _fWaterPlaneSize, vCamPos.y - _fWaterPlaneSize, fWaterLevel),
            Vec3(vCamPos.x + _fWaterPlaneSize, vCamPos.y + _fWaterPlaneSize, fWaterLevel));

        if ((!GetVisAreaManager()->IsOceanVisible() && rCamera.IsAABBVisible_EM(boxOcean)) ||
            (GetVisAreaManager()->IsOceanVisible() && rCamera.IsAABBVisible_E(boxOcean)))
        {
            // make element if not ready
            if (!m_pREOcclusionQueries[nBufID])
            {
                m_pREOcclusionQueries[nBufID] = (CREOcclusionQuery*)GetRenderer()->EF_CreateRE(eDATA_OcclusionQuery);
                m_pREOcclusionQueries[nBufID]->m_pRMBox = (CRenderMesh*)GetObjManager()->GetRenderMeshBox();
            }

            // get last test result
            //  if((m_pREOcclusionQueries[nFillThreadID][nBufID]->m_nCheckFrame - passInfo.GetFrameID())<2)
            {
                COcean::m_nVisiblePixelsCount = m_pREOcclusionQueries[nBufID]->m_nVisSamples;
                if (COcean::IsWaterVisibleOcclusionCheck())
                {
                    m_nLastVisibleFrameId = passInfo.GetFrameID();
                    bWaterVisible = true;
                }
            }

            // request new test
            m_pREOcclusionQueries[nBufID]->m_vBoxMin(boxOcean.min.x, boxOcean.min.y, boxOcean.min.z - 1.0f);
            m_pREOcclusionQueries[nBufID]->m_vBoxMax(boxOcean.max.x, boxOcean.max.y, boxOcean.max.z);

            m_pREOcclusionQueries[nBufID]->mfReadResult_Try(COcean::m_nVisiblePixelsCount);
            if (!m_pREOcclusionQueries[nBufID]->m_nDrawFrame || m_pREOcclusionQueries[nBufID]->HasSucceeded())
            {
                SShaderItem shItem(m_pShaderOcclusionQuery);
                CRenderObject* pObj = GetIdentityCRenderObject(passInfo.ThreadID());
                if (!pObj)
                {
                    return;
                }
                GetRenderer()->EF_AddEf(m_pREOcclusionQueries[nBufID], shItem, pObj, passInfo, EFSLIST_WATER_VOLUMES, 0, SRendItemSorter::CreateDefaultRendItemSorter());
            }
        }
    }
    else
    {
        m_nLastVisibleFrameId = passInfo.GetFrameID();
        bWaterVisible = true;
    }

    if (bWaterVisible || vCamPos.z  < fWaterLevel)
    {
        m_p3DEngine->SetOceanRenderFlags(OCR_OCEANVOLUME_VISIBLE);

        // lazy mesh creation
        if (bWaterVisible)
        {
            Create();
        }
    }
}

void COcean::Create()
{
    // Calculate water geometry and update vertex buffers
    int32 nScrGridSizeX;
    int32 nScrGridSizeY;

    GetOceanGridSize(nScrGridSizeX, nScrGridSizeY);

    bool bUseWaterTessHW;
    GetRenderer()->EF_Query(EFQ_WaterTessellation, bUseWaterTessHW);

    // Generate screen space grid
    if ((m_bOceanFFT && m_bUsingFFT != m_bOceanFFT) || m_bUseTessHW != bUseWaterTessHW || m_swathWidth != GetCVars()->e_WaterTessellationSwathWidth || !m_nVertsCount || !m_nIndicesCount ||  nScrGridSizeX * nScrGridSizeY != m_nPrevGridDim)
    {
        m_nPrevGridDim = nScrGridSizeX * nScrGridSizeY;
        m_pMeshVerts.Clear();
        m_pMeshIndices.Clear();
        m_nVertsCount = 0;
        m_nIndicesCount = 0;

        m_bUsingFFT = m_bOceanFFT;
        m_bUseTessHW = bUseWaterTessHW;
        // Update the swath width
        m_swathWidth = GetCVars()->e_WaterTessellationSwathWidth;

        // Render ocean with screen space tessellation

        int32 nScreenY = GetRenderer()->GetHeight();
        int32 nScreenX = GetRenderer()->GetWidth();

        if (!nScreenY || !nScreenX)
        {
            return;
        }

        float fRcpScrGridSizeX = 1.0f / ((float) nScrGridSizeX - 1);
        float fRcpScrGridSizeY = 1.0f / ((float) nScrGridSizeY - 1);

        SVF_P3F_C4B_T2F tmp;
        Vec3 vv;
        vv.z = 0;

        m_pMeshVerts.reserve(nScrGridSizeX * nScrGridSizeY);
        m_pMeshIndices.reserve(nScrGridSizeX * nScrGridSizeY);

        // Grid vertex generation
        for (int32 y(0); y < nScrGridSizeY; ++y)
        {
            vv.y = (float) y * fRcpScrGridSizeY;// + fRcpScrGridSize;

            for (int32 x(0); x < nScrGridSizeX; ++x)
            {
                // vert 1
                vv.x = (float) x * fRcpScrGridSizeX;// + fRcpScrGridSize;

                // store in z edges information
                float fx = fabs((vv.x) * 2.0f - 1.0f);
                float fy = fabs((vv.y) * 2.0f - 1.0f);
                //float fEdgeDisplace = sqrt_tpl(fx*fx + fy * fy);//max(fx, fy);
                float fEdgeDisplace = max(fx, fy);
                //sqrt_tpl(fx*fx + fy * fy);
                vv.z = fEdgeDisplace; //!((y==0 ||y == nScrGridSize-1) || (x==0 || x == nScrGridSize-1));

                int32 n = m_pMeshVerts.Count();
                tmp.xyz = vv;
                m_pMeshVerts.Add(tmp);
            }
        }

        if (m_bUseTessHW)
        {
            // Normal approach
            int32 nIndex = 0;
            for (int32 y(0); y < nScrGridSizeY - 1; ++y)
            {
                for (int32 x(0); x < nScrGridSizeX - 1; ++x, ++nIndex)
                {
                    m_pMeshIndices.Add(nScrGridSizeX * y + x);
                    m_pMeshIndices.Add(nScrGridSizeX * y + x + 1);
                    m_pMeshIndices.Add(nScrGridSizeX * (y + 1) + x);

                    m_pMeshIndices.Add(nScrGridSizeX * (y + 1) + x);
                    m_pMeshIndices.Add(nScrGridSizeX * y + x + 1);
                    m_pMeshIndices.Add(nScrGridSizeX * (y + 1) + x + 1);

                    //m_pMeshIndices.Add( nIndex );
                    //m_pMeshIndices.Add( nIndex + 1);
                    //m_pMeshIndices.Add( nIndex + nScrGridSizeX);

                    //m_pMeshIndices.Add( nIndex + nScrGridSizeX);
                    //m_pMeshIndices.Add( nIndex + 1);
                    //m_pMeshIndices.Add( nIndex + nScrGridSizeX + 1);
                }
            }
        }
        else
        {
            // Grid index generation

            if (m_swathWidth < 0)
            {
                // Normal approach
                int32 nIndex = 0;
                for (int32 y(0); y < nScrGridSizeY - 1; ++y)
                {
                    for (int32 x(0); x < nScrGridSizeX; ++x, ++nIndex)
                    {
                        m_pMeshIndices.Add(nIndex);
                        m_pMeshIndices.Add(nIndex + nScrGridSizeX);
                    }

                    if (nScrGridSizeY - 2 > y)
                    {
                        m_pMeshIndices.Add(nIndex + nScrGridSizeY - 1);
                        m_pMeshIndices.Add(nIndex);
                    }
                }
            }
            else if(m_swathWidth > 1)
            {
                // Boustrophedonic walk
                //
                //  0  1  2  3  4
                //  5  6  7  8  9
                // 10 11 12 13 14
                // 15 16 17 18 19
                //
                // Should generate the following indices
                // 0 5 1 6 2 7 3 8 4 9 9 14 14 9 13 8 12 7 11 6 10 5 5 10 10 15 11 16 12 17 13 18 14 19
                //

                int32 startX = 0, endX = m_swathWidth - 1;

                do
                {
                    for (int32 y(0); y < nScrGridSizeY - 1; y += 2)
                    {
                        // Forward
                        for (int32 x(startX); x <= endX; ++x)
                        {
                            m_pMeshIndices.Add(y * nScrGridSizeX + x);
                            m_pMeshIndices.Add((y + 1) * nScrGridSizeX + x);
                        }

                        // Can we go backwards?
                        if (y + 2 < nScrGridSizeY)
                        {
                            // Restart strip by duplicating last and first of next strip
                            m_pMeshIndices.Add((y + 1) * nScrGridSizeX + endX);
                            m_pMeshIndices.Add((y + 2) * nScrGridSizeX + endX);

                            //Backward
                            for (int32 x(endX); x >= startX; --x)
                            {
                                m_pMeshIndices.Add((y + 2) * nScrGridSizeX + x);
                                m_pMeshIndices.Add((y + 1) * nScrGridSizeX + x);
                            }

                            // Restart strip
                            if (y + 2 == nScrGridSizeY - 1 && endX < nScrGridSizeX - 1)
                            {
                                if (endX < nScrGridSizeX - 1)
                                {
                                    // Need to restart at the top of the next column
                                    m_pMeshIndices.Add((nScrGridSizeY - 1) * nScrGridSizeX + startX);
                                    m_pMeshIndices.Add(endX);
                                }
                            }
                            else
                            {
                                m_pMeshIndices.Add((y + 1) * nScrGridSizeX + startX);
                                m_pMeshIndices.Add((y + 2) * nScrGridSizeX + startX);
                            }
                        }
                        else
                        {
                            // We can restart to next column
                            if (endX < nScrGridSizeX - 1)
                            {
                                // Restart strip for next swath
                                m_pMeshIndices.Add((nScrGridSizeY - 1) * nScrGridSizeX + endX);
                                m_pMeshIndices.Add(endX);
                            }
                        }
                    }

                    startX = endX;
                    endX   = startX + m_swathWidth - 1;

                    if (endX >= nScrGridSizeX)
                    {
                        endX = nScrGridSizeX - 1;
                    }
                } while (startX < nScrGridSizeX - 1);
            }
            else
            {
                AZ_TracePrintf("Ocean", "e_WaterTessellationSwathWidth cannot be 0.")
            }
        }

        m_nVertsCount = m_pMeshVerts.Count();
        m_nIndicesCount = m_pMeshIndices.Count();

        m_pRenderMesh = GetRenderer()->CreateRenderMeshInitialized(
                m_pMeshVerts.GetElements(),
                m_pMeshVerts.Count(),
                eVF_P3F_C4B_T2F,
                m_pMeshIndices.GetElements(),
                m_pMeshIndices.Count(),
                m_bUseTessHW ? prtTriangleList : prtTriangleStrip,
                "OutdoorWaterGrid", "OutdoorWaterGrid",
                eRMT_Static);

        m_pRenderMesh->SetChunk(m_pMaterial, 0, m_pMeshVerts.Count(), 0, m_pMeshIndices.Count(), 1.0f, eVF_P3F_C4B_T2F);

        if (m_bOceanFFT)
        {
            m_pOceanRE->Create(m_pMeshVerts.Count(), m_pMeshVerts.GetElements(), m_pMeshIndices.Count(), m_pMeshIndices.GetElements(), sizeof(m_pMeshIndices[0]));
        }

        m_pMeshVerts.Free();
        m_pMeshIndices.Free();
    }
}

void COcean::Render(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    // if reaches render stage - means ocean is visible

    C3DEngine* p3DEngine = (C3DEngine*)Get3DEngine();
    IRenderer* pRenderer(GetRenderer());

    int32 nBufID = (passInfo.GetFrameID() & 1);
    Vec3 vCamPos = passInfo.GetCamera().GetPosition();
    float fWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : p3DEngine->GetWaterLevel();

    CRenderObject* pObject = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
    if (!pObject)
    {
        return;
    }
    pObject->m_II.m_Matrix.SetIdentity();
    pObject->m_pRenderNode = this;

    m_fLastFov = passInfo.GetCamera().GetFov();

    // make distance to water level near to zero
    m_pRenderMesh->SetBBox(vCamPos, vCamPos);

    // test for multiple lights and shadows support

    SRenderObjData* pOD = pRenderer->EF_GetObjData(pObject, true, passInfo.ThreadID());

    m_Camera = passInfo.GetCamera();
    pObject->m_fAlpha = 1.f;

    int32 gridSizeX;
    int32 gridSizeY;
    GetOceanGridSize(gridSizeX, gridSizeY);
    const float tessellationScaleFactor = 1000.0f; // physical size of grid is how many grid tiles per thousand meters.
    m_fRECustomData[0] = tessellationScaleFactor / (gridSizeX - 1);

    const auto oceanAnimationData = p3DEngine->GetOceanAnimationParams();

    m_fRECustomData[1] = oceanAnimationData.fWindSpeed;
    m_fRECustomData[2] = oceanAnimationData.fWavesSpeed;
    m_fRECustomData[3] = oceanAnimationData.fWavesAmount;
    m_fRECustomData[4] = oceanAnimationData.fWavesSize;

    const float timeDiff = p3DEngine->GetTimer()->GetFrameTime();

    // calculate the wind direction
    AZ::Vector2 windDirectionVector = AZ::Vector2::CreateFromAngle(oceanAnimationData.fWindDirection);

    // calculate wind offset based on speed and time delta
    float windFrameOffset = 0.0025f * timeDiff * oceanAnimationData.fWindSpeed;
    m_windUvTransform += windDirectionVector * windFrameOffset;

    // update constant buffer with the values;
    m_fRECustomData[6] = m_windUvTransform.GetX();
    m_fRECustomData[5] = m_windUvTransform.GetY();

    m_fRECustomData[7] = fWaterLevel;

    m_fRECustomData[8] = m_fRECustomData[9] = m_fRECustomData[10] = m_fRECustomData[11] = 0.0f;

    bool isFastpath = GetCVars()->e_WaterOcean == 2;
    bool bUsingMergedFog = false;

    {
        Vec3 camPos(passInfo.GetCamera().GetPosition());

        // Check if we outside water volume - we can enable fast path with merged fog version
        if (camPos.z - fWaterLevel >= oceanAnimationData.fWavesSize)
        {
            Vec3 fogColor;
            if (OceanToggle::IsActive())
            {
                AZ::Vector3 oceanFogColor = OceanRequest::GetFogColorPremultiplied();
                fogColor = Vec3(oceanFogColor.GetX(), oceanFogColor.GetY(), oceanFogColor.GetZ());
            }
            else
            {
                fogColor = m_p3DEngine->m_oceanFogColor;
            }
            Vec3 cFinalFogColor = gEnv->p3DEngine->GetSunColor().CompMul(fogColor);
            float fogDensity = OceanToggle::IsActive() ? OceanRequest::GetFogDensity() : m_p3DEngine->m_oceanFogDensity;
            Vec4 vFogParams = Vec4(cFinalFogColor, fogDensity * 1.44269502f);// log2(e) = 1.44269502

            m_fRECustomData[8] = vFogParams.x;
            m_fRECustomData[9] = vFogParams.y;
            m_fRECustomData[10] = vFogParams.z;
            m_fRECustomData[11] = vFogParams.w;
            if (isFastpath)
            {
                bUsingMergedFog = true;
            }
        }
    }

    {
        CMatInfo* pMatInfo = (CMatInfo*)m_pMaterial.get();
        float fInstanceDistance = AZ::OceanConstants::s_oceanIsVeryFarAway;
        pMatInfo->PrecacheMaterial(fInstanceDistance, 0, false);
    }

    if (!GetCVars()->e_WaterOceanFFT || !m_bOceanFFT)
    {
        m_pRenderMesh->SetREUserData(&m_fRECustomData[0]);
        m_pRenderMesh->AddRenderElements(m_pMaterial, pObject, passInfo, EFSLIST_WATER, 0);
    }
    else
    {
        SShaderItem& shaderItem(m_pMaterial->GetShaderItem(0));
        m_pOceanRE->m_CustomData = &m_fRECustomData[0];
        pRenderer->EF_AddEf(m_pOceanRE, shaderItem, pObject, passInfo, EFSLIST_WATER, 0, SRendItemSorter::CreateDefaultRendItemSorter());
    }

    bool useOceanBottom = OceanToggle::IsActive() ? OceanRequest::GetUseOceanBottom() : (GetCVars()->e_WaterOceanBottom == 1);
    if (useOceanBottom)
    {
        RenderBottomCap(passInfo);
    }

    if (!bUsingMergedFog)
    {
        RenderFog(passInfo);
    }
}

void COcean::SetMaterial(_smart_ptr<IMaterial> pMat)
{
    m_pMaterial = pMat;
}

void COcean::RenderBottomCap(const SRenderingPassInfo& passInfo)
{
    C3DEngine* p3DEngine = (C3DEngine*)Get3DEngine();

    Vec3 vCamPos = passInfo.GetCamera().GetPosition();

    // Render ocean with screen space tessellation

    int32 nScreenY = GetRenderer()->GetHeight();
    int32 nScreenX = GetRenderer()->GetWidth();

    if (!nScreenY || !nScreenX)
    {
        return;
    }

    // Calculate water geometry and update vertex buffers
    static const int32 nScrGridSize = 5;
    // distance between grid points for -1.0 to 1.0 space.
    static const float fRcpScrGridSize = 2.0f / static_cast<float>(nScrGridSize - 1);

    if (!m_pBottomCapVerts.Count() || !m_pBottomCapIndices.Count() ||  nScrGridSize * nScrGridSize != m_pBottomCapVerts.Count())
    {
        m_pBottomCapVerts.Clear();
        m_pBottomCapIndices.Clear();

        SVF_P3F_C4B_T2F tmp;
        tmp.xyz.z = 1.0f;

        // Grid vertex generation
        for (int32 y = 0; y < nScrGridSize; ++y)
        {
            tmp.xyz.y = -1.0f + y * fRcpScrGridSize;
            for (int32 x(0); x < nScrGridSize; ++x)
            {
                tmp.xyz.x = -1.0f + x * fRcpScrGridSize;
                m_pBottomCapVerts.Add(tmp);
            }
        }

        // Normal approach
        int32 nIndex = 0;
        for (int32 y(0); y < nScrGridSize - 1; ++y)
        {
            for (int32 x(0); x < nScrGridSize; ++x, ++nIndex)
            {
                m_pBottomCapIndices.Add(nIndex);
                m_pBottomCapIndices.Add(nIndex + nScrGridSize);
            }

            if (nScrGridSize - 2 > y)
            {
                m_pBottomCapIndices.Add(nIndex + nScrGridSize - 1);
                m_pBottomCapIndices.Add(nIndex);
            }
        }

        m_pBottomCapRenderMesh = GetRenderer()->CreateRenderMeshInitialized(
                m_pBottomCapVerts.GetElements(),
                m_pBottomCapVerts.Count(),
                eVF_P3F_C4B_T2F,
                m_pBottomCapIndices.GetElements(),
                m_pBottomCapIndices.Count(),
                prtTriangleStrip,
                "OceanBottomGrid", "OceanBottomGrid",
                eRMT_Static);

        m_pBottomCapRenderMesh->SetChunk(m_pBottomCapMaterial, 0, m_pBottomCapVerts.Count(), 0, m_pBottomCapIndices.Count(), 1.0f, eVF_P3F_C4B_T2F);
    }

    CRenderObject* pObject = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
    if (!pObject)
    {
        return;
    }
    pObject->m_II.m_Matrix.SetIdentity();
    pObject->m_pRenderNode = this;

    // make distance to water level near to zero
    m_pBottomCapRenderMesh->SetBBox(vCamPos, vCamPos);

    m_Camera = passInfo.GetCamera();
    pObject->m_fAlpha = 1.f;

    m_pBottomCapRenderMesh->AddRenderElements(m_pBottomCapMaterial, pObject, passInfo, EFSLIST_GENERAL, 0);
}

void COcean::RenderFog(const SRenderingPassInfo& passInfo)
{
    if (!GetCVars()->e_Fog || !GetCVars()->e_FogVolumes)
    {
        return;
    }

    IRenderer* pRenderer(GetRenderer());
    C3DEngine* p3DEngine(Get3DEngine());

    const int fillThreadID = passInfo.ThreadID();

    CRenderObject* pROVol(pRenderer->EF_GetObject_Temp(fillThreadID));
    if (!pROVol)
    {
        return;
    }

    bool isFastpath = GetCVars()->e_WaterOcean == 2;
    bool isLowSpec(GetCVars()->e_ObjQuality == CONFIG_LOW_SPEC || isFastpath);
    if (pROVol && m_pWVRE[fillThreadID] && ((!isLowSpec && m_pFogIntoMat && m_pFogOutofMat) ||
                                            (isLowSpec && m_pFogIntoMatLowSpec && m_pFogOutofMatLowSpec)))
    {
        Vec3 camPos(passInfo.GetCamera().GetPosition());
        float waterLevel(OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : p3DEngine->GetWaterLevel());
        Vec3 planeOrigin(camPos.x, camPos.y, waterLevel);

        // fill water volume param structure
        m_wvParams[fillThreadID].m_center = planeOrigin;
        m_wvParams[fillThreadID].m_fogPlane.Set(Vec3(0, 0, 1), -waterLevel);

        float distCamToFogPlane(camPos.z + m_wvParams[fillThreadID].m_fogPlane.d);
        m_wvParams[fillThreadID].m_viewerCloseToWaterPlane = (distCamToFogPlane) < 0.5f;
        m_wvParams[fillThreadID].m_viewerInsideVolume = distCamToFogPlane < 0.00f;
        m_wvParams[fillThreadID].m_viewerCloseToWaterVolume = true;

        const auto oceanAnimationData = p3DEngine->GetOceanAnimationParams();

        if (!isFastpath || (distCamToFogPlane < oceanAnimationData.fWavesSize))
        {
            Vec3 fogColor;
            Vec3 fogColorShallow;
            bool oceanToggleIsActive = OceanToggle::IsActive();
            if (oceanToggleIsActive)
            {
                AZ::Vector3 oceanFogColor = OceanRequest::GetFogColorPremultiplied();
                fogColor = Vec3(oceanFogColor.GetX(), oceanFogColor.GetY(), oceanFogColor.GetZ());

                AZ::Vector3 nearFogColor = OceanRequest::GetNearFogColor();
                fogColorShallow = Vec3(nearFogColor.GetX(), nearFogColor.GetY(), nearFogColor.GetZ());
            }
            else
            {
                fogColor = m_p3DEngine->m_oceanFogColor;
                fogColorShallow = m_p3DEngine->m_oceanFogColorShallow;
            }
            float fogDensity = oceanToggleIsActive ? OceanRequest::GetFogDensity() : m_p3DEngine->m_oceanFogDensity;

            if (isLowSpec)
            {
                m_wvParams[fillThreadID].m_fogColor = fogColor;
                m_wvParams[fillThreadID].m_fogDensity = fogDensity;

                m_wvoParams[fillThreadID].m_fogColor = Vec3(0, 0, 0);   // not needed for low spec
                m_wvoParams[fillThreadID].m_fogColorShallow = Vec3(0, 0, 0);   // not needed for low spec
                m_wvoParams[fillThreadID].m_fogDensity = 0; // not needed for low spec

                m_pWVRE[fillThreadID]->m_pOceanParams = 0;
            }
            else
            {
                m_wvParams[fillThreadID].m_fogColor = Vec3(0, 0, 0);   // not needed, we set ocean specific params below
                m_wvParams[fillThreadID].m_fogDensity = 0; // not needed, we set ocean specific params below

                m_wvoParams[fillThreadID].m_fogColor = fogColor;
                m_wvoParams[fillThreadID].m_fogColorShallow = fogColorShallow;
                m_wvoParams[fillThreadID].m_fogDensity = fogDensity;

                m_pWVRE[fillThreadID]->m_pOceanParams = &m_wvoParams[fillThreadID];
            }

            // tessellate plane
            float planeSize(2.0f * passInfo.GetCamera().GetFarPlane());
            size_t subDivSize(min(64, 1 + (int32) (planeSize / 512.0f)));
            if (isFastpath)
            {
                subDivSize = 4;
            }

            size_t numSubDivVerts((subDivSize + 1) * (subDivSize + 1));

            if (m_wvVertices[fillThreadID].size() != numSubDivVerts)
            {
                m_wvVertices[fillThreadID].resize(numSubDivVerts);
                m_wvParams[fillThreadID].m_pVertices = &m_wvVertices[fillThreadID][0];
                m_wvParams[fillThreadID].m_numVertices = m_wvVertices[fillThreadID].size();

                m_wvIndices[fillThreadID].resize(subDivSize * subDivSize * 6);
                m_wvParams[fillThreadID].m_pIndices = &m_wvIndices[fillThreadID][0];
                m_wvParams[fillThreadID].m_numIndices = m_wvIndices[fillThreadID].size();

                size_t ind(0);
                for (uint32 y(0); y < subDivSize; ++y)
                {
                    for (uint32 x(0); x < subDivSize; ++x, ind += 6)
                    {
                        m_wvIndices[fillThreadID][ind + 0] = (y) * (subDivSize + 1) + (x);
                        m_wvIndices[fillThreadID][ind + 1] = (y) * (subDivSize + 1) + (x + 1);
                        m_wvIndices[fillThreadID][ind + 2] = (y + 1) * (subDivSize + 1) + (x + 1);

                        m_wvIndices[fillThreadID][ind + 3] = (y) * (subDivSize + 1) + (x);
                        m_wvIndices[fillThreadID][ind + 4] = (y + 1) * (subDivSize + 1) + (x + 1);
                        m_wvIndices[fillThreadID][ind + 5] = (y + 1) * (subDivSize + 1) + (x);
                    }
                }
            }
            {
                float xyDelta(2.0f * planeSize / (float) subDivSize);
                float zDelta(waterLevel - camPos.z);

                size_t ind(0);
                float yd(-planeSize);
                for (uint32 y(0); y <= subDivSize; ++y, yd += xyDelta)
                {
                    float xd(-planeSize);
                    for (uint32 x(0); x <= subDivSize; ++x, xd += xyDelta, ++ind)
                    {
                        m_wvVertices[fillThreadID][ind].xyz = Vec3(xd, yd, zDelta);
                        m_wvVertices[fillThreadID][ind].st = Vec2(0, 0);
                    }
                }
            }

            // fill in data for render object
            pROVol->m_II.m_Matrix.SetIdentity();
            pROVol->m_fSort = 0;

            // get shader item
            SShaderItem& shaderItem(m_wvParams[fillThreadID].m_viewerInsideVolume ?
                (isLowSpec ? m_pFogOutofMatLowSpec->GetShaderItem(0) : m_pFogOutofMat->GetShaderItem(0)) :
                (isLowSpec ? m_pFogIntoMatLowSpec->GetShaderItem(0) : m_pFogIntoMat->GetShaderItem(0)));

            // add to renderer
            pRenderer->EF_AddEf(m_pWVRE[fillThreadID], shaderItem, pROVol, passInfo, EFSLIST_WATER_VOLUMES, distCamToFogPlane < -0.1f, SRendItemSorter::CreateDefaultRendItemSorter());
        }
    }
}

bool COcean::IsVisible(const SRenderingPassInfo& passInfo)
{
    if (abs(m_nLastVisibleFrameId - passInfo.GetFrameID()) <= 2)
    {
        m_fLastVisibleFrameTime = 0.0f;
    }

    ITimer* pTimer(gEnv->pTimer);
    m_fLastVisibleFrameTime += gEnv->pTimer->GetFrameTime();

    if (m_fLastVisibleFrameTime > 2.0f) // at least 2 seconds
    {
        return (abs(m_nLastVisibleFrameId - passInfo.GetFrameID()) < 64); // and at least 64 frames
    }
    return true; // keep water visible for a couple frames - or at least 1 second - minimizes popping during fast camera movement
}

void COcean::SetTimer(ITimer* pTimer)
{
    assert(pTimer);
    m_pTimer = pTimer;
}

float COcean::GetWave(const Vec3& pPos, int32 nFrameID)
{
    // todo: optimize...

    IRenderer* pRenderer(GetRenderer());
    if (!pRenderer)
    {
        return 0.0f;
    }

    EShaderQuality nShaderQuality = pRenderer->EF_GetShaderQuality(eST_Water);

    if (!m_pTimer || nShaderQuality < eSQ_High)
    {
        return 0.0f;
    }

    // Return height - matching computation on GPU

    C3DEngine* p3DEngine(Get3DEngine());

    bool bOceanFFT = false;
    if ((pRenderer->GetFeatures() & (RFT_HW_VERTEXTEXTURES) && GetCVars()->e_WaterOceanFFT) && nShaderQuality >= eSQ_High)
    {
        bOceanFFT = true;
    }

    const auto oceanAnimationData = p3DEngine->GetOceanAnimationParams();

    if (bOceanFFT)
    {
        Vec4 pDispPos = Vec4(0, 0, 0, 0);

        if (m_pOceanRE)
        {
            // Get height from FFT grid

            Vec4* pGridFFT = m_pOceanRE->GetDisplaceGrid();
            if (!pGridFFT)
            {
                return 0.0f;
            }

            // match scales used in shader
            float fScaleX = pPos.x * 0.0125f * oceanAnimationData.fWavesAmount * 1.25f;
            float fScaleY = pPos.y * 0.0125f * oceanAnimationData.fWavesAmount * 1.25f;

            float fu = fScaleX * 64.0f;
            float fv = fScaleY * 64.0f;
            int32 u1 = ((int32)fu) & 63;
            int32 v1 = ((int32)fv) & 63;
            int32 u2 = (u1 + 1) & 63;
            int32 v2 = (v1 + 1) & 63;

            // Fractional parts
            float fracu = fu - floorf(fu);
            float fracv = fv - floorf(fv);

            // Get weights
            float w1 = (1 - fracu) * (1 - fracv);
            float w2 = fracu * (1 - fracv);
            float w3 = (1 - fracu) * fracv;
            float w4 = fracu *  fracv;

            Vec4 h1 = pGridFFT[u1 + v1 * 64];
            Vec4 h2 = pGridFFT[u2 + v1 * 64];
            Vec4 h3 = pGridFFT[u1 + v2 * 64];
            Vec4 h4 = pGridFFT[u2 + v2 * 64];

            // scale and sum the four heights
            pDispPos  = h1 * w1 + h2 * w2 + h3 * w3 + h4 * w4;
        }

        // match scales used in shader
        return pDispPos.z * 0.06f * oceanAnimationData.fWavesSize;
    }

    // constant to scale down values a bit
    const float fAnimAmplitudeScale = 1.0f / 5.0f;

    static int32 s_nFrameID = 0;
    static Vec3 s_vFlowDir = Vec3(0, 0, 0);
    static Vec4 s_vFrequencies = Vec4(0, 0, 0, 0);
    static Vec4 s_vPhases = Vec4(0, 0, 0, 0);
    static Vec4 s_vAmplitudes = Vec4(0, 0, 0, 0);

    // Update per-frame data
    if (s_nFrameID != nFrameID)
    {
        sincos_tpl(oceanAnimationData.fWindDirection, &s_vFlowDir.y, &s_vFlowDir.x);
        s_vFrequencies = Vec4(0.233f, 0.455f, 0.6135f, -0.1467f) * oceanAnimationData.fWavesSpeed * 5.0f;
        s_vPhases = Vec4(0.1f, 0.159f, 0.557f, 0.2199f) * oceanAnimationData.fWavesAmount;
        s_vAmplitudes = Vec4(1.0f, 0.5f, 0.25f, 0.5f) * oceanAnimationData.fWavesSize;

        s_nFrameID = nFrameID;
    }

    const float fPhase = sqrt_tpl(pPos.x * pPos.x + pPos.y * pPos.y);
    Vec4 vCosPhase = s_vPhases * (fPhase + pPos.x);

    Vec4 vWaveFreq = s_vFrequencies * m_pTimer->GetCurrTime();

    Vec4 vCosWave = Vec4(cos_tpl(vWaveFreq.x * s_vFlowDir.x + vCosPhase.x),
            cos_tpl(vWaveFreq.y * s_vFlowDir.x + vCosPhase.y),
            cos_tpl(vWaveFreq.z * s_vFlowDir.x + vCosPhase.z),
            cos_tpl(vWaveFreq.w * s_vFlowDir.x + vCosPhase.w));


    Vec4 vSinPhase = s_vPhases * (fPhase + pPos.y);
    Vec4 vSinWave = Vec4(sin_tpl(vWaveFreq.x * s_vFlowDir.y + vSinPhase.x),
            sin_tpl(vWaveFreq.y * s_vFlowDir.y + vSinPhase.y),
            sin_tpl(vWaveFreq.z * s_vFlowDir.y + vSinPhase.z),
            sin_tpl(vWaveFreq.w * s_vFlowDir.y + vSinPhase.w));

    return (vCosWave.Dot(s_vAmplitudes) + vSinWave.Dot(s_vAmplitudes)) * fAnimAmplitudeScale;
}

uint32 COcean::GetVisiblePixelsCount()
{
    return m_nVisiblePixelsCount;
}

bool COcean::IsWaterVisibleOcclusionCheck()
{
    //Metal only supports yes(1 or more pixels)/no occlusion querys. m_nVisiblePixelsCount will always be 1 for yes, 0 for no.
#if defined(IOS) || defined(ANDROID)
    return m_nVisiblePixelsCount >= 1;
#else
    return m_nVisiblePixelsCount > 16;
#endif
}

// Gets the X & Y Grid sizes based on cVars. Individual X & Y CVars should probably go away in
// favor of a single value that exists on an ocean component instead of using CVars at all.
void COcean::GetOceanGridSize(int& outX, int& outY) const
{
    // Calculate water geometry and update vertex buffers
    if (OceanToggle::IsActive())
    {
        outX = AZ::OceanConstants::s_waterTessellationDefault;
        AZ::OceanEnvironmentBus::BroadcastResult(outX, &AZ::OceanEnvironmentBus::Events::GetWaterTessellationAmount);
        outY = outX;
    }
    else
    {
         // Calculate water geometry and update vertex buffers
        outX = outY = GetCVars()->e_WaterTessellationAmount;

        bool bUseWaterTessHW;
        GetRenderer()->EF_Query(EFQ_WaterTessellation, bUseWaterTessHW);

        if (!bUseWaterTessHW && m_bOceanFFT)
        {
            outX = outY = 20 * 10; // for hi/very specs - use maximum tessellation
        }
    }
}

void COcean::SetWaterLevel(float fWaterLevel)
{
    m_fWaterLevel = fWaterLevel;
    OceanGlobals::g_oceanStep = -1; // if e_PhysOceanCell is used, make it re-apply the params on Update
}
