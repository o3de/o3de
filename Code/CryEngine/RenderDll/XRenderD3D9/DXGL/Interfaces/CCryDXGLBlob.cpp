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

// Description : Definition of the DXGL wrapper for ID3D10Blob


#include "RenderDll_precompiled.h"
#include "CCryDXGLBlob.hpp"

CCryDXGLBlob::CCryDXGLBlob(size_t uBufferSize)
    : m_uBufferSize(uBufferSize)
#if defined(DXGL_BLOB_INTEROPERABILITY)
    , m_uRefCount(1)
#endif //defined(DXGL_BLOB_INTEROPERABILITY)
{
    DXGL_INITIALIZE_INTERFACE(D3D10Blob)
    m_pBuffer = new uint8[m_uBufferSize];
}

CCryDXGLBlob::~CCryDXGLBlob()
{
    delete [] m_pBuffer;
}


#if defined(DXGL_BLOB_INTEROPERABILITY)

////////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLBlob::QueryInterface(REFIID riid, void** ppvObject)
{
    return E_NOINTERFACE;
}

ULONG CCryDXGLBlob::AddRef(void)
{
    return ++m_uRefCount;
}

ULONG CCryDXGLBlob::Release(void)
{
    --m_uRefCount;
    if (m_uRefCount == 0)
    {
        delete this;
        return 0;
    }
    return m_uRefCount;
}

#endif //defined(DXGL_BLOB_INTEROPERABILITY)


////////////////////////////////////////////////////////////////////////////////
// ID3D10Blob implementation
////////////////////////////////////////////////////////////////////////////////

LPVOID CCryDXGLBlob::GetBufferPointer()
{
    return m_pBuffer;
}

SIZE_T CCryDXGLBlob::GetBufferSize()
{
    return m_uBufferSize;
}
