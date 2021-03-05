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
template <class VertexType>
class FencedVB
{
public:
    FencedVB(uint32 nVertexCount, uint32 nVertStride);
    ~FencedVB();

    VertexType*     LockVB(uint32 nLockCount);
    void                    UnlockVB();
    HRESULT             Bind(uint32 StreamNumber = 0, int nBytesOffset = 0, int nStride = 0);

    uint32 GetVertexCount() const;

    void SetFence();
    void WaitForFence();

private:
    D3DBuffer*    m_pVB;
    uint32                      m_nVertexCount;

    VertexType*             m_pLockedData;
    uint32                      m_nVertStride;
    DeviceFenceHandle   m_Fence;
};

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
FencedVB<VertexType>::FencedVB(uint32 nVertexCount, uint32 nVertStride)
    : m_pVB(NULL)
    , m_pLockedData(NULL)
    , m_nVertStride(nVertStride)
    , m_nVertexCount(nVertexCount)
{
    HRESULT hr = gRenDev->m_DevMan.CreateDirectAccessBuffer(m_nVertexCount, m_nVertStride, CDeviceManager::BIND_VERTEX_BUFFER, (D3DBuffer**)&m_pVB);
    CHECK_HRESULT(hr);

    gRenDev->m_DevMan.CreateFence(m_Fence);
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
FencedVB<VertexType>::~FencedVB()
{
    UnlockVB();
    if (m_pVB)
    {
        gRenDev->m_DevMan.DestroyDirectAccessBuffer((D3DBuffer*)m_pVB);
    }

    gRenDev->m_DevMan.ReleaseFence(m_Fence);
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
VertexType* FencedVB<VertexType>::LockVB([[maybe_unused]] uint32 nLockCount)
{
    // Ensure there is enough space in the VB for this data
    assert (nLockCount <= m_nVertexCount);

    if (m_pLockedData)
    {
        return m_pLockedData;
    }

    if (m_pVB)
    {
        gRenDev->m_DevMan.LockDirectAccessBuffer((D3DBuffer*)m_pVB, CDeviceManager::BIND_VERTEX_BUFFER, (void**)&m_pLockedData);
    }

    return m_pLockedData;
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
void FencedVB<VertexType>::UnlockVB()
{
    if (m_pLockedData && m_pVB)
    {
        gRenDev->m_DevMan.UnlockDirectAccessBuffer((D3DBuffer*)m_pVB, CDeviceManager::BIND_VERTEX_BUFFER);
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(FencedVB_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#   else
        CDeviceManager::InvalidateCpuCache(m_pLockedData, 0, m_nVertexCount * m_nVertStride);
        CDeviceManager::InvalidateGpuCache((D3DBuffer*)m_pVB, m_pLockedData, 0, m_nVertexCount * m_nVertStride);
#   endif
        m_pLockedData = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
HRESULT FencedVB<VertexType>::Bind(uint32 StreamNumber, int nBytesOffset, int nStride)
{
    HRESULT h = gcpRendD3D->FX_SetVStream(StreamNumber, m_pVB, nBytesOffset, nStride == 0 ? m_nVertStride : nStride);
    CHECK_HRESULT(h);
    return h;
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
uint32 FencedVB<VertexType>::GetVertexCount() const
{
    return m_nVertexCount;
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
void FencedVB<VertexType>::SetFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
    gRenDev->m_DevMan.IssueFence(m_Fence);
#endif
}

///////////////////////////////////////////////////////////////////////////////
template <class VertexType>
void FencedVB<VertexType>::WaitForFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
    gRenDev->m_DevMan.SyncFence(m_Fence, true, false);
#endif
}
