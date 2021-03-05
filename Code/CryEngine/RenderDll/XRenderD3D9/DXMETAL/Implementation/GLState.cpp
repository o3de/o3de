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

// Description : Implementation of the functions used to create states
//               and apply them to an OpenGL device

#include "RenderDll_precompiled.h"
#include "GLState.hpp"
#include "MetalDevice.hpp"

namespace NCryMetal
{
    //  Confetti BEGIN: Igor Lobanchikov
    bool GetComparisonFunc(MTLCompareFunction& eMTLCompareFunc, D3D11_COMPARISON_FUNC eD3DCompareFunc)
    {
        switch (eD3DCompareFunc)
        {
        case D3D11_COMPARISON_NEVER:
            eMTLCompareFunc = MTLCompareFunctionNever;
            break;
        case D3D11_COMPARISON_LESS:
            eMTLCompareFunc = MTLCompareFunctionLess;
            break;
        case D3D11_COMPARISON_EQUAL:
            eMTLCompareFunc = MTLCompareFunctionEqual;
            break;
        case D3D11_COMPARISON_LESS_EQUAL:
            eMTLCompareFunc = MTLCompareFunctionLessEqual;
            break;
        case D3D11_COMPARISON_GREATER:
            eMTLCompareFunc = MTLCompareFunctionGreater;
            break;
        case D3D11_COMPARISON_NOT_EQUAL:
            eMTLCompareFunc = MTLCompareFunctionNotEqual;
            break;
        case D3D11_COMPARISON_GREATER_EQUAL:
            eMTLCompareFunc = MTLCompareFunctionGreaterEqual;
            break;
        case D3D11_COMPARISON_ALWAYS:
            eMTLCompareFunc = MTLCompareFunctionAlways;
            break;
        default:
            return false;
        }
        return true;
    }

    bool GetStencilOperation(MTLStencilOperation& eMTLStencilOp, D3D11_STENCIL_OP eD3DStencilOp)
    {
        switch (eD3DStencilOp)
        {
        case D3D11_STENCIL_OP_KEEP:
            eMTLStencilOp = MTLStencilOperationKeep;
            break;
        case D3D11_STENCIL_OP_ZERO:
            eMTLStencilOp = MTLStencilOperationZero;
            break;
        case D3D11_STENCIL_OP_REPLACE:
            eMTLStencilOp = MTLStencilOperationReplace;
            break;
        case D3D11_STENCIL_OP_INCR_SAT:
            eMTLStencilOp = MTLStencilOperationIncrementClamp;
            break;
        case D3D11_STENCIL_OP_DECR_SAT:
            eMTLStencilOp = MTLStencilOperationDecrementClamp;
            break;
        case D3D11_STENCIL_OP_INVERT:
            eMTLStencilOp = MTLStencilOperationInvert;
            break;
        case D3D11_STENCIL_OP_INCR:
            eMTLStencilOp = MTLStencilOperationIncrementWrap;
            break;
        case D3D11_STENCIL_OP_DECR:
            eMTLStencilOp = MTLStencilOperationDecrementWrap;
            break;
        default:
            return false;
        }
        return true;
    }
    //  Confetti End: Igor Lobanchikov

    //  Confetti BEGIN: Igor Lobanchikov
    MTLColorWriteMask DX11ToMetalColorMask(UINT8 RenderTargetWriteMask)
    {
        uint32 res = ((RenderTargetWriteMask & D3D11_COLOR_WRITE_ENABLE_RED) ? MTLColorWriteMaskRed : 0)
            | ((RenderTargetWriteMask & D3D11_COLOR_WRITE_ENABLE_GREEN) ? MTLColorWriteMaskGreen : 0)
            | ((RenderTargetWriteMask & D3D11_COLOR_WRITE_ENABLE_BLUE) ? MTLColorWriteMaskBlue : 0)
            | ((RenderTargetWriteMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) ? MTLColorWriteMaskAlpha : 0);

        return (MTLColorWriteMask)res;
    }

