#version 400

// MIT License
// Copyright (c) 2011 Simon Wallner
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions 
// of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// 

// Based on "A Practical Analytic Model for Daylight"
// aka The Preetham Model, the de facto standard analytic skydome model
// http://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (std140, binding = 0) uniform perFrame
{
    mat4 u_mClipToWord;
    vec3 vSunDirection;
    float padding;
    float rayleigh;
    float turbidity;
    float mieCoefficient;
    
    float luminance;
    float mieDirectionalG;
} myPerFrame;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------


//const vec3 up = vec3( 0.0, 1.0, 0.0 );

// constants for atmospheric scattering
const float e = 2.71828182845904523536028747135266249775724709369995957;
//const float pi = 3.141592653589793238462643383279502884197169;

// wavelength of used primaries, according to preetham
const vec3 lambda = vec3( 680E-9, 550E-9, 450E-9 );
// this pre-calcuation replaces older TotalRayleigh(vec3 lambda) function:
// (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, vec3(4.0)) * (6.0 - 7.0 * pn))
const vec3 totalRayleigh = vec3( 5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5 );

// mie stuff
// K coefficient for the primaries
const float v = 4.0;
const vec3 K = vec3( 0.686, 0.678, 0.666 );
// MieConst = pi * pow( ( 2.0 * pi ) / lambda, vec3( v - 2.0 ) ) * K
const vec3 MieConst = vec3( 1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14 );

// earth shadow hack
// cutoffAngle = pi / 1.95;
const float cutoffAngle = 1.6110731556870734;
const float steepness = 1.5;
const float EE = 1000.0;


/////////////////

const vec3 cameraPos = vec3( 0.0, 0.0, 0.0 );

    // constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

const float n = 1.0003; // refractive index of air
const float N = 2.545E25; // number of molecules per unit volume for air at
                                // 288.15K and 1013mb (sea level -45 celsius)

    // optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const vec3 up = vec3( 0.0, 1.0, 0.0 );
    // 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

    // 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
    // 1.0 / ( 4.0 * pi )
const float ONE_OVER_FOURPI = 0.07957747154594767;

//--------------------------------------------------------------------------------------
// Code 
//--------------------------------------------------------------------------------------

float sunIntensity( float zenithAngleCos ) {
    zenithAngleCos = clamp( zenithAngleCos, -1.0, 1.0 );
    return EE * max( 0.0, 1.0 - pow( e, -( ( cutoffAngle - acos( zenithAngleCos ) ) / steepness ) ) );
}

vec3 totalMie( float T ) {
    float c = ( 0.2 * T ) * 10E-18;
    return 0.434 * c * MieConst;
}

float rayleighPhase( float cosTheta ) {
    return THREE_OVER_SIXTEENPI * ( 1.0 + pow( cosTheta, 2.0 ) );
}

float hgPhase( float cosTheta, float g ) {
    float g2 = pow( g, 2.0 );
    float inverse = 1.0 / pow( 1.0 - 2.0 * g * cosTheta + g2, 1.5 );
    return ONE_OVER_FOURPI * ( ( 1.0 - g2 ) * inverse );
}

void main() {

    vec4 clip = vec4(2 * inTexCoord.x - 1, 1 - 2 * inTexCoord.y, 1, 1);
    vec3 vWorldPosition = (myPerFrame.u_mClipToWord * clip).xyz;

    //this can be done in a VS

    float vSunE = sunIntensity( dot( myPerFrame.vSunDirection, up ) );

    float vSunfade = 1.0 - clamp( 1.0 - exp( ( myPerFrame.vSunDirection.y ) ), 0.0, 1.0 );

    float rayleighCoefficient = myPerFrame.rayleigh - ( 1.0 * ( 1.0 - vSunfade ) );

    // extinction (absorbtion + out scattering)
    // rayleigh coefficients
    vec3 vBetaR = totalRayleigh * rayleighCoefficient;

    // mie coefficients
    vec3 vBetaM = totalMie( myPerFrame.turbidity ) * myPerFrame.mieCoefficient;


    // PS stuff

    // optical length
    // cutoff angle at 90 to avoid singularity in next formula.
    float zenithAngle = acos( max( 0.0, dot( up, normalize( vWorldPosition - cameraPos ) ) ) );
    float inverse = 1.0 / ( cos( zenithAngle ) + 0.15 * pow( 93.885 - ( ( zenithAngle * 180.0 ) / pi ), -1.253 ) );
    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;

    // combined extinction factor
    vec3 Fex = exp( -( vBetaR * sR + vBetaM * sM ) );

    // in scattering
    float cosTheta = dot( normalize( vWorldPosition - cameraPos ), myPerFrame.vSunDirection );

    float rPhase = rayleighPhase( cosTheta * 0.5 + 0.5 );
    vec3 betaRTheta = vBetaR * rPhase;

    float mPhase = hgPhase( cosTheta, myPerFrame.mieDirectionalG );
    vec3 betaMTheta = vBetaM * mPhase;

    vec3 Lin = pow( vSunE * ( ( betaRTheta + betaMTheta ) / ( vBetaR + vBetaM ) ) * ( 1.0 - Fex ), vec3( 1.5 ) );
    Lin *= mix( vec3( 1.0 ), pow( vSunE * ( ( betaRTheta + betaMTheta ) / ( vBetaR + vBetaM ) ) * Fex, vec3( 1.0 / 2.0 ) ), clamp( pow( 1.0 - dot( up, myPerFrame.vSunDirection ), 5.0 ), 0.0, 1.0 ) );

    // nightsky
    vec3 direction = normalize( vWorldPosition - cameraPos );
    float theta = acos( direction.y ); // elevation --> y-axis, [-pi/2, pi/2]',
    float phi = atan( direction.z, direction.x ); // azimuth --> x-axis [-pi/2, pi/2]',
    vec2 uv = vec2( phi, theta ) / vec2( 2.0 * pi, pi ) + vec2( 0.5, 0.0 );
    vec3 L0 = vec3( 0.1 ) * Fex;

    // composition + solar disc
    float sundisk = smoothstep( sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta );
    L0 += ( vSunE * 19000.0 * Fex ) * sundisk;

    vec3 texColor = ( Lin + L0 ) * 0.04 + vec3( 0.0, 0.0003, 0.00075 );

    //no tonemapped
    vec3 color = ( log2( 2.0 / pow( myPerFrame.luminance, 4.0 ) ) ) * texColor;

    vec3 retColor = pow( color, vec3( 1.0 / ( 1.2 + ( 1.2 * vSunfade ) ) ) );

    outColor = vec4( retColor, 1.0 );
}

