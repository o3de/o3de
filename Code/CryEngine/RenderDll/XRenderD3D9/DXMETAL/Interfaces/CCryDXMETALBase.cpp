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

//  Description: Definition of the reference counted base class for all
//               DXGL interface implementations

#include "RenderDll_precompiled.h"
#include "CCryDXMETALBase.hpp"
#include "../Implementation/GLCommon.hpp"

CCryDXGLBase::CCryDXGLBase()
    : m_uRefCount(1)
{
    DXGL_INITIALIZE_INTERFACE(Unknown);
}

CCryDXGLBase::~CCryDXGLBase()
{
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of IUnknown
////////////////////////////////////////////////////////////////////////////////

#if DXGL_FULL_EMULATION

SAggregateNode& CCryDXGLBase::GetAggregateHead()
{
    return m_kAggregateHead;
}

#else

HRESULT CCryDXGLBase::QueryInterface(REFIID riid, void** ppvObject)
{
    return E_NOINTERFACE;
}

#endif

ULONG CCryDXGLBase::AddRef(void)
{
    return ++m_uRefCount;
}

ULONG CCryDXGLBase::Release(void)
{
    --m_uRefCount;
    if (m_uRefCount == 0)
    {
        delete this;
        return 0;
    }
    return m_uRefCount;
}


////////////////////////////////////////////////////////////////////////////////
// CCryDXGLPrivateDataContainer
////////////////////////////////////////////////////////////////////////////////

struct CCryDXGLPrivateDataContainer::SPrivateData
{
    union
    {
        uint8* m_pBuffer;
        IUnknown* m_pInterface;
    };
    uint32 m_uSize;
    bool m_bInterface;

    SPrivateData(const void* pData, uint32 uSize)
    {
        m_pBuffer = new uint8[uSize];
        m_uSize = uSize;
        m_bInterface = false;
        memcpy(m_pBuffer, pData, uSize);
    }

    SPrivateData(IUnknown* pInterface)
    {
        pInterface->AddRef();
        m_pInterface = pInterface;
        m_uSize = sizeof(IUnknown*);
        m_bInterface = true;
    }

    ~SPrivateData()
    {
        if (m_bInterface)
        {
            m_pInterface->Release();
        }
        else
        {
            delete [] m_pBuffer;
        }
    }
};

CCryDXGLPrivateDataContainer::CCryDXGLPrivateDataContainer()
{
}

CCryDXGLPrivateDataContainer::~CCryDXGLPrivateDataContainer()
{
    TPrivateDataMap::iterator kPrivateIter(m_kPrivateDataMap.begin());
    TPrivateDataMap::iterator kPrivateEnd(m_kPrivateDataMap.end());
    for (; kPrivateIter != kPrivateEnd; ++kPrivateIter)
    {
        delete kPrivateIter->second;
    }
}

HRESULT CCryDXGLPrivateDataContainer::GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
{
    if (pData == NULL)
    {
        if (*pDataSize != 0)
        {
            return E_FAIL;
        }
        RemovePrivateData(guid);
    }
    else
    {
        CRY_ASSERT(pDataSize != NULL);
        TPrivateDataMap::const_iterator kFound(m_kPrivateDataMap.find(guid));
        if (kFound == m_kPrivateDataMap.end() || *pDataSize < kFound->second->m_uSize)
        {
            return E_FAIL;
        }

        if (kFound->second->m_bInterface)
        {
            kFound->second->m_pInterface->AddRef();
            *static_cast<IUnknown**>(pData) = kFound->second->m_pInterface;
        }
        else
        {
            memcpy(pData, kFound->second->m_pBuffer, kFound->second->m_uSize);
        }
        *pDataSize = kFound->second->m_uSize;
    }

    return S_OK;
}

HRESULT CCryDXGLPrivateDataContainer::SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
{
    RemovePrivateData(guid);

    m_kPrivateDataMap.insert(TPrivateDataMap::value_type(guid, new SPrivateData(pData, DataSize)));
    return S_OK;
}

HRESULT CCryDXGLPrivateDataContainer::SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
{
    RemovePrivateData(guid);

    m_kPrivateDataMap.insert(TPrivateDataMap::value_type(guid, new SPrivateData(const_cast<IUnknown*>(pData))));    // The specification requires that IUnknown::AddRef, Release are called on pData thus the const cast
    return S_OK;
}

void CCryDXGLPrivateDataContainer::RemovePrivateData(REFGUID guid)
{
    TPrivateDataMap::iterator kFound(m_kPrivateDataMap.find(guid));
    if (kFound != m_kPrivateDataMap.end())
    {
        delete kFound->second;
        m_kPrivateDataMap.erase(kFound);
    }
}

size_t CCryDXGLPrivateDataContainer::SGuidHashCompare::operator()(const GUID& kGuid) const
{
    return (size_t)NCryMetal::GetCRC32(reinterpret_cast<const char*>(&kGuid), sizeof(kGuid), 0xFFFFFFFF);
}

bool CCryDXGLPrivateDataContainer::SGuidHashCompare::operator()(const GUID& kLeft, const GUID& kRight) const
{
    return memcmp(&kLeft, &kRight, sizeof(kLeft)) == 0;
}
