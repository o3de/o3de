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

// Description : Definition of the DXGL wrapper for ID3D11Device

#include "RenderDll_precompiled.h"
#include "CCryDXMETALBlendState.hpp"
#include "CCryDXMETALBuffer.hpp"
#include "CCryDXMETALDepthStencilState.hpp"
#include "CCryDXMETALDepthStencilView.hpp"
#include "CCryDXMETALDevice.hpp"
#include "CCryDXMETALDeviceContext.hpp"
#include "CCryDXMETALDevice.hpp"
#include "CCryDXMETALInputLayout.hpp"
#include "CCryDXMETALQuery.hpp"
#include "CCryDXMETALRasterizerState.hpp"
#include "CCryDXMETALRenderTargetView.hpp"
#include "CCryDXMETALSamplerState.hpp"
#include "CCryDXMETALSwapChain.hpp"
#include "CCryDXMETALShader.hpp"
#include "CCryDXMETALShaderResourceView.hpp"
#include "CCryDXMETALTexture1D.hpp"
#include "CCryDXMETALTexture2D.hpp"
#include "CCryDXMETALTexture3D.hpp"
#include "CCryDXMETALUnorderedAccessView.hpp"
#include "../Implementation/MetalDevice.hpp"
#include "../Implementation/GLFormat.hpp"
#include "../Implementation/GLResource.hpp"
#include "../Implementation/GLShader.hpp"

CCryDXGLDevice::CCryDXGLDevice(CCryDXGLGIAdapter* pAdapter, D3D_FEATURE_LEVEL eFeatureLevel)
    : m_spAdapter(pAdapter)
    , m_eFeatureLevel(eFeatureLevel)
{
    DXGL_INITIALIZE_INTERFACE(DXGIDevice)
    DXGL_INITIALIZE_INTERFACE(D3D11Device)

    CCryDXGLDeviceContext * pImmediateContext(new CCryDXGLDeviceContext());
    m_spImmediateContext = pImmediateContext;
    pImmediateContext->Release();
}

CCryDXGLDevice::~CCryDXGLDevice()
{
    m_spImmediateContext->Shutdown();
}

#if !DXGL_FULL_EMULATION

HRESULT CCryDXGLDevice::QueryInterface(REFIID riid, void** ppvObject)
{
    if (SingleInterface<ID3D11Device>::Query(this, riid, ppvObject) ||
        SingleInterface<CCryDXGLDevice>::Query(this, riid, ppvObject))
    {
        return S_OK;
    }
#if DXGL_VIRTUAL_DEVICE_AND_CONTEXT
    return E_NOINTERFACE;
#else
    return CCryDXGLBase::QueryInterface(riid, ppvObject);
#endif
}

#endif //!DXGL_FULL_EMULATION

bool CCryDXGLDevice::Initialize(const DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    if (!pDesc || !ppSwapChain || !m_spAdapter || !m_spAdapter->GetGLAdapter())
    {
        return false;
    }

    m_spGLDevice = new NCryMetal::CDevice();
    if (!m_spGLDevice->Initialize(pDesc->OutputWindow))
    {
        return false;
    }

    CCryDXGLSwapChain* pDXGLSwapChain(new CCryDXGLSwapChain(this, *pDesc));
    CCryDXGLSwapChain::ToInterface(ppSwapChain, pDXGLSwapChain);
    if (!pDXGLSwapChain->Initialize())
    {
        return false;
    }

    return m_spImmediateContext->Initialize(this);
}

NCryMetal::CDevice* CCryDXGLDevice::GetGLDevice()
{
    return m_spGLDevice;
}


////////////////////////////////////////////////////////////////////////////////
// IDXGIObject overrides
////////////////////////////////////////////////////////////////////////////////