    bool DX11ToMetalBlendOperation(D3D11_BLEND_OP BlendOp, MTLBlendOperation& res)
    {
        switch (BlendOp)
        {
        case D3D11_BLEND_OP_ADD:
            res = MTLBlendOperationAdd;
            break;
        case D3D11_BLEND_OP_SUBTRACT:
            res = MTLBlendOperationSubtract;
            break;
        case D3D11_BLEND_OP_REV_SUBTRACT:
            res = MTLBlendOperationReverseSubtract;
            break;
        case D3D11_BLEND_OP_MIN:
            res = MTLBlendOperationMin;
            break;
        case D3D11_BLEND_OP_MAX:
            res = MTLBlendOperationMax;
            break;

        default:
            DXGL_ERROR("Invalid blend operator");
            return false;
        }
        ;

        return true;
    }

    bool DX11ToMetalBlendFactor(D3D11_BLEND factor, bool bAlpha, MTLBlendFactor& res)
    {
        switch (factor)
        {
        case D3D11_BLEND_ZERO:
            res = MTLBlendFactorZero;
            break;
        case D3D11_BLEND_ONE:
            res = MTLBlendFactorOne;
            break;
        case D3D11_BLEND_SRC_COLOR:
            res = MTLBlendFactorSourceColor;
            break;
        case D3D11_BLEND_INV_SRC_COLOR:
            res = MTLBlendFactorOneMinusSourceColor;
            break;
        case D3D11_BLEND_SRC_ALPHA:
            res = MTLBlendFactorSourceAlpha;
            break;
        case D3D11_BLEND_INV_SRC_ALPHA:
            res = MTLBlendFactorOneMinusSourceAlpha;
            break;
        case D3D11_BLEND_DEST_ALPHA:
            res = MTLBlendFactorDestinationAlpha;
            break;
        case D3D11_BLEND_INV_DEST_ALPHA:
            res = MTLBlendFactorOneMinusDestinationAlpha;
            break;
        case D3D11_BLEND_DEST_COLOR:
            res = MTLBlendFactorDestinationColor;
            break;
        case D3D11_BLEND_INV_DEST_COLOR:
            res = MTLBlendFactorOneMinusDestinationColor;
            break;
        case D3D11_BLEND_SRC_ALPHA_SAT:
            res = MTLBlendFactorSourceAlphaSaturated;
            break;
        case D3D11_BLEND_BLEND_FACTOR:
            res = bAlpha ? MTLBlendFactorBlendAlpha : MTLBlendFactorBlendColor;
            break;
        case D3D11_BLEND_INV_BLEND_FACTOR:
            res = bAlpha ? MTLBlendFactorOneMinusBlendAlpha : MTLBlendFactorOneMinusBlendColor;
            break;

        default:
            DXGL_ERROR("Invalid blend factor");
            return false;
        }

        return true;
    }
    //  Confetti End: Igor Lobanchikov

    bool InitializeBlendState(const D3D11_BLEND_DESC& kDesc, SBlendState& kState, CDevice*)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        assert(!kDesc.AlphaToCoverageEnable && "Not Implemented yet");

        {
            assert(DXGL_ARRAY_SIZE(kDesc.RenderTarget) == DXGL_ARRAY_SIZE(kState.colorAttachements));
            for (uint32 uTarget = 0; uTarget <  DXGL_ARRAY_SIZE(kDesc.RenderTarget); ++uTarget)
            {
                if (uTarget && !kDesc.IndependentBlendEnable)
                {
                    kState.colorAttachements[uTarget] = kState.colorAttachements[0];
                }
                else
                {
                    const D3D11_RENDER_TARGET_BLEND_DESC& kRTDesc(kDesc.RenderTarget[uTarget]);
                    SMetalBlendState& kRTState(kState.colorAttachements[uTarget]);

                    //  Igor: warning: ResetToDefault resets all members. Please, be careful and set other members after reset.
                    kRTState.blendingEnabled = kRTDesc.BlendEnable == TRUE;

                    if (kRTState.blendingEnabled)
                    {
                        if (!DX11ToMetalBlendOperation(kRTDesc.BlendOp, kRTState.rgbBlendOperation))
                        {
                            return false;
                        }
                        if (!DX11ToMetalBlendOperation(kRTDesc.BlendOpAlpha, kRTState.alphaBlendOperation))
                        {
                            return false;
                        }

                        if (!DX11ToMetalBlendFactor(kRTDesc.SrcBlend, false, kRTState.sourceRGBBlendFactor))
                        {
                            return false;
                        }
                        if (!DX11ToMetalBlendFactor(kRTDesc.DestBlend, false, kRTState.destinationRGBBlendFactor))
                        {
                            return false;
                        }

                        if (!DX11ToMetalBlendFactor(kRTDesc.SrcBlendAlpha, true, kRTState.sourceAlphaBlendFactor))
                        {
                            return false;
                        }
                        if (!DX11ToMetalBlendFactor(kRTDesc.DestBlendAlpha, true, kRTState.destinationAlphaBlendFactor))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        kRTState.ResetToDefault();
                    }

                    kRTState.writeMask = DX11ToMetalColorMask(kRTDesc.RenderTargetWriteMask);
                }
            }
        }

