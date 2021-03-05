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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_NULLD3D11DEVICE_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_NULLD3D11DEVICE_H
#pragma once


#if defined(ENABLE_NULL_D3D11DEVICE)


#if defined(DEVICE_SUPPORTS_D3D11_1)
class NullD3D11Device
    : public D3DDevice
#else
class NullD3D11Device
    : public ID3D11Device
#endif
{
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] REFIID riid, void** ppvObj)
    {
        if (ppvObj)
        {
            *ppvObj = 0;
        }
        return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return CryInterlockedIncrement(&m_refCount);
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        long refCount = CryInterlockedDecrement(&m_refCount);
        if (refCount <= 0)
        {
            delete this;
        }
        return refCount;
    };

    // ID3D11Device
    virtual HRESULT STDMETHODCALLTYPE CreateBuffer([[maybe_unused]] const D3D11_BUFFER_DESC* pDesc, [[maybe_unused]] const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer)
    {
        if (ppBuffer)
        {
            *ppBuffer = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateTexture1D([[maybe_unused]] const D3D11_TEXTURE1D_DESC* pDesc, [[maybe_unused]] const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture1D** ppTexture1D)
    {
        if (ppTexture1D)
        {
            *ppTexture1D = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateTexture2D([[maybe_unused]] const D3D11_TEXTURE2D_DESC* pDesc, [[maybe_unused]] const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
    {
        if (ppTexture2D)
        {
            *ppTexture2D = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateTexture3D([[maybe_unused]] const D3D11_TEXTURE3D_DESC* pDesc, [[maybe_unused]] const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D)
    {
        if (ppTexture3D)
        {
            *ppTexture3D = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
    {
        if (ppSRView)
        {
            *ppSRView = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppUAView)
    {
        if (ppUAView)
        {
            *ppUAView = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
    {
        if (ppRTView)
        {
            *ppRTView = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDepthStencilView)
    {
        if (ppDepthStencilView)
        {
            *ppDepthStencilView = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateInputLayout([[maybe_unused]] const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, [[maybe_unused]] UINT NumElements, [[maybe_unused]] const void* pShaderBytecodeWithInputSignature, [[maybe_unused]] SIZE_T BytecodeLength, ID3D11InputLayout** ppInputLayout)
    {
        if (ppInputLayout)
        {
            *ppInputLayout = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader)
    {
        if (ppVertexShader)
        {
            *ppVertexShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
    {
        if (ppGeometryShader)
        {
            *ppGeometryShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, [[maybe_unused]] UINT NumEntries, [[maybe_unused]] const UINT* pBufferStrides, [[maybe_unused]] UINT NumStrides, [[maybe_unused]] UINT RasterizedStream, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
    {
        if (ppGeometryShader)
        {
            *ppGeometryShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader)
    {
        if (ppPixelShader)
        {
            *ppPixelShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateHullShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11HullShader** ppHullShader)
    {
        if (ppHullShader)
        {
            *ppHullShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDomainShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11DomainShader** ppDomainShader)
    {
        if (ppDomainShader)
        {
            *ppDomainShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateComputeShader([[maybe_unused]] const void* pShaderBytecode, [[maybe_unused]] SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage, ID3D11ComputeShader** ppComputeShader)
    {
        if (ppComputeShader)
        {
            *ppComputeShader = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(ID3D11ClassLinkage** ppLinkage)
    {
        if (ppLinkage)
        {
            *ppLinkage = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState([[maybe_unused]] const D3D11_BLEND_DESC* pBlendStateDesc, ID3D11BlendState** ppBlendState)
    {
        if (ppBlendState)
        {
            *ppBlendState = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState([[maybe_unused]] const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState)
    {
        if (ppDepthStencilState)
        {
            *ppDepthStencilState = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState([[maybe_unused]] const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState)
    {
        if (ppRasterizerState)
        {
            *ppRasterizerState = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateSamplerState([[maybe_unused]] const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState)
    {
        if (ppSamplerState)
        {
            *ppSamplerState = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateQuery([[maybe_unused]] const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery)
    {
        if (ppQuery)
        {
            *ppQuery = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreatePredicate([[maybe_unused]] const D3D11_QUERY_DESC* pPredicateDesc, ID3D11Predicate** ppPredicate)
    {
        if (ppPredicate)
        {
            *ppPredicate = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCounter([[maybe_unused]] const D3D11_COUNTER_DESC* pCounterDesc, ID3D11Counter** ppCounter)
    {
        if (ppCounter)
        {
            *ppCounter = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext([[maybe_unused]] UINT ContextFlags, ID3D11DeviceContext** ppDeferredContext)
    {
        if (ppDeferredContext)
        {
            *ppDeferredContext = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource([[maybe_unused]] HANDLE hResource, [[maybe_unused]] REFIID ReturnedInterface, void** ppResource)
    {
        if (ppResource)
        {
            *ppResource = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport([[maybe_unused]] DXGI_FORMAT Format, UINT* pFormatSupport)
    {
        if (pFormatSupport)
        {
            pFormatSupport = 0;
        }
        return E_FAIL;
    }

    virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels([[maybe_unused]] DXGI_FORMAT Format, [[maybe_unused]] UINT SampleCount, UINT* pNumQualityLevels)
    {
        if (pNumQualityLevels)
        {
            pNumQualityLevels = 0;
        }
        return S_FALSE;
    }

    virtual void STDMETHODCALLTYPE CheckCounterInfo(D3D11_COUNTER_INFO* pCounterInfo)
    {
        if (pCounterInfo)
        {
            pCounterInfo->LastDeviceDependentCounter = D3D11_COUNTER_DEVICE_DEPENDENT_0;
            pCounterInfo->NumDetectableParallelUnits = 0;
            pCounterInfo->NumSimultaneousCounters = 0;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE CheckCounter([[maybe_unused]] const D3D11_COUNTER_DESC* pDesc, D3D11_COUNTER_TYPE* pType, UINT* pActiveCounters, LPSTR szName, UINT* pNameLength, LPSTR szUnits, UINT* pUnitsLength, LPSTR szDescription, UINT* pDescriptionLength)
    {
        if (pType)
        {
            *pType = D3D11_COUNTER_TYPE_UINT32;
        }
        if (pActiveCounters)
        {
            *pActiveCounters = 0;
        }
        if (szName)
        {
            *szName = 0;
        }
        if (pNameLength)
        {
            *pNameLength = 0;
        }
        if (szUnits)
        {
            *szUnits = 0;
        }
        if (pUnitsLength)
        {
            *pUnitsLength = 0;
        }
        if (szDescription)
        {
            *szDescription = 0;
        }
        if (pDescriptionLength)
        {
            *pDescriptionLength = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport([[maybe_unused]] D3D11_FEATURE Feature, [[maybe_unused]] void* pFeatureSupportData, [[maybe_unused]] UINT FeatureSupportDataSize)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData([[maybe_unused]] REFGUID guid, UINT* pDataSize, [[maybe_unused]] void* pData)
    {
        if (pDataSize)
        {
            *pDataSize = 0;
        }
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData([[maybe_unused]] REFGUID guid, [[maybe_unused]] UINT DataSize, [[maybe_unused]] const void* pData)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface([[maybe_unused]] REFGUID guid, [[maybe_unused]] const IUnknown* pData)
    {
        return S_FALSE;
    }

    virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel()
    {
        return D3D_FEATURE_LEVEL_11_0;
    }

    virtual UINT STDMETHODCALLTYPE GetCreationFlags()
    {
        return 0;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason()
    {
        return S_OK;
    }

    virtual void STDMETHODCALLTYPE GetImmediateContext(ID3D11DeviceContext** ppImmediateContext);

    virtual HRESULT STDMETHODCALLTYPE SetExceptionMode([[maybe_unused]] UINT RaiseFlags)
    {
        return S_FALSE;
    }

    virtual UINT STDMETHODCALLTYPE GetExceptionMode()
    {
        return 0;
    }

#if defined(DEVICE_SUPPORTS_D3D11_1)
    virtual void STDMETHODCALLTYPE GetImmediateContext1(ID3D11DeviceContext1** ppImmediateContext);

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext1(UINT ContextFlags, ID3D11DeviceContext1** ppDeferredContext)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState1(const D3D11_BLEND_DESC1* pBlendStateDesc, ID3D11BlendState1** ppBlendState)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState1(const D3D11_RASTERIZER_DESC1* pRasterizerDesc, ID3D11RasterizerState1** ppRasterizerState)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDeviceContextState(UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, REFIID EmulatedInterface, D3D_FEATURE_LEVEL* pChosenFeatureLevel, ID3DDeviceContextState** ppContextState)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource1(HANDLE hResource, REFIID returnedInterface, void** ppResource)
    {
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResourceByName(LPCWSTR lpName, DWORD dwDesiredAccess, REFIID returnedInterface, void** ppResource)
    {
        return S_FALSE;
    }
#endif

public:
    NullD3D11Device()
        : m_refCount(1)
        , m_pImmediateCtx(0) {}

protected:
    virtual ~NullD3D11Device()
    {
        SAFE_RELEASE(m_pImmediateCtx);
    }

protected:
    volatile int m_refCount;

#if defined(DEVICE_SUPPORTS_D3D11_1)
    ID3D11DeviceContext1* m_pImmediateCtx;
#else
    ID3D11DeviceContext* m_pImmediateCtx;
#endif
};


namespace NullD3D11DeviceInternal
{
#if defined(DEVICE_SUPPORTS_D3D11_1)
    class DeviceContext
        : public ID3D11DeviceContext1
#else
    class DeviceContext
        : public ID3D11DeviceContext
#endif
    {
    public:
        // IUnknown
        virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] REFIID riid, void** ppvObj)
        {
            if (ppvObj)
            {
                *ppvObj = 0;
            }
            return E_NOINTERFACE;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef()
        {
            return CryInterlockedIncrement(&m_refCount);
        }

        virtual ULONG STDMETHODCALLTYPE Release()
        {
            long refCount = CryInterlockedDecrement(&m_refCount);
            if (refCount <= 0)
            {
                delete this;
            }
            return refCount;
        };

        // ID3D11DeviceChild
        virtual void STDMETHODCALLTYPE GetDevice(ID3D11Device** ppDevice)
        {
            if (ppDevice)
            {
                if (m_pDevice)
                {
                    m_pDevice->AddRef();
                }
                *ppDevice = m_pDevice;
            }
        }

        virtual HRESULT STDMETHODCALLTYPE GetPrivateData([[maybe_unused]] REFGUID guid, UINT* pDataSize, [[maybe_unused]] void* pData)
        {
            if (pDataSize)
            {
                *pDataSize = 0;
            }
            return S_FALSE;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateData([[maybe_unused]] REFGUID guid, [[maybe_unused]] UINT DataSize, [[maybe_unused]] const void* pData)
        {
            return S_FALSE;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface([[maybe_unused]] REFGUID guid, [[maybe_unused]] const IUnknown* pData)
        {
            return S_FALSE;
        }

        // ID3D11DeviceContext
        virtual void STDMETHODCALLTYPE VSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}
        virtual void STDMETHODCALLTYPE PSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE PSSetShader([[maybe_unused]] ID3D11PixelShader* pPixelShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE PSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE VSSetShader([[maybe_unused]] ID3D11VertexShader* pVertexShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE DrawIndexed([[maybe_unused]] UINT IndexCount, [[maybe_unused]] UINT StartIndexLocation, [[maybe_unused]] INT BaseVertexLocation) {}
        virtual void STDMETHODCALLTYPE Draw([[maybe_unused]] UINT VertexCount, [[maybe_unused]] UINT StartVertexLocation) {}

        virtual HRESULT STDMETHODCALLTYPE Map([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] UINT Subresource, [[maybe_unused]] D3D11_MAP MapType, [[maybe_unused]] UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
        {
            if (pMappedResource)
            {
                pMappedResource->RowPitch = 0;
                pMappedResource->DepthPitch = 0;
                pMappedResource->pData = 0;
            }
            return S_FALSE;
        }

        virtual void STDMETHODCALLTYPE Unmap([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] UINT Subresource) {}
        virtual void STDMETHODCALLTYPE PSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}
        virtual void STDMETHODCALLTYPE IASetInputLayout([[maybe_unused]] ID3D11InputLayout* pInputLayout) {}
        virtual void STDMETHODCALLTYPE IASetVertexBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppVertexBuffers, [[maybe_unused]] const UINT* pStrides, [[maybe_unused]] const UINT* pOffsets) {}
        virtual void STDMETHODCALLTYPE IASetIndexBuffer([[maybe_unused]] ID3D11Buffer* pIndexBuffer, [[maybe_unused]] DXGI_FORMAT Format, [[maybe_unused]] UINT Offset) {}
        virtual void STDMETHODCALLTYPE DrawIndexedInstanced([[maybe_unused]] UINT IndexCountPerInstance, [[maybe_unused]] UINT InstanceCount, [[maybe_unused]] UINT StartIndexLocation, [[maybe_unused]] INT BaseVertexLocation, [[maybe_unused]] UINT StartInstanceLocation) {}
        virtual void STDMETHODCALLTYPE DrawInstanced([[maybe_unused]] UINT VertexCountPerInstance, [[maybe_unused]] UINT InstanceCount, [[maybe_unused]] UINT StartVertexLocation, [[maybe_unused]] UINT StartInstanceLocation) {}
        virtual void STDMETHODCALLTYPE GSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}
        virtual void STDMETHODCALLTYPE GSSetShader([[maybe_unused]] ID3D11GeometryShader* pShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE IASetPrimitiveTopology([[maybe_unused]] D3D11_PRIMITIVE_TOPOLOGY Topology) {}
        virtual void STDMETHODCALLTYPE VSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE VSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE Begin([[maybe_unused]] ID3D11Asynchronous* pAsync) {}
        virtual void STDMETHODCALLTYPE End([[maybe_unused]] ID3D11Asynchronous* pAsync) {}

        virtual HRESULT STDMETHODCALLTYPE GetData([[maybe_unused]] ID3D11Asynchronous* pAsync, [[maybe_unused]] void* pData, [[maybe_unused]] UINT DataSize, [[maybe_unused]] UINT GetDataFlags)
        {
            return S_FALSE;
        }

        virtual void STDMETHODCALLTYPE SetPredication([[maybe_unused]] ID3D11Predicate* pPredicate, [[maybe_unused]] BOOL PredicateValue) {}
        virtual void STDMETHODCALLTYPE GSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE GSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE OMSetRenderTargets([[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11RenderTargetView* const* ppRenderTargetViews, [[maybe_unused]] ID3D11DepthStencilView* pDepthStencilView) {}
        virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews([[maybe_unused]] UINT NumRTVs, [[maybe_unused]]  ID3D11RenderTargetView* const* ppRenderTargetViews, [[maybe_unused]] ID3D11DepthStencilView* pDepthStencilView, [[maybe_unused]] UINT UAVStartSlot, [[maybe_unused]] UINT NumUAVs, [[maybe_unused]] ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, [[maybe_unused]] const UINT* pUAVInitialCounts) {}
        virtual void STDMETHODCALLTYPE OMSetBlendState([[maybe_unused]] ID3D11BlendState* pBlendState, [[maybe_unused]] const FLOAT BlendFactor[ 4 ], [[maybe_unused]] UINT SampleMask) {}
        virtual void STDMETHODCALLTYPE OMSetDepthStencilState([[maybe_unused]] ID3D11DepthStencilState* pDepthStencilState, [[maybe_unused]] UINT StencilRef) {}
        virtual void STDMETHODCALLTYPE SOSetTargets([[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppSOTargets, [[maybe_unused]] const UINT* pOffsets) {}
        virtual void STDMETHODCALLTYPE DrawAuto(void) {}
        virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect([[maybe_unused]] ID3D11Buffer* pBufferForArgs, [[maybe_unused]] UINT AlignedByteOffsetForArgs) {}
        virtual void STDMETHODCALLTYPE DrawInstancedIndirect([[maybe_unused]] ID3D11Buffer* pBufferForArgs, [[maybe_unused]] UINT AlignedByteOffsetForArgs) {}
        virtual void STDMETHODCALLTYPE Dispatch([[maybe_unused]] UINT ThreadGroupCountX, [[maybe_unused]] UINT ThreadGroupCountY, [[maybe_unused]] UINT ThreadGroupCountZ) {}
        virtual void STDMETHODCALLTYPE DispatchIndirect([[maybe_unused]] ID3D11Buffer* pBufferForArgs, [[maybe_unused]] UINT AlignedByteOffsetForArgs) {}
        virtual void STDMETHODCALLTYPE RSSetState([[maybe_unused]] ID3D11RasterizerState* pRasterizerState) {}
        virtual void STDMETHODCALLTYPE RSSetViewports([[maybe_unused]] UINT NumViewports, [[maybe_unused]] const D3D11_VIEWPORT* pViewports) {}
        virtual void STDMETHODCALLTYPE RSSetScissorRects([[maybe_unused]] UINT NumRects, [[maybe_unused]] const D3D11_RECT* pRects) {}
        virtual void STDMETHODCALLTYPE CopySubresourceRegion([[maybe_unused]] ID3D11Resource* pDstResource, [[maybe_unused]] UINT DstSubresource, [[maybe_unused]] UINT DstX, [[maybe_unused]] UINT DstY, [[maybe_unused]] UINT DstZ, [[maybe_unused]] ID3D11Resource* pSrcResource, [[maybe_unused]] UINT SrcSubresource, [[maybe_unused]] const D3D11_BOX* pSrcBox) {}
        virtual void STDMETHODCALLTYPE CopyResource([[maybe_unused]] ID3D11Resource* pDstResource, [[maybe_unused]] ID3D11Resource* pSrcResource) {}
        virtual void STDMETHODCALLTYPE UpdateSubresource([[maybe_unused]] ID3D11Resource* pDstResource, [[maybe_unused]] UINT DstSubresource, [[maybe_unused]] const D3D11_BOX* pDstBox, [[maybe_unused]] const void* pSrcData, [[maybe_unused]] UINT SrcRowPitch, [[maybe_unused]] UINT SrcDepthPitch) {}
        virtual void STDMETHODCALLTYPE CopyStructureCount([[maybe_unused]] ID3D11Buffer* pDstBuffer, [[maybe_unused]] UINT DstAlignedByteOffset, [[maybe_unused]] ID3D11UnorderedAccessView* pSrcView) {}
        virtual void STDMETHODCALLTYPE ClearRenderTargetView([[maybe_unused]] ID3D11RenderTargetView* pRenderTargetView, [[maybe_unused]] const FLOAT ColorRGBA[ 4 ]) {}
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint([[maybe_unused]] ID3D11UnorderedAccessView* pUnorderedAccessView, [[maybe_unused]] const UINT Values[ 4 ]) {}
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat([[maybe_unused]] ID3D11UnorderedAccessView* pUnorderedAccessView, [[maybe_unused]] const FLOAT Values[ 4 ]) {}
        virtual void STDMETHODCALLTYPE ClearDepthStencilView([[maybe_unused]] ID3D11DepthStencilView* pDepthStencilView, [[maybe_unused]] UINT ClearFlags, [[maybe_unused]] FLOAT Depth, [[maybe_unused]] UINT8 Stencil) {}
        virtual void STDMETHODCALLTYPE GenerateMips([[maybe_unused]] ID3D11ShaderResourceView* pShaderResourceView) {}
        virtual void STDMETHODCALLTYPE SetResourceMinLOD([[maybe_unused]] ID3D11Resource* pResource, [[maybe_unused]] FLOAT MinLOD) {}

        virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD([[maybe_unused]] ID3D11Resource* pResource)
        {
            return 0;
        }

        virtual void STDMETHODCALLTYPE ResolveSubresource([[maybe_unused]] ID3D11Resource* pDstResource, [[maybe_unused]] UINT DstSubresource, [[maybe_unused]] ID3D11Resource* pSrcResource, [[maybe_unused]] UINT SrcSubresource, [[maybe_unused]] DXGI_FORMAT Format) {}
        virtual void STDMETHODCALLTYPE ExecuteCommandList([[maybe_unused]] ID3D11CommandList* pCommandList, [[maybe_unused]] BOOL RestoreContextState) {}
        virtual void STDMETHODCALLTYPE HSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE HSSetShader([[maybe_unused]] ID3D11HullShader* pHullShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE HSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE HSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}
        virtual void STDMETHODCALLTYPE DSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE DSSetShader([[maybe_unused]] ID3D11DomainShader* pDomainShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE DSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE DSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}
        virtual void STDMETHODCALLTYPE CSSetShaderResources([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumViews, [[maybe_unused]] ID3D11ShaderResourceView* const* ppShaderResourceViews) {}
        virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumUAVs, [[maybe_unused]] ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, [[maybe_unused]] const UINT* pUAVInitialCounts) {}
        virtual void STDMETHODCALLTYPE CSSetShader([[maybe_unused]] ID3D11ComputeShader* pComputeShader, [[maybe_unused]] ID3D11ClassInstance* const* ppClassInstances, [[maybe_unused]] UINT NumClassInstances) {}
        virtual void STDMETHODCALLTYPE CSSetSamplers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumSamplers, [[maybe_unused]] ID3D11SamplerState* const* ppSamplers) {}
        virtual void STDMETHODCALLTYPE CSSetConstantBuffers([[maybe_unused]] UINT StartSlot, [[maybe_unused]] UINT NumBuffers, [[maybe_unused]] ID3D11Buffer* const* ppConstantBuffers) {}

        virtual void STDMETHODCALLTYPE VSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE PSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE PSGetShader(ID3D11PixelShader** ppPixelShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppPixelShader)
            {
                *ppPixelShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE PSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE VSGetShader(ID3D11VertexShader** ppVertexShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppVertexShader)
            {
                *ppVertexShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE PSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE IAGetInputLayout(ID3D11InputLayout** ppInputLayout)
        {
            if (ppInputLayout)
            {
                *ppInputLayout = 0;
            }
        }

        virtual void STDMETHODCALLTYPE IAGetVertexBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppVertexBuffers, UINT* pStrides, UINT* pOffsets)
        {
            if (ppVertexBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppVertexBuffers[i] = 0;
                    pStrides[i] = 0;
                    pOffsets[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE IAGetIndexBuffer(ID3D11Buffer** pIndexBuffer, DXGI_FORMAT* Format, UINT* Offset)
        {
            if (pIndexBuffer)
            {
                pIndexBuffer = 0;
            }
            if (Format)
            {
                *Format = DXGI_FORMAT_UNKNOWN;
            }
            if (Offset)
            {
                *Offset = 0;
            }
        }

        virtual void STDMETHODCALLTYPE GSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE GSGetShader(ID3D11GeometryShader** ppGeometryShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppGeometryShader)
            {
                *ppGeometryShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY* pTopology)
        {
            if (pTopology)
            {
                *pTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
            }
        }

        virtual void STDMETHODCALLTYPE VSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE VSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE GetPredication(ID3D11Predicate** ppPredicate, BOOL* pPredicateValue)
        {
            if (ppPredicate)
            {
                *ppPredicate = 0;
            }
            if (pPredicateValue)
            {
                *pPredicateValue = FALSE;
            }
        }

        virtual void STDMETHODCALLTYPE GSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE GSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView)
        {
            if (ppRenderTargetViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppRenderTargetViews[i] = 0;
                }
            }

            if (ppDepthStencilView)
            {
                *ppDepthStencilView = 0;
            }
        }

        virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView, [[maybe_unused]] UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
        {
            if (ppRenderTargetViews)
            {
                for (UINT i = 0; i < NumRTVs; ++i)
                {
                    ppRenderTargetViews[i] = 0;
                }
            }

            if (ppDepthStencilView)
            {
                *ppDepthStencilView = 0;
            }

            if (ppUnorderedAccessViews)
            {
                for (UINT i = 0; i < NumUAVs; ++i)
                {
                    ppUnorderedAccessViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE OMGetBlendState(ID3D11BlendState** ppBlendState, FLOAT BlendFactor[ 4 ], UINT* pSampleMask)
        {
            if (ppBlendState)
            {
                *ppBlendState = 0;
            }
            BlendFactor[0] = BlendFactor[1] = BlendFactor[2] = BlendFactor[3] = 0;
            if (pSampleMask)
            {
                *pSampleMask = 0;
            }
        }

        virtual void STDMETHODCALLTYPE OMGetDepthStencilState(ID3D11DepthStencilState** ppDepthStencilState, UINT* pStencilRef)
        {
            if (ppDepthStencilState)
            {
                *ppDepthStencilState = 0;
            }
            if (pStencilRef)
            {
                *pStencilRef = 0;
            }
        }

        virtual void STDMETHODCALLTYPE SOGetTargets(UINT NumBuffers, ID3D11Buffer** ppSOTargets)
        {
            if (ppSOTargets)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppSOTargets[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE RSGetState(ID3D11RasterizerState** ppRasterizerState)
        {
            if (ppRasterizerState)
            {
                *ppRasterizerState = 0;
            }
        }

        virtual void STDMETHODCALLTYPE RSGetViewports(UINT* pNumViewports, [[maybe_unused]] D3D11_VIEWPORT* pViewports)
        {
            if (pNumViewports)
            {
                *pNumViewports = 0;
            }
        }

        virtual void STDMETHODCALLTYPE RSGetScissorRects(UINT* pNumRects, [[maybe_unused]] D3D11_RECT* pRects)
        {
            if (pNumRects)
            {
                *pNumRects = 0;
            }
        }

        virtual void STDMETHODCALLTYPE HSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE HSGetShader(ID3D11HullShader** ppHullShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppHullShader)
            {
                *ppHullShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE HSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE HSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE DSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE DSGetShader(ID3D11DomainShader** ppDomainShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppDomainShader)
            {
                *ppDomainShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE DSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE DSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE CSGetShaderResources([[maybe_unused]] UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
        {
            if (ppShaderResourceViews)
            {
                for (UINT i = 0; i < NumViews; ++i)
                {
                    ppShaderResourceViews[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews([[maybe_unused]] UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
        {
            if (ppUnorderedAccessViews)
            {
                for (UINT i = 0; i < NumUAVs; ++i)
                {
                    ppUnorderedAccessViews[i] = 0;
                }
            }
        }
        virtual void STDMETHODCALLTYPE CSGetShader(ID3D11ComputeShader** ppComputeShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
        {
            if (ppComputeShader)
            {
                *ppComputeShader = 0;
            }
            if (ppClassInstances)
            {
                *ppClassInstances = 0;
            }
            if (pNumClassInstances)
            {
                pNumClassInstances = 0;
            }
        }

        virtual void STDMETHODCALLTYPE CSGetSamplers([[maybe_unused]] UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
        {
            if (ppSamplers)
            {
                for (UINT i = 0; i < NumSamplers; ++i)
                {
                    ppSamplers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE CSGetConstantBuffers([[maybe_unused]] UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
        {
            if (ppConstantBuffers)
            {
                for (UINT i = 0; i < NumBuffers; ++i)
                {
                    ppConstantBuffers[i] = 0;
                }
            }
        }

        virtual void STDMETHODCALLTYPE ClearState() {}
        virtual void STDMETHODCALLTYPE Flush() {}

        virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType()
        {
            return D3D11_DEVICE_CONTEXT_IMMEDIATE;
        }

        virtual UINT STDMETHODCALLTYPE GetContextFlags()
        {
            return 0;
        }

        virtual HRESULT STDMETHODCALLTYPE FinishCommandList([[maybe_unused]] BOOL RestoreDeferredContextState, ID3D11CommandList** ppCommandList)
        {
            if (ppCommandList)
            {
                *ppCommandList = 0;
            }
            return DXGI_ERROR_INVALID_CALL;
        }

#if defined(DEVICE_SUPPORTS_D3D11_1)
        void STDMETHODCALLTYPE CopySubresourceRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags) {}
        void STDMETHODCALLTYPE CopyResource1(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource, UINT CopyFlags) {}
        void STDMETHODCALLTYPE UpdateSubresource1(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags) {}
        void STDMETHODCALLTYPE DiscardResource(ID3D11Resource* pResource) {}
        void STDMETHODCALLTYPE DiscardView(ID3D11View* pResourceView) {}

        void STDMETHODCALLTYPE VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}
        void STDMETHODCALLTYPE HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}
        void STDMETHODCALLTYPE DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}
        void STDMETHODCALLTYPE GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}
        void STDMETHODCALLTYPE PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}
        void STDMETHODCALLTYPE CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) {}

        void STDMETHODCALLTYPE VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}
        void STDMETHODCALLTYPE HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}
        void STDMETHODCALLTYPE DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}
        void STDMETHODCALLTYPE GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}
        void STDMETHODCALLTYPE PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}
        void STDMETHODCALLTYPE CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) {}

        void STDMETHODCALLTYPE SwapDeviceContextState(ID3DDeviceContextState* pState, ID3DDeviceContextState** ppPreviousState) {}
        void STDMETHODCALLTYPE ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects) {}
        void STDMETHODCALLTYPE DiscardView1(ID3D11View* pResourceView, const D3D11_RECT* pRects, UINT NumRects) {}
#endif

    protected:
        virtual ~DeviceContext()
        {
        }

#if defined(DEVICE_SUPPORTS_D3D11_1)
    public:
        DeviceContext(ID3D11Device1* pDevice)
            : m_refCount(1)
            , m_pDevice(pDevice) {}

    protected:
        volatile int m_refCount;
        ID3D11Device1* m_pDevice;
#else
    public:
        DeviceContext(ID3D11Device* pDevice)
            : m_refCount(1)
            , m_pDevice(pDevice) {}

    protected:
        volatile int m_refCount;
        ID3D11Device* m_pDevice;
#endif
    };
} // namespace NullD3D11DeviceInternal


void NullD3D11Device::GetImmediateContext(ID3D11DeviceContext** ppImmediateContext)
{
    if (ppImmediateContext)
    {
        if (!m_pImmediateCtx)
        {
            m_pImmediateCtx = new NullD3D11DeviceInternal::DeviceContext(this);
        }

        m_pImmediateCtx->AddRef();
        *ppImmediateContext = m_pImmediateCtx;
    }
}

#if defined(DEVICE_SUPPORTS_D3D11_1)
void NullD3D11Device::GetImmediateContext1(ID3D11DeviceContext1** ppImmediateContext)
{
    if (ppImmediateContext)
    {
        if (!m_pImmediateCtx)
        {
            m_pImmediateCtx = new NullD3D11DeviceInternal::DeviceContext(this);
        }

        m_pImmediateCtx->AddRef();
        *ppImmediateContext = m_pImmediateCtx;
    }
}
#endif


#endif // #if defined(ENABLE_NULL_D3D11DEVICE)

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_NULLD3D11DEVICE_H