HRESULT CCryDXGLDevice::GetParent(REFIID riid, void** ppParent)
{
    IUnknown* pAdapterInterface;
    CCryDXGLBase::ToInterface(&pAdapterInterface, m_spAdapter);
    if (pAdapterInterface->QueryInterface(riid, ppParent) == S_OK && ppParent != NULL)
    {
        return S_OK;
    }
#if DXGL_VIRTUAL_DEVICE_AND_CONTEXT && !DXGL_FULL_EMULATION
    return E_FAIL;
#else
    return CCryDXGLGIObject::GetParent(riid, ppParent);
#endif
}


////////////////////////////////////////////////////////////////////////////////
// IDXGIDevice implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLDevice::GetAdapter(IDXGIAdapter** pAdapter)
{
    if (m_spAdapter == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLGIAdapter::ToInterface(pAdapter, m_spAdapter);
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateSurface(const DXGI_SURFACE_DESC* pDesc, UINT NumSurfaces, DXGI_USAGE Usage, const DXGI_SHARED_RESOURCE* pSharedResource, IDXGISurface** ppSurface)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::QueryResourceResidency(IUnknown* const* ppResources, DXGI_RESIDENCY* pResidencyStatus, UINT NumResources)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::SetGPUThreadPriority(INT Priority)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::GetGPUThreadPriority(INT* pPriority)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////////
// ID3D11Device implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLDevice::CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer)
{
    if (ppBuffer == NULL)
    {
        // In this case the method should perform parameter validation and return the result
        DXGL_NOT_IMPLEMENTED
        return E_FAIL;
    }

    NCryMetal::SBufferPtr spGLBuffer(NCryMetal::CreateBuffer(*pDesc, pInitialData, m_spGLDevice));
    if (spGLBuffer == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLBuffer::ToInterface(ppBuffer, new CCryDXGLBuffer(*pDesc, spGLBuffer, this));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateTexture1D(const D3D11_TEXTURE1D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture1D** ppTexture1D)
{
    if (ppTexture1D == NULL)
    {
        // In this case the method should perform parameter validation and return the result
        DXGL_NOT_IMPLEMENTED
        return E_FAIL;
    }

    NCryMetal::STexturePtr spGLTexture(NCryMetal::CreateTexture1D(*pDesc, pInitialData, m_spGLDevice));
    if (spGLTexture == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLTexture1D::ToInterface(ppTexture1D, new CCryDXGLTexture1D(*pDesc, spGLTexture, this));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
    if (ppTexture2D == NULL)
    {
        // In this case the method should perform parameter validation and return the result
        DXGL_NOT_IMPLEMENTED
        return E_FAIL;
    }

    NCryMetal::STexturePtr spGLTexture(NCryMetal::CreateTexture2D(*pDesc, pInitialData, m_spGLDevice));
    if (spGLTexture == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLTexture2D::ToInterface(ppTexture2D, new CCryDXGLTexture2D(*pDesc, spGLTexture, this));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateTexture3D(const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D)
{
    if (ppTexture3D == NULL)
    {
        // In this case the method should perform parameter validation and return the result
        DXGL_NOT_IMPLEMENTED
        return E_FAIL;
    }

    NCryMetal::STexturePtr spGLTexture(NCryMetal::CreateTexture3D(*pDesc, pInitialData, m_spGLDevice));
    if (spGLTexture == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLTexture3D::ToInterface(ppTexture3D, new CCryDXGLTexture3D(*pDesc, spGLTexture, this));
    return S_OK;
}

bool GetStandardViewDesc(CCryDXGLTexture1D* pTexture, D3D11_SHADER_RESOURCE_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE1D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    if (kTextureDesc.ArraySize > 0)
    {
        kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
        kStandardDesc.Texture1DArray.MostDetailedMip = 0;
        kStandardDesc.Texture1DArray.MipLevels = -1;
        kStandardDesc.Texture1DArray.FirstArraySlice = 0;
        kStandardDesc.Texture1DArray.ArraySize = kTextureDesc.ArraySize;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
        kStandardDesc.Texture1D.MostDetailedMip = 0;
        kStandardDesc.Texture1DArray.MipLevels = -1;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture2D* pTexture, D3D11_SHADER_RESOURCE_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE2D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    if (kTextureDesc.ArraySize > 1)
    {
        if (kTextureDesc.SampleDesc.Count > 1)
        {
            kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            kStandardDesc.Texture2DMSArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DMSArray.ArraySize = kTextureDesc.ArraySize;
        }
        else
        {
            kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            kStandardDesc.Texture2DArray.MostDetailedMip = 0;
            kStandardDesc.Texture2DArray.MipLevels = -1;
            kStandardDesc.Texture2DArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DArray.ArraySize = kTextureDesc.ArraySize;
        }
    }
    else if (kTextureDesc.SampleDesc.Count > 1)
    {
        kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        kStandardDesc.Texture2D.MostDetailedMip = 0;
        kStandardDesc.Texture2D.MipLevels = -1;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture3D* pTexture, D3D11_SHADER_RESOURCE_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE3D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    kStandardDesc.Texture3D.MostDetailedMip = 0;
    kStandardDesc.Texture3D.MipLevels = -1;
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLBuffer* pBuffer, D3D11_SHADER_RESOURCE_VIEW_DESC& kStandardDesc)
{
    D3D11_BUFFER_DESC kBufferDesc;
    pBuffer->GetDesc(&kBufferDesc);

    bool bSuccess((kBufferDesc.MiscFlags | D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) != 0);
    if (bSuccess)
    {
        kStandardDesc.Format = DXGI_FORMAT_UNKNOWN;
        kStandardDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        kStandardDesc.Buffer.FirstElement = 0;
        kStandardDesc.Buffer.NumElements = kBufferDesc.StructureByteStride;
    }
    else
    {
        DXGL_ERROR("Default shader resource view for a buffer requires element size specification");
    }

    pBuffer->Release();
    return bSuccess;
}

bool GetStandardViewDesc(CCryDXGLTexture1D* pTexture, D3D11_RENDER_TARGET_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE1D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    if (kTextureDesc.ArraySize > 0)
    {
        kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
        kStandardDesc.Texture1DArray.MipSlice = 0;
        kStandardDesc.Texture1DArray.FirstArraySlice = 0;
        kStandardDesc.Texture1DArray.ArraySize = kTextureDesc.ArraySize;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
        kStandardDesc.Texture1D.MipSlice = 0;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture2D* pTexture, D3D11_RENDER_TARGET_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE2D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    if (kTextureDesc.ArraySize > 1)
    {
        if (kTextureDesc.SampleDesc.Count > 1)
        {
            kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
            kStandardDesc.Texture2DMSArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DMSArray.ArraySize = kTextureDesc.ArraySize;
        }
        else
        {
            kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            kStandardDesc.Texture2DArray.MipSlice = 0;
            kStandardDesc.Texture2DArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DArray.ArraySize = kTextureDesc.ArraySize;
        }
    }
    else if (kTextureDesc.SampleDesc.Count > 1)
    {
        kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        kStandardDesc.Texture2D.MipSlice = 0;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture3D* pTexture, D3D11_RENDER_TARGET_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE3D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
    kStandardDesc.Texture3D.MipSlice = 0;
    kStandardDesc.Texture3D.FirstWSlice = 0;
    kStandardDesc.Texture3D.WSize = -1;
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLBuffer* pBuffer, D3D11_RENDER_TARGET_VIEW_DESC& kStandardDesc)
{
    D3D11_BUFFER_DESC kBufferDesc;
    pBuffer->GetDesc(&kBufferDesc);

    bool bSuccess((kBufferDesc.MiscFlags | D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) != 0);
    if (bSuccess)
    {
        kStandardDesc.Format = DXGI_FORMAT_UNKNOWN;
        kStandardDesc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
        kStandardDesc.Buffer.FirstElement = 0;
        kStandardDesc.Buffer.NumElements = kBufferDesc.StructureByteStride;
    }
    else
    {
        DXGL_ERROR("Default render target view for a buffer requires element size specification");
    }

    pBuffer->Release();
    return bSuccess;
}

bool GetStandardViewDesc(CCryDXGLTexture1D* pTexture, D3D11_DEPTH_STENCIL_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE1D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    kStandardDesc.Flags = 0;
    if (kTextureDesc.ArraySize > 0)
    {
        kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
        kStandardDesc.Texture1DArray.MipSlice = 0;
        kStandardDesc.Texture1DArray.FirstArraySlice = 0;
        kStandardDesc.Texture1DArray.ArraySize = kTextureDesc.ArraySize;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
        kStandardDesc.Texture1D.MipSlice = 0;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture2D* pTexture, D3D11_DEPTH_STENCIL_VIEW_DESC& kStandardDesc)
{
    D3D11_TEXTURE2D_DESC kTextureDesc;
    pTexture->GetDesc(&kTextureDesc);

    kStandardDesc.Format = kTextureDesc.Format;
    kStandardDesc.Flags = 0;
    if (kTextureDesc.ArraySize > 0)
    {
        if (kTextureDesc.SampleDesc.Count > 1)
        {
            kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
            kStandardDesc.Texture2DMSArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DMSArray.ArraySize = kTextureDesc.ArraySize;
        }
        else
        {
            kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            kStandardDesc.Texture2DArray.MipSlice = 0;
            kStandardDesc.Texture2DArray.FirstArraySlice = 0;
            kStandardDesc.Texture2DArray.ArraySize = kTextureDesc.ArraySize;
        }
    }
    else if (kTextureDesc.SampleDesc.Count > 1)
    {
        kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        kStandardDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        kStandardDesc.Texture2D.MipSlice = 0;
    }
    pTexture->Release();
    return true;
}

bool GetStandardViewDesc(CCryDXGLTexture3D* pTexture, D3D11_DEPTH_STENCIL_VIEW_DESC&)
{
    DXGL_ERROR("Cannot bind a depth stencil view to a 3D texture");
    pTexture->Release();
    return false;
}

bool GetStandardViewDesc(CCryDXGLBuffer* pBuffer, D3D11_DEPTH_STENCIL_VIEW_DESC&)
{
    DXGL_ERROR("Cannot bind a depth stencil view to a buffer");
    pBuffer->Release();
    return false;
}

template <typename ViewDesc>
bool GetStandardViewDesc(ID3D11Resource* pResource, ViewDesc& kStandardDesc)
{
    memset(&kStandardDesc, 0, sizeof(kStandardDesc));

    void* pvData(NULL);
    if (!FAILED(pResource->QueryInterface(__uuidof(ID3D11Texture1D), &pvData)) && pvData != NULL)
    {
        return GetStandardViewDesc(CCryDXGLTexture1D::FromInterface(reinterpret_cast<ID3D11Texture1D*>(pvData)), kStandardDesc);
    }
    if (!FAILED(pResource->QueryInterface(__uuidof(ID3D11Texture2D), &pvData)) && pvData != NULL)
    {
        return GetStandardViewDesc(CCryDXGLTexture2D::FromInterface(reinterpret_cast<ID3D11Texture2D*>(pvData)), kStandardDesc);
    }
    if (!FAILED(pResource->QueryInterface(__uuidof(ID3D11Texture3D), &pvData)) && pvData != NULL)
    {
        return GetStandardViewDesc(CCryDXGLTexture3D::FromInterface(reinterpret_cast<ID3D11Texture3D*>(pvData)), kStandardDesc);
    }
    if (!FAILED(pResource->QueryInterface(__uuidof(ID3D11Buffer), &pvData)) && pvData != NULL)
    {
        return GetStandardViewDesc(CCryDXGLBuffer::FromInterface(reinterpret_cast<ID3D11Buffer*>(pvData)), kStandardDesc);
    }

    DXGL_ERROR("Unknown resource type for standard view description");
    return false;
}

HRESULT CCryDXGLDevice::CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC kStandardDesc;
    if (pDesc == NULL)
    {
        if (!GetStandardViewDesc(pResource, kStandardDesc))
        {
            return E_INVALIDARG;
        }
        pDesc = &kStandardDesc;
    }
    CRY_ASSERT(pDesc != NULL);

    CCryDXGLShaderResourceView* pSRView(new CCryDXGLShaderResourceView(CCryDXGLResource::FromInterface(pResource), *pDesc, this));

    if (pSRView->Initialize(m_spGLDevice))
    {
        CCryDXGLShaderResourceView::ToInterface(ppSRView, pSRView);
        return S_OK;
    }

    pSRView->Release();
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateUnorderedAccessView(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppUAView)
{
    CCryDXGLUnorderedAccessView::ToInterface(ppUAView, new CCryDXGLUnorderedAccessView(CCryDXGLResource::FromInterface(pResource), *pDesc, this));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateRenderTargetView(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
{
    D3D11_RENDER_TARGET_VIEW_DESC kStandardDesc;
    if (pDesc == NULL)
    {
        if (!GetStandardViewDesc(pResource, kStandardDesc))
        {
            return E_INVALIDARG;
        }
        pDesc = &kStandardDesc;
    }
    CRY_ASSERT(pDesc != NULL);

    CCryDXGLRenderTargetView* pRTView(new CCryDXGLRenderTargetView(CCryDXGLResource::FromInterface(pResource), *pDesc, this));

    if (pRTView->Initialize(m_spGLDevice))
    {
        CCryDXGLRenderTargetView::ToInterface(ppRTView, pRTView);
        return S_OK;
    }

    pRTView->Release();
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateDepthStencilView(ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDepthStencilView)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC kStandardDesc;
    if (pDesc == NULL)
    {
        if (!GetStandardViewDesc(pResource, kStandardDesc))
        {
            return E_INVALIDARG;
        }
        pDesc = &kStandardDesc;
    }
    CRY_ASSERT(pDesc != NULL);

    CCryDXGLDepthStencilView* pDSView(new CCryDXGLDepthStencilView(CCryDXGLResource::FromInterface(pResource), *pDesc, this));

    if (pDSView->Initialize(m_spGLDevice))
    {
        CCryDXGLDepthStencilView::ToInterface(ppDepthStencilView, pDSView);
        return S_OK;
    }

    pDSView->Release();
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout** ppInputLayout)
{
    NCryMetal::TShaderReflection kShaderReflection;
    if (!InitializeShaderReflection(&kShaderReflection, pShaderBytecodeWithInputSignature))
    {
        return 0;
    }

    _smart_ptr<NCryMetal::SInputLayout> spGLInputLayout(NCryMetal::CreateInputLayout(pInputElementDescs, NumElements, kShaderReflection));

    if (spGLInputLayout == NULL)
    {
        return E_FAIL;
    }

    CCryDXGLInputLayout::ToInterface(ppInputLayout, new CCryDXGLInputLayout(spGLInputLayout, this));

    return S_OK;
}

_smart_ptr<NCryMetal::SShader> CreateGLShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, NCryMetal::EShaderType eType, NCryMetal::CDevice* pDevice)
{
    if (pClassLinkage != NULL)
    {
        DXGL_ERROR("Class linkage not supported");
        return NULL;
    }

    _smart_ptr<NCryMetal::SShader> spGLShader(new NCryMetal::SShader());
    spGLShader->m_eType = eType;
    if (!NCryMetal::InitializeShader(spGLShader, pShaderBytecode, BytecodeLength, pDevice->GetMetalDevice()))
    {
        return NULL;
    }
    return spGLShader;
}

template <typename DXGLShader, typename D3DShader>
HRESULT CreateShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, D3DShader** ppShader, NCryMetal::EShaderType eType, CCryDXGLDevice* pDevice)
{
    _smart_ptr<NCryMetal::SShader> spGLShader(CreateGLShader(pShaderBytecode, BytecodeLength, pClassLinkage, eType, pDevice->GetGLDevice()));
    if (spGLShader == NULL)
    {
        return E_FAIL;
    }

    DXGLShader::ToInterface(ppShader, new DXGLShader(spGLShader, pDevice));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateVertexShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader)
{
    return CreateShader<CCryDXGLVertexShader, ID3D11VertexShader>(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader, NCryMetal::eST_Vertex, this);
}

HRESULT CCryDXGLDevice::CreateGeometryShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
{
    DXGL_ERROR("Geometry shaders are not supported by this GL implementation.");
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateGeometryShaderWithStreamOutput(const void* pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, UINT NumEntries, const UINT* pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreatePixelShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader)
{
    return CreateShader<CCryDXGLPixelShader, ID3D11PixelShader>(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader, NCryMetal::eST_Fragment, this);
}

HRESULT CCryDXGLDevice::CreateHullShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11HullShader** ppHullShader)
{
    DXGL_ERROR("Hull shaders are not supported by this GL implementation.");
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateDomainShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11DomainShader** ppDomainShader)
{
    DXGL_ERROR("Domain shaders are not supported by this GL implementation.");
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateComputeShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11ComputeShader** ppComputeShader)
{
#if DXGL_SUPPORT_COMPUTE
    return CreateShader<CCryDXGLComputeShader, ID3D11ComputeShader>(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader, NCryMetal::eST_Compute, this);
#else
    DXGL_ERROR("Compute shaders are not supported by this GL implementation.");
    return E_FAIL;
#endif
}

HRESULT CCryDXGLDevice::CreateClassLinkage(ID3D11ClassLinkage** ppLinkage)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc, ID3D11BlendState** ppBlendState)
{
    CCryDXGLBlendState* pState(new CCryDXGLBlendState(*pBlendStateDesc, this));

    if (!pState->Initialize(this))
    {
        pState->Release();
        return E_FAIL;
    }

    CCryDXGLBlendState::ToInterface(ppBlendState, pState);
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState)
{
    CCryDXGLDepthStencilState* pState(new CCryDXGLDepthStencilState(*pDepthStencilDesc, this));

    if (!pState->Initialize(this))
    {
        pState->Release();
        return E_FAIL;
    }

    CCryDXGLDepthStencilState::ToInterface(ppDepthStencilState, pState);
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState)
{
    CCryDXGLRasterizerState* pState(new CCryDXGLRasterizerState(*pRasterizerDesc, this));

    if (!pState->Initialize(this))
    {
        pState->Release();
        return E_FAIL;
    }

    CCryDXGLRasterizerState::ToInterface(ppRasterizerState, pState);
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState)
{
    CCryDXGLSamplerState* pState(new CCryDXGLSamplerState(*pSamplerDesc, this));

    if (!pState->Initialize(this))
    {
        pState->Release();
        return E_FAIL;
    }

    CCryDXGLSamplerState::ToInterface(ppSamplerState, pState);
    return S_OK;
}

HRESULT CCryDXGLDevice::CreateQuery(const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery)
{
    NCryMetal::SQueryPtr spGLQuery(NCryMetal::CreateQuery(*pQueryDesc, m_spGLDevice));
    if (spGLQuery == NULL)
    {
        return E_FAIL;
    }
    CCryDXGLQuery::ToInterface(ppQuery, new CCryDXGLQuery(*pQueryDesc, spGLQuery, this));
    return S_OK;
}

HRESULT CCryDXGLDevice::CreatePredicate(const D3D11_QUERY_DESC* pPredicateDesc, ID3D11Predicate** ppPredicate)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateCounter(const D3D11_COUNTER_DESC* pCounterDesc, ID3D11Counter** ppCounter)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CreateDeferredContext(UINT ContextFlags, ID3D11DeviceContext** ppDeferredContext)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::OpenSharedResource(HANDLE hResource, REFIID ReturnedInterface, void** ppResource)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CheckFormatSupport(DXGI_FORMAT Format, UINT* pFormatSupport)
{
    NCryMetal::EGIFormat eGIFormat(NCryMetal::GetGIFormat(Format));
    if (eGIFormat == NCryMetal::eGIF_NUM)
    {
        DXGL_ERROR("Unknown DXGI format");
        return E_FAIL;
    }

    (*pFormatSupport) = m_spAdapter->GetGLAdapter()->m_giFormatSupport[eGIFormat];
    return S_OK;
}

HRESULT CCryDXGLDevice::CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT* pNumQualityLevels)
{
    NCryMetal::EGIFormat eGIFormat(NCryMetal::GetGIFormat(Format));
    if (eGIFormat != NCryMetal::eGIF_NUM && (SampleCount <= m_spAdapter->GetGLAdapter()->m_maxSamples))
    {
        *pNumQualityLevels = 1;
    }
    else
    {
        *pNumQualityLevels = 0;
    }
    DXGL_TODO("Check if there's a way to query for specific quality levels");
    return S_OK;
}

void CCryDXGLDevice::CheckCounterInfo(D3D11_COUNTER_INFO* pCounterInfo)
{
    DXGL_NOT_IMPLEMENTED
}

HRESULT CCryDXGLDevice::CheckCounter(const D3D11_COUNTER_DESC* pDesc, D3D11_COUNTER_TYPE* pType, UINT* pActiveCounters, LPSTR szName, UINT* pNameLength, LPSTR szUnits, UINT* pUnitsLength, LPSTR szDescription, UINT* pDescriptionLength)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLDevice::CheckFeatureSupport(D3D11_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize)
{
    switch (Feature)
    {
    case D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:
    {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS* pData(static_cast<D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS*>(pFeatureSupportData));
        bool bComputeShaderSupported(m_spAdapter->GetGLAdapter()->m_features.Get(NCryMetal::eF_ComputeShader));
        pData->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = bComputeShaderSupported ? TRUE : FALSE;
        return S_OK;
    }
    break;
    default:
        DXGL_TODO("Add supported 11.1 features")
        return E_FAIL;
    }
}

HRESULT CCryDXGLDevice::GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
{
    return m_kPrivateDataContainer.GetPrivateData(guid, pDataSize, pData);
}

HRESULT CCryDXGLDevice::SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
{
    return m_kPrivateDataContainer.SetPrivateData(guid, DataSize, pData);
}

HRESULT CCryDXGLDevice::SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
{
    return m_kPrivateDataContainer.SetPrivateDataInterface(guid, pData);
}

D3D_FEATURE_LEVEL CCryDXGLDevice::GetFeatureLevel(void)
{
    DXGL_NOT_IMPLEMENTED
    return D3D_FEATURE_LEVEL_11_0;
}

UINT CCryDXGLDevice::GetCreationFlags(void)
{
    DXGL_NOT_IMPLEMENTED
    return 0;
}

HRESULT CCryDXGLDevice::GetDeviceRemovedReason(void)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

void CCryDXGLDevice::GetImmediateContext(ID3D11DeviceContext** ppImmediateContext)
{
    m_spImmediateContext->AddRef();
    CCryDXGLDeviceContext::ToInterface(ppImmediateContext, m_spImmediateContext);
}

HRESULT CCryDXGLDevice::SetExceptionMode(UINT RaiseFlags)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

UINT CCryDXGLDevice::GetExceptionMode(void)
{
    DXGL_NOT_IMPLEMENTED
    return 0;
}
