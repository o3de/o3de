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
#include "DriverD3D.h"
#include "ConstantBufferCache.h"

namespace
{
    void ReportStatistics(AZ::u32 registerCountMax)
    {
#ifdef DO_RENDERLOG
        if (CD3D9Renderer::CV_d3d11_CBUpdateStats)
        {
            static unsigned int s_lastFrame(0);
            static unsigned int s_numCalls(0);
            static unsigned int s_minUpdateBytes(0);
            static unsigned int s_maxUpdateBytes(0);
            static unsigned int s_totalUpdateBytes(0);

            unsigned int updateBytes = (unsigned int)(registerCountMax * sizeof(Vec4));
            unsigned int curFrame = gcpRendD3D->GetFrameID(false);
            if (s_lastFrame != curFrame)
            {
                if (s_lastFrame != 0)
                {
                    unsigned int avgUpdateBytes = s_totalUpdateBytes / s_numCalls;
                    gEnv->pLog->Log("-------------------------------------------------------");
                    gEnv->pLog->Log("CB update statistics for frame %d:", s_lastFrame);
                    gEnv->pLog->Log("#UpdateSubresource() = %d calls", s_numCalls);
                    gEnv->pLog->Log("SmallestTransfer = %d kb (%d bytes)", (s_minUpdateBytes + 1023) >> 10, s_minUpdateBytes);
                    gEnv->pLog->Log("BiggestTransfer = %d kb (%d bytes)", (s_maxUpdateBytes + 1023) >> 10, s_maxUpdateBytes);
                    gEnv->pLog->Log("AvgTransfer = %d kb (%d bytes)", (avgUpdateBytes + 1023) >> 10, avgUpdateBytes);
                    gEnv->pLog->Log("TotalTransfer = %d kb (%d bytes)", (s_totalUpdateBytes + 1023) >> 10, s_totalUpdateBytes);
                }

                s_lastFrame = curFrame;
                s_numCalls = 1;
                s_minUpdateBytes = updateBytes;
                s_maxUpdateBytes = updateBytes;
                s_totalUpdateBytes = updateBytes;
            }
            else
            {
                ++s_numCalls;
                s_minUpdateBytes = min(updateBytes, s_minUpdateBytes);
                s_maxUpdateBytes = max(updateBytes, s_maxUpdateBytes);
                s_totalUpdateBytes += updateBytes;
            }
        }
#endif
    }
}

namespace AzRHI
{
    ConstantBufferCache::ConstantBufferCache()
        : m_Cache{}
        , m_DeviceManager{ gcpRendD3D->m_DevMan }
        , m_DeviceBufferManager{ gcpRendD3D->m_DevBufMan }
    {
        for (AZ::u32 shaderClass = 0; shaderClass < eHWSC_Num; shaderClass++)
        {
            const AZ::u32 registerCountMax = GetConstantRegisterCountMax(EHWShaderClass(shaderClass));
            if (registerCountMax)
            {
                for (AZ::u32 shaderSlot = 0; shaderSlot < eConstantBufferShaderSlot_Count; shaderSlot++)
                {
                    m_Buffers[shaderClass][shaderSlot].resize(registerCountMax);
                }
            }
        }
    }

    void ConstantBufferCache::Reset()
    {
        for (AZ::u32 shaderClass = 0; shaderClass < eHWSC_Num; shaderClass++)
        {
            for (AZ::u32 shaderSlot = 0; shaderSlot < eConstantBufferShaderSlot_Count; shaderSlot++)
            {
                for (AzRHI::ConstantBuffer*& buffer : m_Buffers[shaderClass][shaderSlot])
                {
                    if (buffer)
                    {
                        buffer->Release();
                        buffer = nullptr;
                    }
                }
            }
        }
    }

    void ConstantBufferCache::CommitAll()
    {
        for (CacheEntryKey key : m_DirtyEntries)
        {
            CacheEntry& entry = GetCacheEntry(key.shaderClass, key.shaderSlot);

            TryCommitConstantBuffer(entry);

            m_DeviceManager.BindConstantBuffer(key.shaderClass, entry.m_buffer, key.shaderSlot);
        }
        m_DirtyEntries.clear();
    }

