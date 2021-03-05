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
#pragma once

namespace AzRHI
{
    class ConstantBufferCache
    {
    public:
        ConstantBufferCache();

        static ConstantBufferCache& GetInstance()
        {
            static ConstantBufferCache instance;
            return instance;
        }

        void Reset();

        ConstantBuffer* GetConstantBuffer(EHWShaderClass shaderClass, EConstantBufferShaderSlot shaderSlot, AZ::u32 registerCountMax);

        void* MapConstantBuffer(EHWShaderClass shaderClass, EConstantBufferShaderSlot shaderslot, AZ::u32 registerCountMax);

        void CommitAll();

        void BeginExternalConstantBuffer(
            EHWShaderClass shaderClass,
            EConstantBufferShaderSlot shaderSlot,
            AzRHI::ConstantBuffer* externalBuffer,
            AZ::u32 registerCountMax);

        void EndExternalConstantBuffer(
            EHWShaderClass shaderClass,
            EConstantBufferShaderSlot shaderSlot);

        inline void WriteConstants(
            EHWShaderClass shaderClass,
            EConstantBufferShaderSlot shaderSlot,
            const Vec4* constants,
            AZ::u32 registerOffset,
            AZ::u32 registerCount,
            AZ::u32 registerCountMax)
        {
#if !defined(RELEASE)
            if (registerOffset + registerCount > registerCountMax)
            {
                AZ_Error(
                    "ConstantBufferCache",
                    false,
                    "Attempt to modify constant buffer: %d outside of the range (%d+%d > %d) (Shader: %s)",
                    (AZ::u32)shaderSlot, registerOffset, registerCount, registerCountMax,
                    gRenDev->m_RP.m_pShader ? gRenDev->m_RP.m_pShader->GetName() : "Unknown");
                return;
            }
#endif

            Vec4* mappedData = reinterpret_cast<Vec4*>(MapConstantBuffer(shaderClass, shaderSlot, registerCountMax));
            if (mappedData)
            {
                SIMDCopy(&mappedData[registerOffset], constants, registerCount);
            }
        }

        inline void WriteConstants(
            EHWShaderClass shaderClass,
            const SCGBind* parameter,
            const void* data,
            AZ::u32 registerCountMax)
        {
            if (parameter)
            {
                EConstantBufferShaderSlot shaderSlot = (EConstantBufferShaderSlot)parameter->m_BindingSlot;
                AZ_Assert(shaderSlot < eConstantBufferShaderSlot_Count, "Invalid shader slot");

                WriteConstants(
                    shaderClass,
                    shaderSlot,
                    reinterpret_cast<const Vec4*>(data),
                    parameter->m_RegisterOffset,
                    parameter->m_RegisterCount,
                    registerCountMax);
            }
        }

        inline void WriteConstants(
            EHWShaderClass shaderClass,
            const SCGBind* parameter,
            const void* data,
            AZ::u32 registerCount,
            AZ::u32 registerCountMax)
        {
            if (parameter)
            {
                EConstantBufferShaderSlot shaderSlot = (EConstantBufferShaderSlot)parameter->m_BindingSlot;
                AZ_Assert(shaderSlot < eConstantBufferShaderSlot_Count, "Invalid shader slot");

                WriteConstants(
                    shaderClass,
                    shaderSlot,
                    reinterpret_cast<const Vec4*>(data),
                    parameter->m_RegisterOffset,
                    registerCount,
                    registerCountMax);
            }
        }

    private:
        struct CacheEntry
        {
            CacheEntry()
                : m_registerCountMax{}
                , m_bExternalActive{}
                , m_mappedData{}
                , m_buffer{}
            {}

            AZ::u32 m_registerCountMax : 16;
            AZ::u32 m_bExternalActive  : 1;
            Vec4* m_mappedData;
            AzRHI::ConstantBuffer* m_buffer;
        };

        bool TryCommitConstantBuffer(CacheEntry& entry);

        inline CacheEntry& GetCacheEntry(EHWShaderClass shaderClass, EConstantBufferShaderSlot shaderSlot)
        {
            return m_Cache[shaderClass][shaderSlot];
        }

        struct CacheEntryKey
        {
            EHWShaderClass shaderClass;
            EConstantBufferShaderSlot shaderSlot;
        };

        CDeviceManager& m_DeviceManager;
        CDeviceBufferManager& m_DeviceBufferManager;
        CacheEntry m_Cache[eHWSC_Num][eConstantBufferShaderSlot_Count];
        AZStd::vector<AzRHI::ConstantBuffer*> m_Buffers[eHWSC_Num][eConstantBufferShaderSlot_Count];
        AZStd::vector<CacheEntryKey> m_DirtyEntries;
    };
}