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
#pragma once

#include "Defs.h"

#if (defined(WIN32) || defined(APPLE) || defined(LINUX) || defined(USE_FEATURE_SPI_INDEXED_CB_BY_DEFAULT))
    #define FEATURE_SPI_INDEXED_CB

    #if defined(DONT_USE_SPI_INDEXED_CB)
        #undef FEATURE_SPI_INDEXED_CB
    #endif
#endif

// DirectX 11.0
#if defined(WIN32) || defined(LINUX) || defined(APPLE) || defined(USE_FEATURE_SPI_INDEXED_CB_BY_DEFAULT)

    #define SPI_NUM_STATIC_INST_CB_DEFAULT (2048 * 64)

    #ifdef FEATURE_SPI_INDEXED_CB
        #define SPI_NUM_INSTS_PER_CB        128 // Must match SPI struct in FXConstantDefs.cfi
        #define SPI_NUM_STATIC_INST_CB  (SPI_NUM_STATIC_INST_CB_DEFAULT / SPI_NUM_INSTS_PER_CB)
    #else
        #define SPI_NUM_INSTS_PER_CB        1
        #define SPI_NUM_STATIC_INST_CB  SPI_NUM_STATIC_INST_CB_DEFAULT
    #endif // FEATURE_SPI_INDEXED_CB

// DirectX 11.1 and higher
#else
    #define SPI_NUM_INSTS_PER_CB    2048
    #define SPI_NUM_STATIC_INST_CB  64
#endif

struct SRendItem;

class IPerInstanceConstantBufferPool
{
    virtual void SetConstantBuffer(SRendItem* renderItem) = 0;
};

class PerInstanceConstantBufferPool : public IPerInstanceConstantBufferPool
{
public:
    PerInstanceConstantBufferPool();

    using ConstantUpdateCB = AZStd::function<void(void*)>;

    inline SRendItem* GetCurrentRenderItem()
    {
        return m_CurrentRenderItem;
    }

    void Init();
    void Shutdown();

    void SetConstantBuffer(SRendItem* renderItem);
    void UpdateConstantBuffer(ConstantUpdateCB callback, float realTime);
    void Update(CRenderView& renderView, float realTime);
    
private:
    SRendItem* m_CurrentRenderItem;

    AzRHI::ConstantBuffer* m_PooledConstantBuffer[SPI_NUM_STATIC_INST_CB];
#if defined(FEATURE_SPI_INDEXED_CB)
    AzRHI::ConstantBuffer* m_PooledIndirectConstantBuffer[SPI_NUM_INSTS_PER_CB];
#endif

    AzRHI::ConstantBuffer* m_UpdateConstantBuffer;
    AzRHI::ConstantBuffer* m_UpdateIndirectConstantBuffer;
};