    void* ConstantBufferCache::MapConstantBuffer(
        EHWShaderClass shaderClass,
        EConstantBufferShaderSlot shaderSlot,
        AZ::u32 registerCountMax)
    {
        CacheEntry& entry = GetCacheEntry(shaderClass, shaderSlot);

        if (entry.m_registerCountMax != registerCountMax)
        {
            TryCommitConstantBuffer(entry);
        }

        if (!entry.m_mappedData)
        {
            if (!entry.m_bExternalActive)
            {
                auto& bufferList = m_Buffers[shaderClass][shaderSlot];

                if (!bufferList[registerCountMax])
                {
                    AzRHI::ConstantBuffer* constantBuffer = m_DeviceBufferManager.CreateConstantBuffer(
                        "ConstantBufferCache",
                        registerCountMax * sizeof(Vec4),
                        AzRHI::ConstantBufferUsage::Dynamic,
                        AzRHI::ConstantBufferFlags::None);

                    if (!constantBuffer)
                    {
                        AZ_Error(
                            "ConstantBufferCache",
                            false,
                            "ERROR: CBuffer %d Create() failed for shader %s result %d",
                            (AZ::u32)shaderSlot,
                            gRenDev->m_RP.m_pShader ? gRenDev->m_RP.m_pShader->GetName() : "Unknown",
                            -1);
                        return nullptr;
                    }
                    bufferList[registerCountMax] = constantBuffer;
                }

                entry.m_buffer = bufferList[registerCountMax];

                CacheEntryKey dirty;
                dirty.shaderClass = shaderClass;
                dirty.shaderSlot = shaderSlot;
                m_DirtyEntries.push_back(dirty);
            }

            {
                STALL_PROFILER("set const_buffer");
                AZ_Assert(entry.m_buffer, "buffer should be valid");
                entry.m_registerCountMax = registerCountMax;
                entry.m_mappedData = (Vec4*)entry.m_buffer->BeginWrite();
            }

            ReportStatistics(registerCountMax);
        }

        return entry.m_mappedData;
    }

    AzRHI::ConstantBuffer* ConstantBufferCache::GetConstantBuffer(EHWShaderClass shaderClass, EConstantBufferShaderSlot shaderSlot, AZ::u32 registerCount)
    {
        return m_Buffers[shaderClass][shaderSlot][registerCount];
    }


    bool ConstantBufferCache::TryCommitConstantBuffer(CacheEntry& entry)
    {
        if (entry.m_mappedData)
        {
            entry.m_buffer->EndWrite();
            entry.m_mappedData = nullptr;
            return true;
        }
        return false;
    }

    void ConstantBufferCache::BeginExternalConstantBuffer(
        EHWShaderClass shaderClass,
        EConstantBufferShaderSlot shaderSlot,
        AzRHI::ConstantBuffer* externalBuffer,
        AZ::u32 registerCountMax)
    {
        CacheEntry& entry = GetCacheEntry(shaderClass, shaderSlot);

        if (entry.m_mappedData)
        {
            TryCommitConstantBuffer(entry);
        }

        AZ_Assert(entry.m_bExternalActive == false, "Already injected external constant buffer");
        entry.m_bExternalActive = true;
        entry.m_buffer = externalBuffer;
        entry.m_mappedData = nullptr;
        entry.m_registerCountMax = registerCountMax;
    }

    void ConstantBufferCache::EndExternalConstantBuffer(
        EHWShaderClass shaderClass,
        EConstantBufferShaderSlot shaderSlot)
    {
        CacheEntry& entry = GetCacheEntry(shaderClass, shaderSlot);

        if (entry.m_mappedData)
        {
            entry.m_buffer->EndWrite();
        }

        entry.m_buffer = nullptr;
        entry.m_mappedData = nullptr;
        entry.m_registerCountMax = 0;
        entry.m_bExternalActive = false;
    }
}
