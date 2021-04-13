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
#ifndef __CCRYDX12OBJECT__
#define __CCRYDX12OBJECT__

#include "API/DX12Base.hpp"

#define DX12_BASE_OBJECT(DerivedType, InterfaceType) \
    typedef DerivedType Class;                       \
    typedef DX12::SmartPtr<Class> Ptr;               \
    typedef DX12::SmartPtr<const Class> ConstPtr;    \
    typedef InterfaceType Super;                     \
    typedef InterfaceType Interface;                 \
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) \
    {                                                                               \
        if (riid == __uuidof(Super))                                                \
        {                                                                           \
            if (ppvObject)                                                          \
            {                                                                       \
                *reinterpret_cast<Super**>(ppvObject) = static_cast<Super*>(this);  \
                static_cast<Super*>(this)->AddRef();                                \
            }                                                                       \
           return S_OK;                                                             \
        }                                                                           \
        return E_NOINTERFACE;                                                       \
    }                                                                               \

#define DX12_OBJECT(DerivedType, SuperType)       \
    typedef DerivedType Class;                    \
    typedef DX12::SmartPtr<Class> Ptr;            \
    typedef DX12::SmartPtr<const Class> ConstPtr; \
    typedef SuperType Super;                      \
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) \
    {                                                                               \
        if (riid == __uuidof(DerivedType))                                          \
        {                                                                           \
            if (ppvObject)                                                          \
            {                                                                       \
                *reinterpret_cast<DerivedType**>(ppvObject) = static_cast<DerivedType*>(this);    \
                static_cast<DerivedType*>(this)->AddRef();                          \
            }                                                                       \
           return S_OK;                                                             \
        }                                                                           \
        return Super::QueryInterface(riid, ppvObject);                              \
    }                                                                               \

#include "CryDX12Guid.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CCryDX12Buffer;
class CCryDX12DepthStencilView;
class CCryDX12Device;
class CCryDX12DeviceContext;
class CCryDX12MemoryManager;
class CCryDX12Query;
class CCryDX12RenderTargetView;
class CCryDX12SamplerState;
class CCryDX12Shader;
class CCryDX12ShaderResourceView;
class CCryDX12SwapChain;
class CCryDX12Texture1D;
class CCryDX12Texture2D;
class CCryDX12Texture3D;
class CCryDX12UnorderedAccessView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class CCryDX12Object
    : public T
{
public:
    DX12_BASE_OBJECT(CCryDX12Object, T);

    CCryDX12Object()
        : m_RefCount(0)
    {
    }

    virtual ~CCryDX12Object()
    {
        DX12_LOG("CCryDX12 object destroyed: %p", this);
    }

    #pragma region /* IUnknown implementation */

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return CryInterlockedIncrement(&m_RefCount);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        ULONG RefCount = CryInterlockedDecrement(&m_RefCount);
        if (!RefCount)
        {
            delete this;
            return 0;
        }

        return RefCount;
    }

    #pragma endregion

private:
    int m_RefCount;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class CCryDX12GIObject
    : public T
{
public:
    DX12_BASE_OBJECT(CCryDX12GIObject, T);

    CCryDX12GIObject()
        : m_RefCount(0)
    {
    }

    virtual ~CCryDX12GIObject()
    {
    }

    #pragma region /* IUnknown implementation */

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return CryInterlockedIncrement(&m_RefCount);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        ULONG RefCount = CryInterlockedDecrement(&m_RefCount);
        if (!RefCount)
        {
            delete this;
            return 0;
        }

        return RefCount;
    }

    #pragma endregion

    #pragma region /* IDXGIObject implementation */

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        [[maybe_unused]] _In_  REFGUID Name,
        [[maybe_unused]] UINT DataSize,
        [[maybe_unused]] _In_reads_bytes_(DataSize) const void* pData)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        [[maybe_unused]] _In_  REFGUID Name,
        [[maybe_unused]] _In_  const IUnknown* pUnknown)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        [[maybe_unused]] _In_  REFGUID Name,
        [[maybe_unused]] _Inout_  UINT* pDataSize,
        [[maybe_unused]] _Out_writes_bytes_(*pDataSize) void* pData)
    {
        return -1;
    }

    virtual HRESULT STDMETHODCALLTYPE GetParent(
        [[maybe_unused]] _In_  REFIID riid,
        [[maybe_unused]] _Out_ void** ppParent)
    {
        return -1;
    }

    #pragma endregion

private:
    int m_RefCount;
};

#endif // __CCRYDX12OBJECT__
