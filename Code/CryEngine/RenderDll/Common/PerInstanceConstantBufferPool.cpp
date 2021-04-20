/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include "PerInstanceConstantBufferPool.h"
#include "DevBuffer.h"
#include "RenderView.h"
#include "RenderPipeline.h"
#include "Include_HLSL_CPP_Shared.h"

#if !defined(NULL_RENDERER)
#include "DriverD3D.h"
#endif

namespace
{
    void BuildPerInstanceConstantBuffer(HLSL_PerInstanceConstantBuffer* outBuffer, CRenderObject* renderObject, float realTime, float realTimePrev)
    {
        AZ::u64 objectFlags = renderObject->m_ObjFlags;

        outBuffer->SPIObjWorldMat = renderObject->GetMatrix();

        SBending* bending = renderObject->m_data.m_pBending;
        if (bending != nullptr)
        {
            outBuffer->SPIBendInfo = bending->GetShaderConstants(realTime);
        }

        bending = renderObject->m_data.m_BendingPrev;
        if (bending != nullptr)
        {
            outBuffer->SPIBendInfoPrev = bending->GetShaderConstants(realTimePrev);
        }

        outBuffer->SPIAmbientOpacity.x = renderObject->m_II.m_AmbColor.r;
        outBuffer->SPIAmbientOpacity.y = renderObject->m_II.m_AmbColor.g;
        outBuffer->SPIAmbientOpacity.z = renderObject->m_II.m_AmbColor.b;
        outBuffer->SPIAmbientOpacity.w = renderObject->m_fAlpha;

        const bool bDissolve = (objectFlags & (FOB_DISSOLVE_OUT | FOB_DISSOLVE)) != 0;
        const bool bDissolveOut = (objectFlags & FOB_DISSOLVE_OUT) != 0;

        outBuffer->SPIDissolveRef.x = bDissolve ? (float)(renderObject->m_DissolveRef) * (1.0f / 255.0f) : 0.0f;
        outBuffer->SPIDissolveRef.y = bDissolveOut ? 1.0f : -1.0f;
        outBuffer->SPIDissolveRef.z = 0.0f;
        outBuffer->SPIDissolveRef.w = 0.0f;
    }
}

PerInstanceConstantBufferPool::PerInstanceConstantBufferPool()
    : m_CurrentRenderItem{}
    , m_UpdateConstantBuffer{}
    , m_UpdateIndirectConstantBuffer{}
    , m_PooledConstantBuffer{}
#if defined(FEATURE_SPI_INDEXED_CB)
    , m_PooledIndirectConstantBuffer{}
#endif
{
}

void PerInstanceConstantBufferPool::Init()
{
    for (AZ::u32 i = 0; i < SPI_NUM_STATIC_INST_CB; ++i)
    {
        m_PooledConstantBuffer[i] = NULL;
    }
#if defined(FEATURE_SPI_INDEXED_CB)
    for (AZ::u32 i = 0; i < SPI_NUM_INSTS_PER_CB; ++i)
    {
        m_PooledIndirectConstantBuffer[i] = NULL;
    }
#endif
    m_UpdateConstantBuffer = nullptr;
    m_UpdateIndirectConstantBuffer = nullptr;
}

void PerInstanceConstantBufferPool::Shutdown()
{
    for (AZ::u32 i = 0; i < SPI_NUM_STATIC_INST_CB; ++i)
    {
        SAFE_RELEASE(m_PooledConstantBuffer[i]);
    }
#if defined(FEATURE_SPI_INDEXED_CB)
    for (AZ::u32 i = 0; i < SPI_NUM_INSTS_PER_CB; ++i)
    {
        SAFE_RELEASE(m_PooledIndirectConstantBuffer[i]);
    }
#endif
    SAFE_RELEASE(m_UpdateIndirectConstantBuffer);
    SAFE_RELEASE(m_UpdateConstantBuffer);
}

