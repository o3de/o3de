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

// Description : draw far objects as sprites


#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "3dEngine.h"

static PodArray<SVegetationSpriteInfo> arrVegetationSprites[RT_COMMAND_BUF_COUNT][MAX_RECURSION_LEVELS];

void CObjManager::UnloadFarObjects()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (int j = 0; j < MAX_RECURSION_LEVELS; ++j)
        {
            stl::free_container(arrVegetationSprites[i][j]);
        }
    }
}

void CObjManager::RenderFarObjects(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    int nCount = 0;
    for (int t = 0; t < nThreadsNum; t++)
    {
        nCount += m_arrVegetationSprites[passInfo.GetRecursiveLevel()][t].size();
    }

    if (!m_REFarTreeSprites)
    {
        m_REFarTreeSprites = (CREFarTreeSprites*)GetRenderer()->EF_CreateRE(eDATA_FarTreeSprites);
    }
    if (m_REFarTreeSprites && GetCVars()->e_VegetationSprites && nCount && !GetCVars()->e_DefaultMaterial)
    {
        arrVegetationSprites[passInfo.ThreadID()][passInfo.GetRecursiveLevel()].Clear();

        for (int t = 0; t < nThreadsNum; t++)
        {
            CThreadSafeRendererContainer<SVegetationSpriteInfo>& rList = m_arrVegetationSprites[passInfo.GetRecursiveLevel()][t];
            if (rList.size())
            {
                rList.CoalesceMemory();
                for (size_t i = 0; i < rList.size(); ++i)
                {
                    arrVegetationSprites[passInfo.ThreadID()][passInfo.GetRecursiveLevel()].Add(rList[i]);
                }
            }
        }
        m_REFarTreeSprites->m_arrVegetationSprites[passInfo.ThreadID()][passInfo.GetRecursiveLevel()] = &arrVegetationSprites[passInfo.ThreadID()][passInfo.GetRecursiveLevel()];
        CRenderObject* pObj = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
        if (!pObj)
        {
            return;
        }
        pObj->m_II.m_Matrix.SetIdentity();
        SShaderItem shItem(m_p3DEngine->m_pFarTreeSprites);
        SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
        GetRenderer()->EF_AddEf(m_REFarTreeSprites, shItem, pObj, passInfo, EFSLIST_GENERAL, 1, rendItemSorter);
    }
}

void CObjManager::DrawFarObjects(float fMaxViewDist, const SRenderingPassInfo& passInfo)
{
    if (!GetCVars()->e_VegetationSprites)
    {
        return;
    }

    FUNCTION_PROFILER_3DENGINE;

    if (passInfo.GetRecursiveLevel() >= MAX_RECURSION_LEVELS)
    {
        assert(!"Recursion depther than MAX_RECURSION_LEVELS is not supported");
        return;
    }

    //////////////////////////////////////////////////////////////////////////////////////
    //  Draw all far
    //////////////////////////////////////////////////////////////////////////////////////
}

void CObjManager::GenerateFarObjects(float fMaxViewDist, const SRenderingPassInfo& passInfo)
{
    if (!GetCVars()->e_VegetationSprites)
    {
        return;
    }

    FUNCTION_PROFILER_3DENGINE;

    if (passInfo.GetRecursiveLevel() >= MAX_RECURSION_LEVELS)
    {
        assert(!"Recursion depther than MAX_RECURSION_LEVELS is not supported");
        return;
    }
}
