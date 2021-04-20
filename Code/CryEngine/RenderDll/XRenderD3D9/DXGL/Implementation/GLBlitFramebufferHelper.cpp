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

#include "RenderDll_precompiled.h"
#include <Implementation/GLBlitFramebufferHelper.hpp>
#include <Implementation/GLBlitShaders.hpp>
#include <Implementation/GLContext.hpp>
#if DXGL_INPUT_GLSL && DXGL_GLSL_FROM_HLSLCROSSCOMPILER
#include <hlslcc.hpp>
#endif //DXGL_INPUT_GLSL && DXGL_GLSL_FROM_HLSLCROSSCOMPILER
#include <hlslcc_bin.hpp>

namespace NCryOpenGL
{
    GLBlitFramebufferHelper::GLBlitFramebufferHelper(CContext& context)
        : m_context(context)
        , m_initialized(false)
        , m_UVMatrixCache(ZERO)
    {
        ZeroMemory(&m_samplerState, sizeof(m_samplerState));
    }

    GLBlitFramebufferHelper::~GLBlitFramebufferHelper()
    {
        Reset();
    }

    bool GLBlitFramebufferHelper::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        m_vertexShader.m_eType = eST_Vertex;
        m_fragmentShader.m_eType = eST_Fragment;

        size_t vsLength = strlen(s_blitVertexShader) + 1;
        size_t fsLength = strlen(s_blitFragmentShader) + 1;
        size_t vsDataSize = GLSL_RESOURCE_SIZE + vsLength;
        size_t fsDataSize = GLSL_SAMPLER_SIZE + fsLength;

        // In order to reuse the code to compile the vertex/fragment shaders and initialize the resources
        // we add some DXBC metadata to the shaders data.
        AZStd::vector<uint8> buffer(AZStd::max(vsDataSize, fsDataSize));
        SDXBCOutputBuffer vertStream(buffer.data(), buffer.data() + buffer.size());
        const char* uniformBufferPos = strstr(s_blitVertexShader, s_uniformBufferName);
        if (!uniformBufferPos)
        {
            Reset();
            return false;
        }

        uint32 uniformBufferOffset = static_cast<uint32>(uniformBufferPos - s_blitVertexShader);
        DXBCWriteUint32(vertStream, DXBC::EncodeResourceIndex(0));
        DXBCWriteUint32(vertStream, DXBC::EncodeEmbeddedName(uniformBufferOffset, strlen(s_uniformBufferName)));
        vertStream.Write(s_blitVertexShader, vsLength);

        m_vertexShader.m_akVersions[eSV_Normal].m_kReflection.m_uNumUniformBuffers = 1;
        m_vertexShader.m_akVersions[eSV_Normal].m_kReflection.m_uGLSLSourceOffset = GLSL_RESOURCE_SIZE;
        m_vertexShader.m_akVersions[eSV_Normal].m_kSource.SetData(reinterpret_cast<const char*>(buffer.data()), vsDataSize);

        SDXBCOutputBuffer fragStream(buffer.data(), buffer.data() + buffer.size());
        const char* textureName = strstr(s_blitFragmentShader, s_tex0SamplerName);
        if (!textureName)
        {
            Reset();
            return false;
        }

        uint32 textureOffset = static_cast<uint32>(textureName - s_blitFragmentShader);
        DXBCWriteUint32(fragStream, DXBC::EncodeTextureData(0, 0, 0, true, false));
        DXBCWriteUint32(fragStream, DXBC::EncodeEmbeddedName(textureOffset, strlen(s_tex0SamplerName)));
        DXBCWriteUint32(fragStream, 0);
        fragStream.Write(s_blitFragmentShader, fsLength);

