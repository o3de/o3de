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

#include "RenderDll_precompiled.h"

//////////////////////////////////////////////////////////////////////////////////////
buffer_handle_t CDeviceBufferManager::Create_Locked(BUFFER_BIND_TYPE, BUFFER_USAGE, size_t)
{
    return buffer_handle_t();
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::Destroy_Locked(buffer_handle_t)
{
}

//////////////////////////////////////////////////////////////////////////////////////
void* CDeviceBufferManager::BeginRead_Locked([[maybe_unused]] buffer_handle_t handle)
{
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////
void* CDeviceBufferManager::BeginWrite_Locked([[maybe_unused]] buffer_handle_t handle)
{
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::EndReadWrite_Locked([[maybe_unused]] buffer_handle_t handle)
{
}

//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::UpdateBuffer_Locked([[maybe_unused]] buffer_handle_t handle, const void*, size_t)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////
size_t CDeviceBufferManager::Size_Locked(buffer_handle_t)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
CDeviceBufferManager::CDeviceBufferManager()
{
}

//////////////////////////////////////////////////////////////////////////////////////
CDeviceBufferManager::~CDeviceBufferManager()
{
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::LockDevMan()
{
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::UnlockDevMan()
{
}

//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::Init()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::Shutdown()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::Sync([[maybe_unused]] uint32 framdid)
{
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::Update(uint32, [[maybe_unused]] bool called_during_loading)
{
}

//////////////////////////////////////////////////////////////////////////////////////
buffer_handle_t CDeviceBufferManager::Create(
    [[maybe_unused]] BUFFER_BIND_TYPE type
    , [[maybe_unused]] BUFFER_USAGE usage
    , [[maybe_unused]] size_t size)
{
    return ~0u;
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::Destroy([[maybe_unused]] buffer_handle_t handle)
{
}

//////////////////////////////////////////////////////////////////////////////////////
void* CDeviceBufferManager::BeginRead([[maybe_unused]] buffer_handle_t handle)
{
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
void* CDeviceBufferManager::BeginWrite([[maybe_unused]] buffer_handle_t handle)
{
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::EndReadWrite([[maybe_unused]] buffer_handle_t handle)
{
}

//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::UpdateBuffer([[maybe_unused]] buffer_handle_t handle, [[maybe_unused]] const void* src, [[maybe_unused]] size_t size)
{
    return true;
}

/////////////////////////////////////////////////////////////
// Legacy interface
//
// Use with care, can be removed at any point!
//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::ReleaseVBuffer(CVertexBuffer* pVB)
{
    SAFE_DELETE(pVB);
}
//////////////////////////////////////////////////////////////////////////////////////
void CDeviceBufferManager::ReleaseIBuffer(CIndexBuffer* pIB)
{
    SAFE_DELETE(pIB);
}
//////////////////////////////////////////////////////////////////////////////////////
CVertexBuffer* CDeviceBufferManager::CreateVBuffer([[maybe_unused]] size_t nVerts, const AZ::Vertex::Format& vertexFormat, [[maybe_unused]] const char* szName, [[maybe_unused]] BUFFER_USAGE usage)
{
    CVertexBuffer* pVB = new CVertexBuffer(NULL, vertexFormat);
    return pVB;
}
//////////////////////////////////////////////////////////////////////////////////////
CIndexBuffer* CDeviceBufferManager::CreateIBuffer([[maybe_unused]] size_t nInds, [[maybe_unused]] const char* szNam, [[maybe_unused]] BUFFER_USAGE usage)
{
    CIndexBuffer* pIB = new CIndexBuffer(NULL);
    return pIB;
}
//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::UpdateVBuffer([[maybe_unused]] CVertexBuffer* pVB, [[maybe_unused]] void* pVerts, [[maybe_unused]] size_t nVerts)
{
    return true;
}
//////////////////////////////////////////////////////////////////////////////////////
bool CDeviceBufferManager::UpdateIBuffer([[maybe_unused]] CIndexBuffer* pIB, [[maybe_unused]] void* pInds, [[maybe_unused]] size_t nInds)
{
    return true;
}
//////////////////////////////////////////////////////////////////////////////////////
CVertexBuffer::~CVertexBuffer()
{
}
//////////////////////////////////////////////////////////////////////////////////////
CIndexBuffer::~CIndexBuffer()
{
}

namespace AzRHI
{
    ConstantBuffer::~ConstantBuffer()
    {}

    void ConstantBuffer::AddRef()
    {}

    AZ::u32 ConstantBuffer::Release()
    {
        return 0;
    }
}
