/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

//////////////////////////////////COPY SHADERS//////////////////////////////////
namespace NCryMetal
{
NSString *metalCopyShaderSource = @"\n\
#include <metal_stdlib>\n\
    using namespace metal;\n\
    struct metalVert_out\n\
    {\n\
        float4 position [[ position ]];\n\
#define Output0 output.position\n\
        float2 tc [[ user(varying1) ]];\n\
    };\n\
    vertex metalVert_out mainVS(\n\
                                uint VertexID [[ vertex_id ]])\n\
    {\n\
        metalVert_out output;\n\
        float4 position;\n\
        position.x = (VertexID == 0)? 3.0 : -1.0;\n\
        position.y = (VertexID == 2)? 3.0 : -1.0;\n\
        position.zw = 1.0;\n\
        output.position=position;\n\
        output.tc = output.position.xy * float2(0.5, -0.5) + 0.5;\n\
        return output;\n\
    }\n\
    struct metalFrag_stageIn\n\
    {\n\
        float2 tc [[ user(varying1) ]];\n\
    };\n\
    struct metalFrag_out\n\
    {\n\
        float4 PixOutput0 [[ color(0) ]];\n\
    };\n\
    fragment metalFrag_out mainPS(\n\
                                  metalFrag_stageIn stageIn [[ stage_in ]],\n\
                                  sampler Texture0_s[[ sampler(0) ]],\n\
                                  texture2d<float> Texture0_t[[ texture(0) ]])\n\
    {\n\
        metalFrag_out output;\n\
        float2 tc = stageIn.tc;\n\
        output.PixOutput0 = ((Texture0_t.sample(Texture0_s, tc.xy)));\n\
        return output;\n\
    }\n\
    ";
    
NSString *metalCopyShaderSourceLanczos = @"\n\
#include <metal_stdlib>\n\
    using namespace metal;\n\
    struct metalVert_out\n\
    {\n\
        float4 position [[ position ]];\n\
#define Output0 output.position\n\
        float2 tc [[ user(varying1) ]];\n\
    };\n\
    vertex metalVert_out mainVS(\n\
                                uint VertexID [[ vertex_id ]])\n\
    {\n\
        metalVert_out output;\n\
        float4 position;\n\
        position.x = (VertexID == 0)? 3.0 : -1.0;\n\
        position.y = (VertexID == 2)? 3.0 : -1.0;\n\
        position.zw = 1.0;\n\
        output.position=position;\n\
        output.tc = output.position.xy * float2(0.5, -0.5) + 0.5;\n\
        return output;\n\
    }\n\
    struct metalFrag_stageIn\n\
    {\n\
        float2 tc [[ user(varying1) ]];\n\
    };\n\
    struct metalFrag_out\n\
    {\n\
        float4 PixOutput0 [[ color(0) ]];\n\
    };\n\
    struct Uniforms\n\
    {\n\
        float4 KernelRadius_ClippedRatio;\n\
        float4 SampleSize_FirstSampleOffset;\n\
        float4 SampleStep_FirstSamplePos;\n\
    };\n\
    fragment metalFrag_out mainLanczosPS(\n\
                                         metalFrag_stageIn stageIn [[ stage_in ]],\n\
                                         sampler Texture0_s[[ sampler(0) ]],\n\
                                         texture2d<float> Texture0_t[[ texture(0) ]],\n\
                                         constant Uniforms &uniforms [[buffer(0)]])\n\
    {\n\
        metalFrag_out output;\n\
        float2 tc = stageIn.tc;\n\
        float2 baseTc = tc + uniforms.SampleSize_FirstSampleOffset.zw;\n\
        float2 kernelPos = uniforms.SampleStep_FirstSamplePos.zw;\n\
        float4 accumSample = float4(0,0,0,0);\n\
        float accumWeight = 0;\n\
        for (; kernelPos.y < uniforms.KernelRadius_ClippedRatio.y; kernelPos.y += uniforms.SampleStep_FirstSamplePos.y)\n\
        {\n\
            tc.x = baseTc.x;\n\
            for (kernelPos.x = uniforms.SampleStep_FirstSamplePos.x;\n\
                 kernelPos.x < uniforms.KernelRadius_ClippedRatio.x;\n\
                 kernelPos.x += uniforms.SampleStep_FirstSamplePos.x)\n\
            {\n\
                float2 piProduct = 3.14159265f * (kernelPos + 1e-4f);\n\
                float2 weights2d = (sin(piProduct) * sin(piProduct * 0.5f) * 2.f) / (piProduct * piProduct);\n\
                float weight = weights2d.x * weights2d.y;\n\
                accumSample += weight * ((Texture0_t.sample(Texture0_s, tc.xy * uniforms.KernelRadius_ClippedRatio.zw)));\n\
                accumWeight += weight;\n\
                tc.x += uniforms.SampleSize_FirstSampleOffset.x;\n\
            }\n\
            tc.y += uniforms.SampleSize_FirstSampleOffset.y;\n\
        }\n\
        output.PixOutput0 = accumSample / accumWeight;\n\
        return output;\n\
    }\n\
    fragment metalFrag_out mainBicubicPS(\n\
                                         metalFrag_stageIn stageIn [[ stage_in ]],\n\
                                         sampler Texture0_s[[ sampler(0) ]],\n\
                                         texture2d<float> Texture0_t[[ texture(0) ]])\n\
    {\n\
        metalFrag_out output;\n\
        int width = Texture0_t.get_width();\n\
        int height = Texture0_t.get_height();\n\
        float2 texSize = float2((float)width, (float)height);\n\
        float2 unnormedTc = stageIn.tc * texSize;\n\
        float2 unnormedCenteredTc = floor( unnormedTc - 0.5f ) + 0.5f;\n\
        float2 f = unnormedTc - unnormedCenteredTc;\n\
        float2 f2 = f*f;\n\
        float2 f3 = f*f2;\n\
        float2 w0 = f2 - 0.5f * (f3 + f);\n\
        float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;\n\
        float2 w2 = -1.5 * f3 + 2 * f2 + 0.5 *f;\n\
        float2 w3 = 0.5 * (f3 - f2);\n\
        float2 s0 = w0 + w1;\n\
        float2 s1 = w2 + w3;\n\
        float2 f0 = w1 / (w0 + w1);\n\
        float2 f1 = w3 / (w2 + w3);\n\
        float2 t0 = (unnormedCenteredTc - 1.f + f0) / texSize;\n\
        float2 t1 = (unnormedCenteredTc + 1.f + f1) / texSize;\n\
        output.PixOutput0  = ( (Texture0_t.sample(Texture0_s, float2(t0.x, t0.y))) * s0.x \n\
                              +   (Texture0_t.sample(Texture0_s, float2(t1.x, t0.y))) * s1.x) * s0.y\n\
        + ( (Texture0_t.sample(Texture0_s, float2(t0.x, t1.y))) * s0.x \n\
           +   (Texture0_t.sample(Texture0_s, float2(t1.x, t1.y))) * s1.x) * s1.y;\n\
        return output;\n\
    }\n\
    ";
    // Bicubic filtering based off Phill Djonov's blog: http://vec3.ca/bicubic-filtering-in-fewer-taps/
    ////////////////////////////////////////////////////////////////////////////////
}

