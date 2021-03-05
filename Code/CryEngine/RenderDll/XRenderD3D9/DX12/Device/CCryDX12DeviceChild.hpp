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
#ifndef __CCRYDX12DEVICECHILD__
#define __CCRYDX12DEVICECHILD__

#include "DX12/CCryDX12Object.hpp"

DEFINE_GUID(WKPDID_D3DDebugObjectName, 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00);
DEFINE_GUID(WKPDID_D3DDebugObjectNameW, 0x4cca5fd8, 0x921f, 0x42c8, 0x85, 0x66, 0x70, 0xca, 0xf2, 0xa9, 0xb7, 0x41);
DEFINE_GUID(WKPDID_D3DDebugClearValue, 0x01234567, 0x0123, 0x0123, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01);

// Specialize std::hash
namespace std {
    template<>
    struct hash<GUID>
    {
        size_t operator()(const GUID& guid) const /*noexcept*/
        {
            //  RPC_STATUS status;
            //  return UuidHash((UUID*)&guid, &status);

            const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
            std::hash<uint64_t> hash;
            return hash(p[0]) ^ hash(p[1]);
        }
    };
}

template <typename T>
class CCryDX12DeviceChild
    : public CCryDX12Object<T>
{
public:
    DX12_OBJECT(CCryDX12DeviceChild, CCryDX12Object<T>);

    virtual ~CCryDX12DeviceChild()
    {
    }

#if !defined(RELEASE)
    std::string GetName()
    {
        UINT len = 512;
        char str[512] = "-";

        GetPrivateData(WKPDID_D3DDebugObjectName, &len, str);

        return str;
    }

    HRESULT STDMETHODCALLTYPE GetPrivateCustomData(
        _In_  REFGUID guid,
        _Inout_  UINT* pDataSize,
        _Out_writes_bytes_opt_(*pDataSize)  void* pData)
    {
        TDatas::iterator elm = m_Data.find(guid);
        if (elm != m_Data.end())
        {
            void* Blob = (*elm).second.second;
            UINT nCopied = std::min(*pDataSize, (*elm).second.first);

            *pDataSize = nCopied;
            if (pData)
            {
                memcpy(pData, Blob, nCopied);
            }

            return S_OK;
        }

        return E_FAIL;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateCustomData(
        _In_  REFGUID guid,
        _In_  UINT DataSize,
        _In_reads_bytes_opt_(DataSize)  const void* pData)
    {
        TDatas::iterator elm = m_Data.find(guid);
        if (elm != m_Data.end())
        {
            CryModuleFree((*elm).second.second);
        }

        if (pData)
        {
            void* Blob = CryModuleMalloc(DataSize);
            m_Data[guid] = std::make_pair(DataSize, Blob);
            memcpy(Blob, pData, DataSize);
        }
        else
        {
            m_Data.erase(elm);
        }

        return S_OK;
    }
#endif

    #pragma region /* ID3D11DeviceChild implementation */

    virtual void STDMETHODCALLTYPE GetDevice(
        _Out_  ID3D11Device** ppDevice)
    {
        *ppDevice = m_Device;
    }

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        [[maybe_unused]] _In_  REFGUID guid,
        [[maybe_unused]] _Inout_  UINT* pDataSize,
        [[maybe_unused]] _Out_writes_bytes_opt_(*pDataSize)  void* pData)
    {
#if !defined(RELEASE)
        if (m_pChild)
        {
            return m_pChild->GetPrivateData(guid, pDataSize, pData);
        }

        return GetPrivateCustomData(guid, pDataSize, pData);
#else
        return E_FAIL;
#endif
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        [[maybe_unused]] _In_  REFGUID guid,
        [[maybe_unused]] _In_  UINT DataSize,
        [[maybe_unused]] _In_reads_bytes_opt_(DataSize)  const void* pData)
    {
#if !defined(RELEASE)
        if (m_pChild)
        {
            return m_pChild->SetPrivateData(guid, DataSize, pData);
        }

        return SetPrivateCustomData(guid, DataSize, pData);
#else
        return E_FAIL;
#endif
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        [[maybe_unused]] _In_  REFGUID guid,
        [[maybe_unused]] _In_opt_  const IUnknown* pData)
    {
#if !defined(RELEASE)
        if (m_pChild)
        {
            return m_pChild->SetPrivateDataInterface(guid, pData);
        }
#endif

        return E_FAIL;
    }

    #pragma endregion

    CCryDX12Device* GetDevice()
    {
        return m_Device;
    }

protected:
    CCryDX12DeviceChild(CCryDX12Device* pDevice, [[maybe_unused]] ID3D12DeviceChild* pChild)
    {
        m_Device = pDevice;
#if !defined(RELEASE)
        m_pChild = pChild;
#endif
    }

private:
    CCryDX12Device* m_Device;
#if !defined(RELEASE)
    typedef std::unordered_map<GUID, std::pair<UINT, void*> > TDatas;
    TDatas m_Data;
    ID3D12DeviceChild* m_pChild;
#endif
};

#endif