void PerInstanceConstantBufferPool::Update(CRenderView& renderView, float realTime)
{
#if !defined(NULL_RENDERER)

    if (m_PooledConstantBuffer[0] == nullptr)
    {
        auto& bufferManager = gRenDev->m_DevBufMan;

        for (AZ::u32 i = 0; i < SPI_NUM_STATIC_INST_CB; ++i)
        {
            m_PooledConstantBuffer[i] = bufferManager.CreateConstantBuffer(
                "PerInstancePool",
                SPI_NUM_INSTS_PER_CB * sizeof(HLSL_PerInstanceConstantBuffer),
                AzRHI::ConstantBufferUsage::Dynamic,
                AzRHI::ConstantBufferFlags::DenyStreaming);
        }

#ifdef FEATURE_SPI_INDEXED_CB
        for (AZ::u32 i = 0; i < SPI_NUM_INSTS_PER_CB; ++i)
        {
            AZ::u32 data[4] = { i, 0, 0, 0 };
            m_PooledIndirectConstantBuffer[i] = bufferManager.CreateConstantBuffer(
                "PerInstanceIndirectPool",
                sizeof(AZ::u32) * 4,
                AzRHI::ConstantBufferUsage::Static,
                AzRHI::ConstantBufferFlags::DenyStreaming);
            m_PooledIndirectConstantBuffer[i]->UpdateBuffer(&data, sizeof(AZ::u32) * 4);
        }
#endif

        m_UpdateConstantBuffer = bufferManager.CreateConstantBuffer(
            "PerInstanceUpdate",
            SPI_NUM_INSTS_PER_CB * sizeof(HLSL_PerInstanceConstantBuffer),
            AzRHI::ConstantBufferUsage::Dynamic,
            AzRHI::ConstantBufferFlags::DenyStreaming);

        AZ::u32 data[4] = { 0, 0, 0, 0 };
        m_UpdateIndirectConstantBuffer = bufferManager.CreateConstantBuffer(
            "PerInstanceIndirectUpdate",
            sizeof(AZ::u32) * 4,
            AzRHI::ConstantBufferUsage::Static,
            AzRHI::ConstantBufferFlags::DenyStreaming);

        m_UpdateIndirectConstantBuffer->UpdateBuffer(&data, sizeof(AZ::u32) * 4);
    }

    PROFILE_FRAME(UpdatePerInstanceConstants);
    AZ_TRACE_METHOD();

    AZ::u32 nextBufferIdx = 0;
    AZ::u32 nextInstanceIdx = 0;
    AZ::u32 constantBufferIdxLimit = SPI_NUM_STATIC_INST_CB;
    void* mappedData = nullptr;
        
    // Assign half of the constant buffer budget per eye when in VR mode
    if (gcpRendD3D->GetIStereoRenderer()->IsRenderingToHMD())
    {
        // For the right eye (rendered second), begin indexing half way into the array
        if (gRenDev->m_CurRenderEye == STEREO_EYE_RIGHT)
        {
            nextBufferIdx = constantBufferIdxLimit / 2;
        }
        else
        {
            // For the left eye, just reduce the limit by half
            constantBufferIdxLimit /= 2;
        }
    }

    float realTimePrev = realTime - CRenderer::GetElapsedTime();

    for (AZ::u32 renderListIdx = EFSLIST_PREPROCESS; renderListIdx < EFSLIST_NUM; renderListIdx++)
    {
        for (AZ::u32 bAfterWater = 0; bAfterWater < 2; bAfterWater++)
        {
            auto& renderItems = renderView.GetRenderItems(bAfterWater, renderListIdx);

            for (AZ::u32 itemIndex = 0; itemIndex < renderItems.size(); itemIndex++)
            {
                SRendItem* renderItem = &renderItems[itemIndex];
                CRenderObject* renderObject = renderItem->pObj;

                if (!renderObject)
                {
                    AZ_Assert(false, "Failed to update static inst buffer pool, index %u - the render object is null", nextBufferIdx);
                    continue;
                }

                if (renderObject->m_PerInstanceConstantBufferKey.IsValid())
                {
                    continue;
                }

                if (nextBufferIdx >= constantBufferIdxLimit)
                {
                    auto* renderer = gEnv->pRenderer;

                    int nDrawCalls, nShadowGenDrawCalls;
                    renderer->GetCurrentNumberOfDrawCalls(nDrawCalls, nShadowGenDrawCalls);
                    int nTotalDrawCalls = nDrawCalls + nShadowGenDrawCalls;
                    int nTotalInstanced = nTotalDrawCalls + renderer->GetNumGeomInstanceDrawCalls();
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Ran out of static inst buffers -- DP: %04d ShadowGen: %04d Total: %04d Instanced: %04d", nDrawCalls, nShadowGenDrawCalls, nTotalDrawCalls, nTotalInstanced);
                    return;
                }

                AzRHI::ConstantBuffer* constantBuffer = m_PooledConstantBuffer[nextBufferIdx];
                if (nextInstanceIdx == 0)
                {
                    mappedData = constantBuffer->BeginWrite();
                    if (!mappedData)
                    {
                        AZ_Error("Renderer", false, "Failed to update static inst buffer pool, index %u", nextBufferIdx);
                        return;
                    }
                }

                HLSL_PerInstanceConstantBuffer* outputData = reinterpret_cast<HLSL_PerInstanceConstantBuffer*>(mappedData) + nextInstanceIdx;
                BuildPerInstanceConstantBuffer(outputData, renderObject, realTime, realTimePrev);

                renderObject->m_PerInstanceConstantBufferKey.m_Id = nextInstanceIdx + (nextBufferIdx * SPI_NUM_INSTS_PER_CB);
#ifdef FEATURE_SPI_INDEXED_CB
                renderObject->m_PerInstanceConstantBufferKey.m_IndirectId = nextInstanceIdx;
#endif
                nextInstanceIdx++;
                if (nextInstanceIdx == SPI_NUM_INSTS_PER_CB)
                {
                    constantBuffer->EndWrite();
                    nextInstanceIdx = 0;
                    nextBufferIdx++;
                }
            }
        }
    }

    if (nextInstanceIdx != 0)
    {
        m_PooledConstantBuffer[nextBufferIdx]->EndWrite();
    }
#endif
}

