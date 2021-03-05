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

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Vertex Data container optmized for direct VideoMemory access on Consoles
// No driver overhead, the lock function returns a direct pointer into Video Memory
// which is used by the GPU
// *NOTE: The programmer has to ensure that the Video Memory is not overwritten
//              while beeing used. For this the container provides additional fence and
//              wait for fence functions. Also double buffering of the container could be needed
// *NOTE: On non console platforms, this container is using the driver facilities to ensure
//              no memory is overwrite. This could mean additional memory allocated by the driver
template <class IndexType>
class FencedIB
{
public:
    FencedIB(uint32 nIndexCount, uint32 nIndexStride);
    ~FencedIB();

    IndexType*      LockIB(uint32 nLockCount);
    void                    UnlockIB();
    HRESULT             Bind(uint32 nOffs);

    uint32 GetIndexCount() const;

    void SetFence();
    void WaitForFence();

private:
    D3DBuffer*     m_pIB;
    uint32                      m_nIndexCount;
    IndexType*              m_pLockedData;
    uint32                      m_nIndexStride;
    DeviceFenceHandle   m_Fence;
};

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
FencedIB<IndexType>::FencedIB(uint32 nIndexCount, uint32 nIndexStride)
    : m_pIB(NULL)
    , m_pLockedData(NULL)
    , m_nIndexStride(nIndexStride)
    , m_nIndexCount(nIndexCount)
{
    HRESULT hr = gRenDev->m_DevMan.CreateDirectAccessBuffer(m_nIndexCount, m_nIndexStride, CDeviceManager::BIND_INDEX_BUFFER, (D3DBuffer**)&m_pIB);
    CHECK_HRESULT(hr);

    gRenDev->m_DevMan.CreateFence(m_Fence);
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
FencedIB<IndexType>::~FencedIB()
{
    UnlockIB();
    if (m_pIB)
    {
        gRenDev->m_DevMan.DestroyDirectAccessBuffer((D3DBuffer*)m_pIB);
    }

    gRenDev->m_DevMan.ReleaseFence(m_Fence);
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
IndexType* FencedIB<IndexType>::LockIB([[maybe_unused]] uint32 nLockCount)
{
    // Ensure there is enough space in the VB for this data
    assert (nLockCount <= m_nIndexCount);

    if (m_pLockedData)
    {
        return m_pLockedData;
    }

    if (m_pIB)
    {
        gRenDev->m_DevMan.LockDirectAccessBuffer((D3DBuffer*)m_pIB, CDeviceManager::BIND_INDEX_BUFFER, (void**)&m_pLockedData);
    }

    return m_pLockedData;
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
void FencedIB<IndexType>::UnlockIB()
{
    if (m_pLockedData && m_pIB)
    {
        gRenDev->m_DevMan.UnlockDirectAccessBuffer((D3DBuffer*)m_pIB, CDeviceManager::BIND_INDEX_BUFFER);
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(FencedIB_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#   else
        CDeviceManager::InvalidateCpuCache(m_pLockedData, 0, m_nIndexCount * m_nIndexStride);
        CDeviceManager::InvalidateGpuCache((D3DBuffer*)m_pIB, m_pLockedData, 0, m_nIndexCount * m_nIndexStride);
#   endif
        m_pLockedData = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
HRESULT FencedIB<IndexType>::Bind(uint32 nOffs)
{
    COMPILE_TIME_ASSERT(sizeof(IndexType) == 2 || sizeof(IndexType) == 4);
    return gcpRendD3D->FX_SetIStream(m_pIB, nOffs, (sizeof(IndexType) == 2 ? Index16 : Index32));
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
uint32 FencedIB<IndexType>::GetIndexCount() const
{
    return m_nIndexCount;
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
void FencedIB<IndexType>::SetFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
    gRenDev->m_DevMan.IssueFence(m_Fence);
#endif
}

///////////////////////////////////////////////////////////////////////////////
template <class IndexType>
void FencedIB<IndexType>::WaitForFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
    gRenDev->m_DevMan.SyncFence(m_Fence, true, false);
#endif
}