        return true;
        //  Confetti End: Igor Lobanchikov
    }

    //  Confetti BEGIN: Igor Lobanchikov

    bool InitializeStencilFace(MTLStencilDescriptor* kStencilFace, const D3D11_DEPTH_STENCILOP_DESC& kDesc, UINT8 uWriteMask, UINT8 uReadMask)
    {
        kStencilFace.readMask = uReadMask;
        kStencilFace.writeMask = uWriteMask;


        MTLCompareFunction cmpFunc;
        if (!GetComparisonFunc(cmpFunc, kDesc.StencilFunc))
        {
            DXGL_ERROR("Invalid stencil comparison function");
            return false;
        }
        kStencilFace.stencilCompareFunction = cmpFunc;

        MTLStencilOperation eStencilFailOperation;
        MTLStencilOperation eDepthFailOperation;
        MTLStencilOperation eDepthPassOperation;

        if (!GetStencilOperation(eStencilFailOperation, kDesc.StencilFailOp) ||
            !GetStencilOperation(eDepthFailOperation, kDesc.StencilDepthFailOp) ||
            !GetStencilOperation(eDepthPassOperation, kDesc.StencilPassOp))
        {
            DXGL_ERROR("Invalid stencil operation");
            return false;
        }
        kStencilFace.stencilFailureOperation = eStencilFailOperation;
        kStencilFace.depthFailureOperation = eDepthFailOperation;
        kStencilFace.depthStencilPassOperation = eDepthPassOperation;

        return true;
    }

    bool InitializeDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& kDesc, id<MTLDepthStencilState>& kState, CDevice* pDevice)
    {
        MTLDepthStencilDescriptor* desc = [[MTLDepthStencilDescriptor alloc] init];
        desc.depthWriteEnabled = (kDesc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL);

        if (kDesc.DepthEnable == TRUE)
        {
            MTLCompareFunction cmpFunc;
            if (!GetComparisonFunc(cmpFunc, kDesc.DepthFunc))
            {
                [desc release];
                DXGL_ERROR("Invalid depth comparison function");
                return false;
            }
            desc.depthCompareFunction = cmpFunc;
        }
        else
        {
            desc.depthCompareFunction = MTLCompareFunctionAlways;
        }

        if (kDesc.StencilEnable == TRUE)
        {
            if (!InitializeStencilFace(desc.frontFaceStencil, kDesc.FrontFace, kDesc.StencilWriteMask, kDesc.StencilReadMask) ||
                !InitializeStencilFace(desc.backFaceStencil, kDesc.BackFace, kDesc.StencilWriteMask, kDesc.StencilReadMask))
            {
                [desc release];
                return false;
            }
        }

        id<MTLDevice> mtlDevice = pDevice->GetMetalDevice();
        kState = [mtlDevice newDepthStencilStateWithDescriptor:desc];
        [desc release];

        return true;
    }
    //  Confetti End: Igor Lobanchikov

    bool InitializeRasterizerState(const D3D11_RASTERIZER_DESC& kDesc, SRasterizerState& kState, CDevice*)
    {
        //  Confetti BEGIN: Igor Lobanchikov
        switch (kDesc.FillMode)
        {
        case D3D11_FILL_SOLID:
            kState.triangleFillMode = MTLTriangleFillModeFill;
            break;
        case D3D11_FILL_WIREFRAME:
            kState.triangleFillMode = MTLTriangleFillModeLines;
            break;
        }

        switch (kDesc.CullMode)
        {
        case D3D11_CULL_NONE:
            kState.cullMode = MTLCullModeNone;
            break;
        case D3D11_CULL_FRONT:
            kState.cullMode = MTLCullModeFront;
            break;
        case D3D11_CULL_BACK:
            kState.cullMode = MTLCullModeBack;
            break;
        }

        kState.frontFaceWinding = kDesc.FrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise;

        kState.depthBias = (float)kDesc.DepthBias;
        kState.depthBiasClamp = kDesc.DepthBiasClamp;
        kState.depthSlopeScale = kDesc.SlopeScaledDepthBias;

        if (!kDesc.DepthClipEnable)
        {
            kState.depthBias = 0;
            kState.depthBiasClamp = 0;
            kState.depthSlopeScale = 0;
        }
        
        kState.depthClipMode = kDesc.DepthClipEnable ? MTLDepthClipModeClip : MTLDepthClipModeClamp;
        
        kState.scissorEnable = kDesc.ScissorEnable;

        if (kDesc.AntialiasedLineEnable == TRUE)
        {
            DXGL_WARNING("Smooth line rasterization is not supported on Metal. This setting will be ignored.");
        }
        if (kDesc.MultisampleEnable == TRUE)
        {
            DXGL_WARNING("Specifying the multisampling mode globally is not supported on Metal. This setting will be ignored.");
        }
        //  Confetti End: Igor Lobanchikov
        return true;
    }

    //  Confetti BEGIN: Igor Lobanchikov

    bool DX11ToMetalTextureAddressMode(D3D11_TEXTURE_ADDRESS_MODE mode, MTLSamplerAddressMode& res)
    {
        switch (mode)
        {
        case D3D11_TEXTURE_ADDRESS_WRAP:
            res = MTLSamplerAddressModeRepeat;
            break;
        case D3D11_TEXTURE_ADDRESS_MIRROR:
            res = MTLSamplerAddressModeMirrorRepeat;
            break;
        case D3D11_TEXTURE_ADDRESS_CLAMP:
            res = MTLSamplerAddressModeClampToEdge;
            break;
        case D3D11_TEXTURE_ADDRESS_BORDER:
            DXGL_WARNING("Sampler with texture border clamping is not supported. Border Zero is used");
            res = MTLSamplerAddressModeClampToZero;
            break;
            return true;
        default:
            DXGL_WARNING("Unsupported sampler address mode");
            return false;
        }
        return true;
    }

    bool DX11ToMetalFilterMode(D3D11_FILTER FilterFunc, MTLSamplerMinMagFilter& minFilter, MTLSamplerMinMagFilter& maxFilter, MTLSamplerMipFilter& mipFilter, bool& bAnizotropic)
    {
        DXMETAL_TODO("Consider getting rid of the comparison samplers for Metal on the engine side.");
        switch (FilterFunc)
        {
#define _FILTER_CASE(_D3D11FilterID, _MinFilter, _MagFilter, _MipFilter, _Anisotropic, _Comparison) \
case _D3D11FilterID:                                                                                \
    minFilter = MTLSamplerMinMagFilter##_MinFilter;                                                 \
    maxFilter = MTLSamplerMinMagFilter##_MagFilter;                                                 \
    mipFilter = MTLSamplerMipFilter##_MipFilter;                                                    \
    bAnizotropic = _Anisotropic;                                                                    \
    assert(!_Comparison || _Comparison);                                                            \
    break;
            _FILTER_CASE(D3D11_FILTER_MIN_MAG_MIP_POINT, Nearest, Nearest, Nearest, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR, Nearest, Nearest, Linear, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, Nearest, Linear, Nearest, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, Nearest, Linear, Linear, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT, Linear, Nearest, Nearest, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, Linear, Nearest, Linear, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, Linear, Linear, Nearest, false, false);
            _FILTER_CASE(D3D11_FILTER_MIN_MAG_MIP_LINEAR, Linear, Linear, Linear, false, false);
            _FILTER_CASE(D3D11_FILTER_ANISOTROPIC, Linear, Linear, Linear, true, false);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT, Nearest, Nearest, Nearest, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR, Nearest, Nearest, Linear, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT, Nearest, Linear, Nearest, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR, Nearest, Linear, Linear, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, Linear, Nearest, Nearest, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR, Linear, Nearest, Linear, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, Linear, Linear, Nearest, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, Linear, Linear, Linear, false, true);
            _FILTER_CASE(D3D11_FILTER_COMPARISON_ANISOTROPIC, Linear, Linear, Linear, true, true);
#undef _FILTER_CASE
        default:
            return false;
        }
        return true;
    }

    bool UpdateMtlSamplerDescriptor(const D3D11_SAMPLER_DESC& kDesc, MTLSamplerDescriptor* desc)
    {
        MTLSamplerAddressMode addressMode;
        
        if (!DX11ToMetalTextureAddressMode(kDesc.AddressU, addressMode))
        {
            return false;
        }
        desc.sAddressMode = addressMode;
        
        if (!DX11ToMetalTextureAddressMode(kDesc.AddressV, addressMode))
        {
            return false;
        }
        desc.tAddressMode = addressMode;
        
        if (!DX11ToMetalTextureAddressMode(kDesc.AddressW, addressMode))
        {
            return false;
        }
        desc.rAddressMode = addressMode;
        
        if (kDesc.MipLODBias)
        {
            DXGL_WARNING("Metal sampler: MipLODBias is not supported.");
        }
        
        desc.maxAnisotropy = kDesc.MaxAnisotropy;
        desc.lodMinClamp = kDesc.MinLOD;
        desc.lodMaxClamp = kDesc.MaxLOD;
        
        MTLSamplerMinMagFilter minFilter;
        MTLSamplerMinMagFilter maxFilter;
        MTLSamplerMipFilter mipFilter;
        bool bAnizotropic;
        
        if (!DX11ToMetalFilterMode(kDesc.Filter, minFilter, maxFilter, mipFilter, bAnizotropic))
        {
            DXGL_WARNING("Metal sampler: filter mode is not supported.");
            return false;
        }
        
        if (!bAnizotropic)
        {
            desc.maxAnisotropy = 1;
        }
        
        desc.minFilter = minFilter;
        desc.magFilter = maxFilter;
        desc.mipFilter = mipFilter;
        
        //D3D11_COMPARISON_FUNC ComparisonFunc;
        DXMETAL_TODO("Consider getting rid of the comparison samplers for Metal on the engine side.");
        //  Igor: Motivation: Metal supports comparison sampler states but they must be declared in the
        //  shader body. This is due to the fact that while newer iOS GPUs support comparison sampling
        //  in hardware while the older ones emulate comparison sampler behaviour using shader maths.
        //  This leads to the fact that the shader compiler has to know comparison sampler configuration
        //  at compile time.
        //  At the moment we still allow to create the sampler object (which can't be configured to be
        //  comparison one) because the engine expects to have one. This might lead to confusion but at the
        //  moment it seems to be the best solution.
        
        return true;
    }
    
    bool InitializeSamplerState(const D3D11_SAMPLER_DESC& kDesc, id<MTLSamplerState>& kState,
                                MTLSamplerDescriptor* mtlSamplerDesc, CDevice* pDevice)
    {
        bool result = UpdateMtlSamplerDescriptor(kDesc, mtlSamplerDesc);
        if(result)
        {
            kState = [pDevice->GetMetalDevice() newSamplerStateWithDescriptor:mtlSamplerDesc];
        }
        return result;
    }
    
    void SetLodMinClamp(id<MTLSamplerState>& mtlSamplerState, MTLSamplerDescriptor* mtlSamplerDesc, const float lodMinClamp, CDevice* pDevice)
    {
        //once mtlSamplerState is created the behavior of a sampler state object is fixed and cannot be changed.
        //Hence we create a new object that has the updated samplerdescriptor.
        mtlSamplerDesc.lodMinClamp = lodMinClamp;
        mtlSamplerState = [pDevice->GetMetalDevice() newSamplerStateWithDescriptor:mtlSamplerDesc];
    }

}
