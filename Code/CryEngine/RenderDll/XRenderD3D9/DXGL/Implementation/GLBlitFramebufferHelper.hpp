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

#pragma once

#include <Implementation/GLCommon.hpp>
#include <Implementation/GLState.hpp>
#include <Implementation/GLResource.hpp>
#include <Implementation/GLShader.hpp>

namespace NCryOpenGL
{
    class CContext;

    // Helper class to blit a texture into a framebuffer using a shader.
    // This should only be used if glBlitFramebuffer is not an option.
    class GLBlitFramebufferHelper
    {
    public:
        explicit GLBlitFramebufferHelper(CContext& context);
        ~GLBlitFramebufferHelper();

        bool BlitTexture(SShaderTextureView* srcTexture,
                        SFrameBufferObject& dstFBO,
                        GLenum dstColorBuffer,
                        GLint srcXMin, GLint srcYMin, GLint srcXMax, GLint srcYMax,
                        GLint dstXMin, GLint dstYMin, GLint dstXMax, GLint dstYMax,
                        GLenum minFilter, GLenum magFilter);

    private:
        bool Initialize();
        void Reset();

        // Shader source code definitions
        static const char* s_uniformBufferName;
        static const char* s_tex0SamplerName;
        static const char* s_vertexInput0;
        static const char* s_blitVertexShader;
        static const char* s_blitFragmentShader;

        bool m_initialized;
        CContext& m_context;
        SShader m_vertexShader;
        SShader m_fragmentShader;
        SBufferPtr m_vertexBuffer;
        SBufferPtr m_constantBuffer;
        SInputLayoutPtr m_layout;
        SSamplerState m_samplerState;
        SDepthStencilState m_depthStencilState;
        SRasterizerState m_rasterState;
        SBlendState m_blendState;

        // Cache variables
        GLenum m_minFilterCache;
        GLenum m_magFilterCache;
        Matrix34 m_UVMatrixCache;
    };
}
