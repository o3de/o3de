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

namespace NCryOpenGL
{
    const char* GLBlitFramebufferHelper::s_uniformBufferName = "vcPER_BATCH";
    const char* GLBlitFramebufferHelper::s_tex0SamplerName   = "text0";
    const char* GLBlitFramebufferHelper::s_vertexInput0      = "Input0";
    
    const char* GLBlitFramebufferHelper::s_blitVertexShader =
                                                            "precision highp float;"
                                                            "layout(std140) uniform;"
                                                            "layout(location = 0) in highp vec4 Input0;"
                                                            "layout(location = 0) out highp vec4 VtxOutput0;"
                                                            "uniform vcPER_BATCH"
                                                            "{"
                                                            "   mat4x3 UVMatrix;"
                                                            "};"
                                                            "void main()"
                                                            "{"
                                                            "   gl_Position = vec4(Input0.xy, 0, 1);"
                                                            "   VtxOutput0 = vec3(Input0.zw, 1.0) * UVMatrix;"
                                                            "}";

    const char* GLBlitFramebufferHelper::s_blitFragmentShader = 
                                                                "precision highp float;"
                                                                "layout(location = 0) in highp vec4 VtxOutput0;"
                                                                "layout(location = 0) out vec4 Output0;"
                                                                "uniform sampler2D text0;"
                                                                "void main()"
                                                                "{"
                                                                "   Output0 = texture(text0, VtxOutput0.xy);"
                                                                "}";
}