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

// Description : Declaration of the DXGL wrapper for ID3D10Blob

#ifndef __CRYMETALGLBLOB__
#define __CRYMETALGLBLOB__

#include "CCryDXMETALBase.hpp"

#if defined(DXGL_BLOB_INTEROPERABILITY) && !DXGL_FULL_EMULATION
class CCryDXGLBlob
    : public ID3D10Blob
#else
class CCryDXGLBlob
    : public CCryDXGLBase
#endif
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLBlob, D3D10Blob)

    CCryDXGLBlob(size_t uBufferSize);
    virtual ~CCryDXGLBlob();

#if defined(DXGL_BLOB_INTEROPERABILITY)
    //IUnknown implementation
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
#endif //defined(DXGL_BLOB_INTEROPERABILITY)

    // ID3D10Blob implementation
    LPVOID STDMETHODCALLTYPE GetBufferPointer();
    SIZE_T STDMETHODCALLTYPE GetBufferSize();
protected:
#if defined(DXGL_BLOB_INTEROPERABILITY)
    uint32 m_uRefCount;
#endif //defined(DXGL_BLOB_INTEROPERABILITY)

    uint8 * m_pBuffer;
    size_t m_uBufferSize;
};


#endif //__CRYMETALGLBLOB__