        m_fragmentShader.m_akVersions[eSV_Normal].m_kReflection.m_uNumSamplers = 1;
        m_fragmentShader.m_akVersions[eSV_Normal].m_kReflection.m_uGLSLSourceOffset = GLSL_SAMPLER_SIZE;
        m_fragmentShader.m_akVersions[eSV_Normal].m_kSource.SetData(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        SShaderReflectionParameter paramReflection;
        paramReflection.m_kDesc.SemanticName = s_vertexInput0;
        paramReflection.m_kDesc.SemanticIndex = 0;
        paramReflection.m_kDesc.Register = 0;
        paramReflection.m_kDesc.ComponentType = D3D_REGISTER_COMPONENT_FLOAT32;
        m_vertexShader.m_akVersions[eSV_Normal].m_kReflection.m_kInputs.push_back(paramReflection);

        // Pos.x, Pos.y, U, V
        const float QUAD[] =
        {
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
        };

        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.ByteWidth = sizeof(QUAD);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        D3D11_SUBRESOURCE_DATA data;
        ZeroMemory(&data, sizeof(data));
        data.pSysMem = &QUAD;
        m_vertexBuffer = CreateBuffer(desc, &data, &m_context);
        if (!m_vertexBuffer)
        {
            Reset();
            return false;
        }

        ZeroMemory(&desc, sizeof(desc));
        m_UVMatrixCache.SetIdentity();
        desc.ByteWidth = sizeof(m_UVMatrixCache);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        ZeroMemory(&data, sizeof(data));
        data.pSysMem = m_UVMatrixCache.GetData();
        m_constantBuffer = CreateBuffer(desc, &data, &m_context);

        if (!m_constantBuffer)
        {
            Reset();
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC inputDesc;
        inputDesc.SemanticName = s_vertexInput0;
        inputDesc.SemanticIndex = 0;
        inputDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputDesc.InputSlot = 0;
        inputDesc.AlignedByteOffset = 0;
        inputDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        inputDesc.InstanceDataStepRate = 0;
        m_layout = CreateInputLayout(&inputDesc, 1, m_vertexShader.m_akVersions[eSV_Normal].m_kReflection, m_context.m_pDevice);
        if (!m_layout)
        {
            Reset();
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory(&samplerDesc, sizeof(samplerDesc));
        samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        if (!InitializeSamplerState(samplerDesc, m_samplerState, &m_context))
        {
            Reset();
            return false;
        }

        m_minFilterCache = GL_NEAREST;
        m_magFilterCache = GL_NEAREST;

        m_depthStencilState.m_bDepthTestingEnabled = false;
        m_depthStencilState.m_bStencilTestingEnabled = false;
        m_depthStencilState.m_bDepthWriteMask = false;

        m_rasterState.m_bCullingEnabled = false;
        m_rasterState.m_bDepthClipEnabled = true;
        m_rasterState.m_bPolygonOffsetEnabled = false;
        m_rasterState.m_eFrontFaceMode = GL_CCW;
#if !DXGLES
        m_rasterState.m_ePolygonMode = GL_FILL;
        m_rasterState.m_bMultisampleEnabled = false;
        m_rasterState.m_bLineSmoothEnabled = false;
#endif

        m_blendState.m_bAlphaToCoverageEnable = false;
        m_blendState.m_bIndependentBlendEnable = false;
        for (int i = 0; i < DXGL_ARRAY_SIZE(m_blendState.m_kTargets); ++i)
        {
            STargetBlendState& targetBlendState = m_blendState.m_kTargets[i];
            targetBlendState.m_bEnable = false;
            targetBlendState.m_bSeparateAlpha = false;
            for (int j = 0; j < DXGL_ARRAY_SIZE(targetBlendState.m_kWriteMask.m_abRGBA); ++j)
            {
                targetBlendState.m_kWriteMask.m_abRGBA[j] = true;
            }

        }
        m_initialized = true;
        return m_initialized;
    }

    bool GLBlitFramebufferHelper::BlitTexture(SShaderTextureView* srcTexture,
                                                SFrameBufferObject& dstFBO,
                                                GLenum dstColorBuffer,
                                                GLint srcXMin, GLint srcYMin, GLint srcXMax, GLint srcYMax,
                                                GLint dstXMin, GLint dstYMin, GLint dstXMax, GLint dstYMax,
                                                GLenum minFilter, GLenum magFilter)
    {
        PROFILE_LABEL_SCOPE("BlitTexture");
        if (!Initialize())
        {
            AZ_Assert(false, "Failed to initialize GLBlitFrameBufferHelper");
            return false;
        }

        float textWidth = static_cast<float>(srcTexture->m_pTexture->m_iWidth);
        float textHeight = static_cast<float>(srcTexture->m_pTexture->m_iHeight);
        Vec3 scale(1.0f);
        scale.x = (srcXMax - srcXMin) / textWidth;
        scale.y = (srcYMax - srcYMin) / textHeight;

        Vec3 translate(0.f);
        translate.x = srcXMin / textWidth;
        translate.y = srcYMin / textHeight;

        Matrix34 UVMatrix = Matrix34::CreateScale(scale);
        UVMatrix.SetTranslation(translate);
        // Check if we need to update the constant buffer
        if (!Matrix34::IsEquivalent(m_UVMatrixCache, UVMatrix))
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            // Map buffer and discard previous content
            if (!(*m_constantBuffer->m_pfMapSubresource)(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource, &m_context))
            {
                AZ_Assert(false, "Failed to map constant buffer");
                return false;
            }

            memcpy(mappedResource.pData, UVMatrix.GetData(), sizeof(UVMatrix));
            (*m_constantBuffer->m_pfUnmapSubresource)(m_constantBuffer, 0, &m_context);
            m_UVMatrixCache = UVMatrix;
        }

        // Check if we need to update the sampler
        if (m_minFilterCache != minFilter)
        {
            glSamplerParameteri(m_samplerState.m_uSamplerObjectNoMip, GL_TEXTURE_MIN_FILTER, minFilter);
            glSamplerParameteri(m_samplerState.m_uSamplerObjectMip, GL_TEXTURE_MIN_FILTER, minFilter);
        }

        if (m_magFilterCache != magFilter)
        {
            glSamplerParameteri(m_samplerState.m_uSamplerObjectNoMip, GL_TEXTURE_MAG_FILTER, magFilter);
            glSamplerParameteri(m_samplerState.m_uSamplerObjectMip, GL_TEXTURE_MIN_FILTER, minFilter);
        }

        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = static_cast<FLOAT>(dstXMin);
        viewport.TopLeftY = static_cast<FLOAT>(dstYMin);
        viewport.Width = static_cast<FLOAT>(dstXMax - dstXMin);
        viewport.Height = static_cast<FLOAT>(dstYMax - dstYMin);
        viewport.MinDepth = 0.f;
        viewport.MaxDepth = 1.f;

        // Set context states
        m_context.SetViewports(1, &viewport);
        m_context.SetDepthStencilState(m_depthStencilState, 0);
        m_context.SetRasterizerState(m_rasterState);
        m_context.SetBlendState(m_blendState);
        m_context.SetVertexBuffer(0, m_vertexBuffer, sizeof(float) * 4, 0);
        m_context.SetInputLayout(m_layout);
        m_context.SetShader(&m_vertexShader, eST_Vertex);
        m_context.SetShader(&m_fragmentShader, eST_Fragment);
        m_context.SetSampler(&m_samplerState, eST_Fragment, 0);
        m_context.SetShaderTexture(srcTexture, eST_Fragment, 0);
        m_context.SetConstantBuffer(m_constantBuffer, SBufferRange(0, sizeof(UVMatrix)), eST_Vertex, 0);
#if !DXGL_SUPPORT_DRAW_WITH_BASE_VERTEX
        m_context.SetVertexOffset(0);
#endif 

        // Flush context states
        m_context.FlushInputAssemblerState();
        m_context.FlushPipelineState();
        m_context.FlushTextureUnits();
        m_context.FlushUniformBufferUnits();

        // We manually bind the draw framebuffer and enable the correct draw buffer
        m_context.BindDrawFrameBuffer(dstFBO.m_kName);
        SFrameBufferObject::TColorAttachmentMask kDstDrawMask(false);
        if (GL_COLOR_ATTACHMENT0 <= dstColorBuffer && (dstColorBuffer - GL_COLOR_ATTACHMENT0) < SFrameBufferConfiguration::MAX_COLOR_ATTACHMENTS)
        {
            kDstDrawMask.Set(dstColorBuffer - GL_COLOR_ATTACHMENT0, true);
        }

        if (CACHE_VAR(dstFBO.m_kDrawMaskCache, kDstDrawMask))
        {
            glDrawBuffers(1, &dstColorBuffer);
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        return true;
    }

    void GLBlitFramebufferHelper::Reset()
    {
        m_vertexBuffer = nullptr;
        m_constantBuffer = nullptr;
        m_layout = nullptr;
        ResetSamplerState(m_samplerState);
        m_initialized = false;
    }
}
