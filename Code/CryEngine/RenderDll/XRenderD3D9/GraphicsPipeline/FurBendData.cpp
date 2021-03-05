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
#include "RenderDll_precompiled.h"
#include "FurBendData.h"
#include <AzCore/Math/Matrix4x4.h>

/*static*/ FurBendData& FurBendData::Get()
{
    static FurBendData s_instance;
    return s_instance;
}

void FurBendData::SetupObject(CRenderObject& renderObject, const SRenderingPassInfo& passInfo)
{
    if (passInfo.IsRecursivePass())
    {
        return;
    }

    renderObject.m_ObjFlags &= ~FOB_HAS_PREVMATRIX;

    // Perhaps use a different distance for fur movement bending?
    if (renderObject.m_fDistance < CRenderer::CV_r_MotionBlurMaxViewDist)
    {
        const AZ::u32 currentFrameId = passInfo.GetMainFrameID();
        const uintptr_t objectId = reinterpret_cast<uintptr_t>(renderObject.m_pRenderNode);
        auto iter = m_objects.find(objectId);
        if (iter != m_objects.end())
        {
            ObjectParameters* pParams = &iter->second;

            // Perhaps use a different threshold for fur movement bending?
            const float fThreshold = CRenderer::CV_r_MotionBlurThreshold;
            if (!Matrix34::IsEquivalent(pParams->m_worldMatrix, renderObject.m_II.m_Matrix, fThreshold))
            {
                renderObject.m_ObjFlags |= FOB_HAS_PREVMATRIX;

                // Slerp rotation, and lerp translation
                // FUTURE: THis will be way more efficient once RenderDll uses AZ Math fully
                float fBendBias = CRenderer::CV_r_FurMovementBendingBias; // Could instead retrieve stiffness from material
                fBendBias = AZ::GetClamp(fBendBias, 0.0f, 1.0f);

                AZ::Vector4 rows[4];
                rows[0] = AZ::Vector4::CreateFromFloat4(renderObject.m_II.m_Matrix.GetData());
                rows[1] = AZ::Vector4::CreateFromFloat4(renderObject.m_II.m_Matrix.GetData() + 4);
                rows[2] = AZ::Vector4::CreateFromFloat4(renderObject.m_II.m_Matrix.GetData() + 8);
                rows[3] = AZ::Vector4::CreateAxisW();

                AZ::Matrix4x4 currMatrix = AZ::Matrix4x4::CreateFromRows(rows[0], rows[1], rows[2], rows[3]);
                rows[0] = AZ::Vector4::CreateFromFloat4(pParams->m_worldMatrix.GetData());
                rows[1] = AZ::Vector4::CreateFromFloat4(pParams->m_worldMatrix.GetData() + 4);
                rows[2] = AZ::Vector4::CreateFromFloat4(pParams->m_worldMatrix.GetData() + 8);
                AZ::Matrix4x4 prevMatrix = AZ::Matrix4x4::CreateFromRows(rows[0], rows[1], rows[2], rows[3]);

                // Lerp should really be time-based, not frame-based
                currMatrix = AZ::Matrix4x4::CreateInterpolated(prevMatrix, currMatrix, fBendBias);

                currMatrix.GetRows(&rows[0], &rows[1], &rows[2], &rows[3]);
                float vals[12];
                rows[0].StoreToFloat4(&vals[0]);
                rows[1].StoreToFloat4(&vals[4]);
                rows[2].StoreToFloat4(&vals[8]);
                memcpy(pParams->m_worldMatrix.GetData(), vals, sizeof(float) * 12);

                pParams->m_updateFrameId = currentFrameId;
                pParams->m_pRenderObject = &renderObject;
            }
        }
        else
        {
            uint32 fillThreadId = passInfo.ThreadID();
            m_fillData[fillThreadId].push_back(ObjectMap::value_type(objectId, ObjectParameters(renderObject, renderObject.m_II.m_Matrix, currentFrameId)));
        }
    }
}

void FurBendData::GetPrevObjToWorldMat(CRenderObject& renderObject, Matrix44A& worldMatrix)
{
    if (renderObject.m_ObjFlags & FOB_HAS_PREVMATRIX)
    {
        const uintptr_t objectId = reinterpret_cast<uintptr_t>(renderObject.m_pRenderNode);

        auto iter = m_objects.find(objectId);
        if (iter != m_objects.end())
        {
            worldMatrix = iter->second.m_worldMatrix;
            return;
        }
    }

    worldMatrix = renderObject.m_II.m_Matrix;
}

void FurBendData::InsertNewElements()
{
    AZ::u32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    if (!m_fillData[nThreadID].empty())
    {
        m_fillData[nThreadID].CoalesceMemory();
        m_objects.insert(&m_fillData[nThreadID][0], &m_fillData[nThreadID][0] + m_fillData[nThreadID].size());
        m_fillData[nThreadID].resize(0);
    }
}

void FurBendData::FreeData()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_fillData[i].clear();
    }

    for (size_t i = 0; i < sizeof(m_objects) / sizeof(m_objects[0]); ++i)
    {
        stl::reconstruct(m_objects[i]);
    }
}

void FurBendData::OnBeginFrame()
{
    AZ_Assert(!gRenDev->m_pRT || gRenDev->m_pRT->IsMainThread(), "");

    const AZ::u32 frameId = gRenDev->GetFrameID(false);
    m_objects.erase_if([frameId](const VectorMap<uintptr_t, ObjectParameters >::value_type& object)
    {
        const AZ::u32 discardThreshold = 60;
        return (frameId - object.second.m_updateFrameId) > discardThreshold;
    });
}