void PerInstanceConstantBufferPool::SetConstantBuffer(SRendItem* renderItem)
{
    CRenderObject* object = renderItem->pObj;
    const AZ::u32 directId = object->m_PerInstanceConstantBufferKey.m_Id;

    if (directId == 0xffff)
    {
        return;
    }

    m_CurrentRenderItem = renderItem;

    AZ::u32 bufferIndex = directId / SPI_NUM_INSTS_PER_CB;

    auto& deviceManager = gRenDev->m_DevMan;

#if (SPI_NUM_INSTS_PER_CB == 1)
    deviceManager.BindConstantBuffer(eHWSC_Vertex, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI);
#elif defined(FEATURE_SPI_INDEXED_CB)
    AZ::u32 indirectId = object->m_PerInstanceConstantBufferKey.m_IndirectId;

    if (indirectId >= SPI_NUM_INSTS_PER_CB)
    {
        CryLogAlways("ERROR: SetBuffer - indirect index is invalid");
        return;
    }

    deviceManager.BindConstantBuffer(eHWSC_Vertex, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI);
    deviceManager.BindConstantBuffer(eHWSC_Vertex, m_PooledIndirectConstantBuffer[indirectId], eConstantBufferShaderSlot_SPIIndex);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, m_PooledIndirectConstantBuffer[indirectId], eConstantBufferShaderSlot_SPIIndex);
#else
    AZ::u32 itemIndex = directId % SPI_NUM_INSTS_PER_CB;
    AZ::u32 first[1] = {itemIndex * static_cast<AZ::u32>(sizeof(HLSL_PerInstanceConstantBuffer))};
    AZ::u32 count[1] = {static_cast<AZ::u32>(sizeof(HLSL_PerInstanceConstantBuffer))};

    deviceManager.BindConstantBuffer(eHWSC_Vertex, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI, first[0], count[0]);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, m_PooledConstantBuffer[bufferIndex], eConstantBufferShaderSlot_SPI, first[0], count[0]);
#endif
}

void PerInstanceConstantBufferPool::UpdateConstantBuffer(ConstantUpdateCB constantUpdateCallback, float realTime)
{
    AZ_Assert(m_CurrentRenderItem, "current render item is null");

    CRenderObject* renderObject = m_CurrentRenderItem->pObj;
    if (!renderObject)
    {
        AZ_Assert(false, "Failed to update static inst buffer - the current render object is null");
        return;
    }

    float realTimePrev = realTime - CRenderer::GetElapsedTime();

    void* mappedData = m_UpdateConstantBuffer->BeginWrite();    
    if (!mappedData)
    {
        AZ_Error("Renderer", false, "Failed to update static inst buffer");
        return;
    }

    BuildPerInstanceConstantBuffer(reinterpret_cast<HLSL_PerInstanceConstantBuffer*>(mappedData), renderObject, realTime, realTimePrev);

    constantUpdateCallback(mappedData);
    m_UpdateConstantBuffer->EndWrite();

    auto& devManager = gRenDev->m_DevMan;
    devManager.BindConstantBuffer(eHWSC_Vertex, m_UpdateConstantBuffer, eConstantBufferShaderSlot_SPI);
    devManager.BindConstantBuffer(eHWSC_Pixel, m_UpdateConstantBuffer, eConstantBufferShaderSlot_SPI);

#if defined(FEATURE_SPI_INDEXED_CB)
    devManager.BindConstantBuffer(eHWSC_Vertex, m_UpdateIndirectConstantBuffer, eConstantBufferShaderSlot_SPIIndex);
    devManager.BindConstantBuffer(eHWSC_Pixel, m_UpdateIndirectConstantBuffer, eConstantBufferShaderSlot_SPIIndex);
#endif
}
