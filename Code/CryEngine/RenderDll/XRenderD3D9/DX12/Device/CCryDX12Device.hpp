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
#ifndef __CCRYDX12DEVICE__
#define __CCRYDX12DEVICE__

#include "DX12/CCryDX12Object.hpp"
#include "DX12/API/DX12Device.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define CCRYDX12DEVICE_HPP_SECTION_1 1
    #define CCRYDX12DEVICE_HPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CCRYDX12DEVICE_HPP_SECTION_1
    #include AZ_RESTRICTED_FILE(CCryDX12Device_hpp)
#endif

#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    using ICCryDX12Device = ID3D11Device1;
#endif

class CCryDX12Device
    : public CCryDX12Object<ICCryDX12Device>
{
public:
    DX12_OBJECT(CCryDX12Device, CCryDX12Object<ICCryDX12Device>);

    static CCryDX12Device* Create(IDXGIAdapter* pAdapter, D3D_FEATURE_LEVEL* pFeatureLevel);

    CCryDX12Device(DX12::Device* device);

    virtual ~CCryDX12Device();

    DX12::Device* GetDX12Device() const { return m_pDevice; }

    ID3D12Device* GetD3D12Device() const { return m_pDevice->GetD3D12Device(); }

    CCryDX12DeviceContext* GetDeviceContext() const { return m_pContext; }

    #pragma region /* ID3D11Device implementation */

    virtual HRESULT STDMETHODCALLTYPE CreateBuffer(
        _In_  const D3D11_BUFFER_DESC* pDesc,
        _In_opt_  const D3D11_SUBRESOURCE_DATA* pInitialData,
        _Out_opt_  ID3D11Buffer** ppBuffer) final;

    virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(
        _In_  const D3D11_TEXTURE1D_DESC * pDesc,
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture1D * *ppTexture1D) final;

    virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(
        _In_  const D3D11_TEXTURE2D_DESC * pDesc,
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture2D * *ppTexture2D) final;

    virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(
        _In_  const D3D11_TEXTURE3D_DESC * pDesc,
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture3D * *ppTexture3D) final;

    virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
        _In_  ID3D11Resource* pResource,
        _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
        _Out_opt_  ID3D11ShaderResourceView** ppSRView) final;

    virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(
        _In_  ID3D11Resource* pResource,
        _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
        _Out_opt_  ID3D11UnorderedAccessView** ppUAView) final;

    virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
        _In_  ID3D11Resource* pResource,
        _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
        _Out_opt_  ID3D11RenderTargetView** ppRTView) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
        _In_  ID3D11Resource* pResource,
        _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
        _Out_opt_  ID3D11DepthStencilView** ppDepthStencilView) final;

    virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(
        _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC * pInputElementDescs,
        _In_range_(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)  UINT NumElements,
        _In_  const void* pShaderBytecodeWithInputSignature,
        _In_  SIZE_T BytecodeLength,
        _Out_opt_  ID3D11InputLayout * *ppInputLayout) final;

    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11VertexShader** ppVertexShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11GeometryShader** ppGeometryShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_reads_opt_(NumEntries)  const D3D11_SO_DECLARATION_ENTRY * pSODeclaration,
        _In_range_(0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT)  UINT NumEntries,
        _In_reads_opt_(NumStrides)  const UINT * pBufferStrides,
        _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumStrides,
        _In_  UINT RasterizedStream,
        _In_opt_  ID3D11ClassLinkage * pClassLinkage,
        _Out_opt_  ID3D11GeometryShader * *ppGeometryShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11PixelShader** ppPixelShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateHullShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11HullShader** ppHullShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDomainShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11DomainShader** ppDomainShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateComputeShader(
        _In_  const void* pShaderBytecode,
        _In_  SIZE_T BytecodeLength,
        _In_opt_  ID3D11ClassLinkage* pClassLinkage,
        _Out_opt_  ID3D11ComputeShader** ppComputeShader) final;

    virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(
        _Out_  ID3D11ClassLinkage** ppLinkage) final;

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
        _In_  const D3D11_BLEND_DESC* pBlendStateDesc,
        _Out_opt_  ID3D11BlendState** ppBlendState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
        _In_  const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
        _Out_opt_  ID3D11DepthStencilState** ppDepthStencilState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(
        _In_  const D3D11_RASTERIZER_DESC* pRasterizerDesc,
        _Out_opt_  ID3D11RasterizerState** ppRasterizerState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(
        _In_  const D3D11_SAMPLER_DESC* pSamplerDesc,
        _Out_opt_  ID3D11SamplerState** ppSamplerState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateQuery(
        _In_  const D3D11_QUERY_DESC* pQueryDesc,
        _Out_opt_  ID3D11Query** ppQuery) final;

    virtual HRESULT STDMETHODCALLTYPE CreatePredicate(
        _In_  const D3D11_QUERY_DESC* pPredicateDesc,
        _Out_opt_  ID3D11Predicate** ppPredicate) final;

    virtual HRESULT STDMETHODCALLTYPE CreateCounter(
        _In_  const D3D11_COUNTER_DESC* pCounterDesc,
        _Out_opt_  ID3D11Counter** ppCounter) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(
        UINT ContextFlags,
        _Out_opt_  ID3D11DeviceContext** ppDeferredContext) final;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(
        _In_  HANDLE hResource,
        _In_  REFIID ReturnedInterface,
        _Out_opt_  void** ppResource) final;

    virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(
        _In_  DXGI_FORMAT Format,
        _Out_  UINT* pFormatSupport) final;

    virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(
        _In_  DXGI_FORMAT Format,
        _In_  UINT SampleCount,
        _Out_  UINT* pNumQualityLevels) final;

    virtual void STDMETHODCALLTYPE CheckCounterInfo(
        _Out_  D3D11_COUNTER_INFO* pCounterInfo) final;

    virtual HRESULT STDMETHODCALLTYPE CheckCounter(
        _In_  const D3D11_COUNTER_DESC* pDesc,
        _Out_  D3D11_COUNTER_TYPE* pType,
        _Out_  UINT* pActiveCounters,
        _Out_writes_opt_(*pNameLength)  LPSTR szName,
        _Inout_opt_  UINT* pNameLength,
        _Out_writes_opt_(*pUnitsLength)  LPSTR szUnits,
        _Inout_opt_  UINT* pUnitsLength,
        _Out_writes_opt_(*pDescriptionLength)  LPSTR szDescription,
        _Inout_opt_  UINT* pDescriptionLength) final;

    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
        D3D11_FEATURE Feature,
        _Out_writes_bytes_(FeatureSupportDataSize)  void* pFeatureSupportData,
        UINT FeatureSupportDataSize) final;

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        _In_  REFGUID guid,
        _Inout_  UINT* pDataSize,
        _Out_writes_bytes_opt_(*pDataSize)  void* pData) final;

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        _In_  REFGUID guid,
        _In_  UINT DataSize,
        _In_reads_bytes_opt_(DataSize)  const void* pData) final;

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        _In_  REFGUID guid,
        _In_opt_  const IUnknown* pData) final;

    virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel() final;

    virtual UINT STDMETHODCALLTYPE GetCreationFlags() final;

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason() final;

    virtual void STDMETHODCALLTYPE GetImmediateContext(
        _Out_  ID3D11DeviceContext** ppImmediateContext) final;

    virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(
        UINT RaiseFlags) final;

    virtual UINT STDMETHODCALLTYPE GetExceptionMode() final;

    #pragma endregion

    #pragma region /* ID3D11Device1 implementation */

    virtual void STDMETHODCALLTYPE GetImmediateContext1(
        /* [annotation] */
        _Outptr_  ID3D11DeviceContext1** ppImmediateContext) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext1(
        UINT ContextFlags,
        /* [annotation] */
        _COM_Outptr_opt_  ID3D11DeviceContext1** ppDeferredContext) final;

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState1(
        /* [annotation] */
        _In_  const D3D11_BLEND_DESC1* pBlendStateDesc,
        /* [annotation] */
        _COM_Outptr_opt_  ID3D11BlendState1** ppBlendState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState1(
        /* [annotation] */
        _In_  const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
        /* [annotation] */
        _COM_Outptr_opt_  ID3D11RasterizerState1** ppRasterizerState) final;

    virtual HRESULT STDMETHODCALLTYPE CreateDeviceContextState(
        UINT Flags,
        /* [annotation] */
        _In_reads_(FeatureLevels)  const D3D_FEATURE_LEVEL * pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        REFIID EmulatedInterface,
        /* [annotation] */
        _Out_opt_  D3D_FEATURE_LEVEL * pChosenFeatureLevel,
        /* [annotation] */
        _Out_opt_  ID3DDeviceContextState * *ppContextState) final;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource1(
        /* [annotation] */
        _In_  HANDLE hResource,
        /* [annotation] */
        _In_  REFIID returnedInterface,
        /* [annotation] */
        _COM_Outptr_  void** ppResource) final;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResourceByName(
        /* [annotation] */
        _In_  LPCWSTR lpName,
        /* [annotation] */
        _In_  DWORD dwDesiredAccess,
        /* [annotation] */
        _In_  REFIID returnedInterface,
        /* [annotation] */
        _COM_Outptr_  void** ppResource) final;

    #pragma endregion

    HRESULT STDMETHODCALLTYPE CreateTexture1D(
        _In_  const D3D11_TEXTURE1D_DESC * pDesc,
        _In_  const FLOAT cClearValue[4],
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture1D * *ppTexture1D);

    HRESULT STDMETHODCALLTYPE CreateTexture2D(
        _In_  const D3D11_TEXTURE2D_DESC * pDesc,
        _In_  const FLOAT cClearValue[4],
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture2D * *ppTexture2D);

    HRESULT STDMETHODCALLTYPE CreateTexture3D(
        _In_  const D3D11_TEXTURE3D_DESC * pDesc,
        _In_  const FLOAT cClearValue[4],
        _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA * pInitialData,
        _Out_opt_  ID3D11Texture3D * *ppTexture3D);

    HRESULT STDMETHODCALLTYPE CreateStagingResource(
        _In_  ID3D11Resource* pInputResource,
        _Out_ ID3D11Resource** ppStagingResource,
        _In_  BOOL Upload);

    HRESULT STDMETHODCALLTYPE ReleaseStagingResource(
        _In_  ID3D11Resource* pStagingResource);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CCRYDX12DEVICE_HPP_SECTION_2
    #include AZ_RESTRICTED_FILE(CCryDX12Device_hpp)
#endif

private:
    DX12::SmartPtr<DX12::Device> m_pDevice;

    DX12::SmartPtr<CCryDX12DeviceContext> m_pContext;
};

#endif // __CCRYDX12DEVICE__
