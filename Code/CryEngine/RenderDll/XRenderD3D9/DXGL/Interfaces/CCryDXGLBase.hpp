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

// Description: Declaration of the reference counted base class for all
//              DXGL interface implementations

#ifndef __CRYDXGLBASE__
#define __CRYDXGLBASE__

#include "../Definitions/CryDXGLGuid.hpp"
#include "../Definitions/ICryDXGLUnknown.hpp"

template <typename Interface>
struct SingleInterface
{
    template <typename Object>
    static bool Query(Object* pThis, REFIID riid, void** ppvObject)
    {
        if (riid == __uuidof(Interface))
        {
            *reinterpret_cast<Interface**>(ppvObject) = static_cast<Interface*>(pThis);
            static_cast<Interface*>(pThis)->AddRef();
            return true;
        }
        return false;
    }
};

#include "DXEmulation.hpp"

////////////////////////////////////////////////////////////////////////////
// Definition of basic types
////////////////////////////////////////////////////////////////////////////

class CCryDXGLBase
#if !DXGL_FULL_EMULATION
    : public IUnknown
#endif //!DXGL_FULL_EMULATION
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLBase, Unknown)

    CCryDXGLBase();
    virtual ~CCryDXGLBase();

#if DXGL_FULL_EMULATION
    SAggregateNode& GetAggregateHead();
#else
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
#endif //!DXGL_FULL_EMULATION
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
protected:
    uint32 m_uRefCount;
#if DXGL_FULL_EMULATION
    SAggregateNode m_kAggregateHead;
#endif //DXGL_FULL_EMULATION
};

class CCryDXGLPrivateDataContainer
{
public:
    CCryDXGLPrivateDataContainer();
    ~CCryDXGLPrivateDataContainer();

    HRESULT GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData);
    HRESULT SetPrivateData(REFGUID guid, UINT DataSize, const void* pData);
    HRESULT SetPrivateDataInterface(REFGUID guid, const IUnknown* pData);
protected:
    void RemovePrivateData(REFGUID guid);
protected:
    struct SPrivateData;

    struct SGuidHashCompare
    {
        size_t operator()(const GUID& kGuid) const;
        bool operator()(const GUID& kLeft, const GUID& kRight) const;
    };

    typedef AZStd::unordered_map<GUID, SPrivateData*, SGuidHashCompare, SGuidHashCompare> TPrivateDataMap;
    TPrivateDataMap m_kPrivateDataMap;
};

#endif //__CRYDXGLBASE__
