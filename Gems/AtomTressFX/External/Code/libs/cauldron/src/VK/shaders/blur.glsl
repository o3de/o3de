#version 400

// AMD Cauldron code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (std140, binding = 0) uniform perFrame
{    
    vec2 u_dir;
    int u_mipLevel;
} myPerFrame;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
layout(set=0, binding=1) uniform sampler2D inputSampler;

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

void main()
{
    //int s_lenght = 3; float s_coeffs[] = { 0.382971, 0.241842, 0.060654, }; // norm = 0.987962
    //int s_lenght = 4; float s_coeffs[] = { 0.292360, 0.223596, 0.099952, 0.026085, }; // norm = 0.991625
    //int s_lenght = 5; float s_coeffs[] = { 0.235833, 0.198063, 0.117294, 0.048968, 0.014408, }; // norm = 0.993299
    //int s_lenght = 6; float s_coeffs[] = { 0.197419, 0.174688, 0.121007, 0.065615, 0.027848, 0.009250, }; // norm = 0.994236
    //int s_lenght = 7; float s_coeffs[] = { 0.169680, 0.155018, 0.118191, 0.075202, 0.039930, 0.017692, 0.006541, }; // norm = 0.994827
    //int s_lenght = 8; float s_coeffs[] = { 0.148734, 0.138756, 0.112653, 0.079592, 0.048936, 0.026183, 0.012191, 0.004939, }; // norm = 0.995231
    //int s_lenght = 9; float s_coeffs[] = { 0.132370, 0.125285, 0.106221, 0.080669, 0.054877, 0.033440, 0.018252, 0.008924, 0.003908, }; // norm = 0.995524
    //int s_lenght = 10; float s_coeffs[] = { 0.119237, 0.114032, 0.099737, 0.079779, 0.058361, 0.039045, 0.023890, 0.013368, 0.006841, 0.003201, }; // norm = 0.995746
    //int s_lenght = 11; float s_coeffs[] = { 0.108467, 0.104534, 0.093566, 0.077782, 0.060053, 0.043061, 0.028677, 0.017737, 0.010188, 0.005435, 0.002693, }; // norm = 0.995919
    //int s_lenght = 12; float s_coeffs[] = { 0.099477, 0.096435, 0.087850, 0.075206, 0.060500, 0.045736, 0.032491, 0.021690, 0.013606, 0.008021, 0.004443, 0.002313, }; // norm = 0.996058
    //int s_lenght = 13; float s_coeffs[] = { 0.091860, 0.089459, 0.082622, 0.072368, 0.060113, 0.047355, 0.035378, 0.025065, 0.016842, 0.010732, 0.006486, 0.003717, 0.002020, }; // norm = 0.996172
    //int s_lenght = 14; float s_coeffs[] = { 0.085325, 0.083397, 0.077868, 0.069455, 0.059181, 0.048172, 0.037458, 0.027824, 0.019744, 0.013384, 0.008667, 0.005361, 0.003168, 0.001789, }; // norm = 0.996267
    //int s_lenght = 15; float s_coeffs[] = { 0.079656, 0.078085, 0.073554, 0.066578, 0.057908, 0.048399, 0.038870, 0.029997, 0.022245, 0.015852, 0.010854, 0.007142, 0.004516, 0.002743, 0.001602, }; // norm = 0.996347
    int s_lenght = 16; float s_coeffs[] = { 0.074693, 0.073396, 0.069638, 0.063796, 0.056431, 0.048197, 0.039746, 0.031648, 0.024332, 0.018063, 0.012947, 0.008961, 0.005988, 0.003864, 0.002407, 0.001448, }; // norm = 0.996416

    vec4 accum = s_coeffs[0] * texture( inputSampler, inTexCoord );   
    for(int i=1;i<s_lenght;i++)
    {
        float f= i;
        accum += s_coeffs[i] * texture( inputSampler, inTexCoord + myPerFrame.u_dir*f);
        accum += s_coeffs[i] * texture( inputSampler, inTexCoord - myPerFrame.u_dir*f);
    }

    outColor =  accum;
}
