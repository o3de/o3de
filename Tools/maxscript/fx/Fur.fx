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

float4x4 WorldMatrix        : World;
float4x4 ViewInverseMatrix  : ViewI;
float4x4 ViewMatrix         : View;
float4x4 ProjMatrix         : Projection;

int texcoord0 : Texcoord // UVs
<
    int Texcoord = 0;
    int MapChannel = 1;
    string UIWidget = "None";
>;

int texcoord1 : Texcoord // Vertex color
<
    int Texcoord = 1;
    int MapChannel = 0;
    string UIWidget = "None";
>;

int texcoord2 : Texcoord // Vertex alpha
<
    int Texcoord = 2;
    int MapChannel = -2;
    string UIWidget = "None";
>;

float4 PointLight0Pos : Position
<
    string UIName =  "Point Lamp 0 Position";
    string Object = "PointLight";
    string Space = "World";
    string UIGroup =  "Lighting";
> = {-54.0f, 50.0f, 100.0f, 1.0f};

float3 PointLight0Col : Diffuse
<
    string UIName =  "Point Lamp 0 Color";
    string UIWidget = "Color";
    string Object = "PointLight";
    string UIGroup =  "Lighting";
> = {1.0f, 0.95f, 0.85f};

float3 AmbientColor : Ambient
<
    string UIName =  "Ambient Light";
    string UIWidget = "Color";
    string UIGroup =  "Lighting";
> = {0.07f, 0.07f, 0.07f};

// Surface attributes
float Kd
<
    string UIName = "Diffuse Brightness";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 1.0;
    float UIStep = 0.05;
    string UIGroup =  "Diffuse";
> = 1.0;

float Ks
<
    string UIName = "Specular Brightness";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 1.0;
    float UIStep = 0.05;
    string UIGroup =  "Lighting";
> = 0.6;

float SpecularExponent : SpecularPower
<
    string UIName = "Specular Power";
    string UIWidget = "Slider";
    float UIMin = 1.0;
    float UIMax = 128.0;
    float UIStep = 1.0;
    string UIGroup =  "Lighting";
> = 25.0;

// Fur parameters and textures
float FurLengthCM
<
    string UIName = "Fur Length (cm)";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 100.0;
    float UIStep = 0.1;
> = 10.0;

float FurMaxGravity
<
    string UIName = "Fur Maximum Gravity";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 5.0;
    float UIStep = 0.01;
> = 2.0;

float FurStiffness
<
    string UIName = "Fur Stiffness";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 1.0;
    float UIStep = 0.001;
> = 1.0;

float FurDensity
<
    string UIName = "Fur Density";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0.0;
    float UIMax = 25.0;
    float UIStep = 0.1;
> = 5.0;

float FurBaseIntrusion 
<
    string UIName = "Fur Base Intrusion";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0;
    float UIMax = 1;
    float UIStep = 0.001;
> = 0.0;

float FurCombBias 
<
    string UIName = "Fur Combing Bias";
    string UIGroup = "Fur Parameters";
    string UIWidget = "Slider";
    float UIMin = 0;
    float UIMax = 5;
    float UIStep = 0.01;
> = 1.0;

bool UseFurCombing
<
    string UIName = "Enable Fur Combing";
    string UIGroup = "Fur Parameters";
> = true;

bool UseSecondDiffuse
<
    string UIName = "Use Base Fur Diffuse Map";
    string UIGroup = "Fur Parameters";
> = true;

texture ColorTexture
<
    string ResourceType = "2D";
    string UIName = "Color Texture";
    string UIGroup = "Diffuse";
>;

sampler2D ColorSampler = sampler_state
{
    Texture = <ColorTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

texture BaseColorTexture
<
    string ResourceType = "2D";
    string UIName = "Fur Tip Color Texture";
    string UIGroup = "Diffuse";
>;

sampler2D BaseColorSampler = sampler_state
{
    Texture = <BaseColorTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

texture FurHeightmap
<
    string ResourceType = "2D";
    string UIName = "Fur Heightmap";
    string UIGroup = "Fur Parameters";
>;

sampler2D FurHeightmapSampler = sampler_state
{
    Texture = <FurHeightmap>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

struct appdata
{
    float3 Position : POSITION;
    float2 baseTC   : TEXCOORD0;
    float3 Color    : TEXCOORD1;
    float3 Alpha    : TEXCOORD2;
    float3 Normal   : NORMAL;
};

struct v2f
{
    float4 HPosition   : POSITION;
    float4 baseTC      : TEXCOORD0;
    float3 vView       : TEXCOORD1;
    half furLength     : TEXCOORD2;
    float3 LightVec    : TEXCOORD3;
    float3 WorldNormal : TEXCOORD4;
};

float GetFurLength()
{
    // Convert from centimeters to inches
    return FurLengthCM * 0.393701;
}

void ComputeFurCombing(inout float3 vFurComb, float4x4 ObjToWorldMatrix)
{
    // Assumes Z-up coordinates
    vFurComb.xyz = 2 * vFurComb.xyz - 1.0;
    if (any(step(0.02, abs(vFurComb)))) // Using slop of 0.02, so that 128 / 255 is applied as 0
    {
        vFurComb = mul(float4(normalize(vFurComb), 0), ObjToWorldMatrix).xyz;
        vFurComb = normalize(vFurComb);
    }
    else
    {
        vFurComb = float3(0, 0, 0);
    }
}

void ModifyVertexForShell(inout float4 vPosWorld, float3 vNormWorld, float4x4 ObjToWorldMatrix, half shellDistance, float3 vFurComb, float furLengthScale)
{
    // Stiffness attenuation is quadratic so that the fur tips can bend a lot while the roots move less
    half stiffness = (1.0 - FurStiffness);
    stiffness *= stiffness;
    stiffness *= shellDistance;

    float3 vFurNormal = vNormWorld;

    // Intrude base fur position by a percentage of fur length
    vPosWorld.xyz -= vNormWorld * FurBaseIntrusion * furLengthScale * GetFurLength();

    if (FurStiffness < 1.0 && FurMaxGravity > 0.0) // Apply gravity for this shell
    {
        float3 vGravityWorld = float3(0, 0, stiffness * -FurMaxGravity);

        float nDotGrav = dot(vFurNormal, vGravityWorld);

        // Compute rejection of G on N: W - N * (G dot N) / (N dot N)
        float3 gravProj = vGravityWorld - nDotGrav * vFurNormal;

        vFurNormal = normalize(vFurNormal + gravProj);
    }

    if (UseFurCombing)
    {
        ComputeFurCombing(vFurComb, ObjToWorldMatrix);

        // Restrict fur combing to the hemisphere pointing in the normal direction
        float nDotComb = dot(vFurNormal, vFurComb);
        if (nDotComb < 0)
        {
            float3 vNCrossComb = cross(vFurNormal, vFurComb);
            float3 vOrthoN = cross(vNCrossComb, vFurNormal);
            vFurComb = normalize(vOrthoN);
        }

        vFurComb = normalize(vFurComb + vFurNormal);
    }
    else
    {
        vFurComb = vFurNormal;
    }

    float3 vOffset = lerp(vFurComb, vFurNormal, 1 - pow(shellDistance, FurCombBias));
    vOffset *= GetFurLength() * furLengthScale * shellDistance;

    vPosWorld.xyz += vOffset;
}

v2f FurVS(appdata IN, uniform half shellProgress) 
{
    v2f OUT = (v2f)0;
    float3 normal = normalize(IN.Normal);
    OUT.WorldNormal = mul(float4(normal, 0), WorldMatrix).xyz;
    OUT.WorldNormal = normalize(OUT.WorldNormal);

    float4 posWorld = mul(float4(IN.Position, 1), WorldMatrix);

    ModifyVertexForShell(posWorld, OUT.WorldNormal, WorldMatrix, shellProgress, IN.Color, IN.Alpha.x);

    OUT.LightVec = normalize(PointLight0Pos.xyz - posWorld.xyz);
    OUT.baseTC = float4(IN.baseTC, 0, 0);
    OUT.vView.xyz = normalize(float3(ViewInverseMatrix[0].w, ViewInverseMatrix[1].w, ViewInverseMatrix[2].w) - posWorld.xyz);

    OUT.HPosition = mul(mul(posWorld, ViewMatrix), ProjMatrix);
    OUT.furLength = shellProgress * GetFurLength() * IN.Alpha.x;

    return OUT;
}

float4 FurPS(v2f IN, uniform half shellProgress) : COLOR 
{
    // Clip shells that are too close to the base surface, but ensure that the base pass draws fully
    clip((IN.furLength - 0.001) * sign(shellProgress));

    half furHeight = max(0.01, tex2D(FurHeightmapSampler, IN.baseTC.xy * FurDensity).r);
    half strandPercent = smoothstep(0.0, furHeight, shellProgress);

    float3 baseColor = tex2D(BaseColorSampler, IN.baseTC.xy).rgb;
    float3 tipColor = tex2D(ColorSampler, IN.baseTC.xy).rgb;
    float3 diffuseColor = lerp(baseColor, tipColor, strandPercent);
    if (!UseSecondDiffuse)
    {
        diffuseColor = tipColor;
    }

    float3 Nn = IN.WorldNormal;
    float3 Vn = IN.vView;
    float3 Ln = IN.LightVec;
    float3 Hn = normalize(Vn + Ln);

    float4 litV = lit(dot(Ln, Nn), dot(Hn, Nn), SpecularExponent);
    float3 diffContrib = (litV.y * Kd) * PointLight0Col * diffuseColor;
    float3 specContrib = (Ks * litV.z) * PointLight0Col;

    float3 result = (diffContrib + specContrib);
    result += (AmbientColor * diffuseColor);

    half scaledFurLength = 1.0 - strandPercent * strandPercent;
    return float4(baseColor, scaledFurLength);
}

#define furPass(shellProgress, blendEnable) VertexShader = compile vs_3_0 FurVS(shellProgress); \
        PixelShader = compile ps_3_0 FurPS(shellProgress); \
        AlphaBlendEnable = blendEnable; \
        SrcBlend = srcalpha; \
        DestBlend = invsrcalpha

technique FurLod0
{
    pass pass00 { furPass(0.0, false); }

    pass pass01 { furPass(0.015625, true); }
    pass pass02 { furPass(0.031250, true); }
    pass pass03 { furPass(0.046875, true); }
    pass pass04 { furPass(0.062500, true); }
    pass pass05 { furPass(0.078125, true); }
    pass pass06 { furPass(0.093750, true); }
    pass pass07 { furPass(0.109375, true); }
    pass pass08 { furPass(0.125000, true); }
    pass pass09 { furPass(0.140625, true); }
    pass pass10 { furPass(0.156250, true); }
    pass pass11 { furPass(0.171875, true); }
    pass pass12 { furPass(0.187500, true); }
    pass pass13 { furPass(0.203125, true); }
    pass pass14 { furPass(0.218750, true); }
    pass pass15 { furPass(0.234375, true); }
    pass pass16 { furPass(0.250000, true); }
    pass pass17 { furPass(0.265625, true); }
    pass pass18 { furPass(0.281250, true); }
    pass pass19 { furPass(0.296875, true); }
    pass pass20 { furPass(0.312500, true); }
    pass pass21 { furPass(0.328125, true); }
    pass pass22 { furPass(0.343750, true); }
    pass pass23 { furPass(0.359375, true); }
    pass pass24 { furPass(0.375000, true); }
    pass pass25 { furPass(0.390625, true); }
    pass pass26 { furPass(0.406250, true); }
    pass pass27 { furPass(0.421875, true); }
    pass pass28 { furPass(0.437500, true); }
    pass pass29 { furPass(0.453125, true); }
    pass pass30 { furPass(0.468750, true); }
    pass pass31 { furPass(0.484375, true); }
    pass pass32 { furPass(0.500000, true); }
    pass pass33 { furPass(0.515625, true); }
    pass pass34 { furPass(0.531250, true); }
    pass pass35 { furPass(0.546875, true); }
    pass pass36 { furPass(0.562500, true); }
    pass pass37 { furPass(0.578125, true); }
    pass pass38 { furPass(0.593750, true); }
    pass pass39 { furPass(0.609375, true); }
    pass pass40 { furPass(0.625000, true); }
    pass pass41 { furPass(0.640625, true); }
    pass pass42 { furPass(0.656250, true); }
    pass pass43 { furPass(0.671875, true); }
    pass pass44 { furPass(0.687500, true); }
    pass pass45 { furPass(0.703125, true); }
    pass pass46 { furPass(0.718750, true); }
    pass pass47 { furPass(0.734375, true); }
    pass pass48 { furPass(0.750000, true); }
    pass pass49 { furPass(0.765625, true); }
    pass pass50 { furPass(0.781250, true); }
    pass pass51 { furPass(0.796875, true); }
    pass pass52 { furPass(0.812500, true); }
    pass pass53 { furPass(0.828125, true); }
    pass pass54 { furPass(0.843750, true); }
    pass pass55 { furPass(0.859375, true); }
    pass pass56 { furPass(0.875000, true); }
    pass pass57 { furPass(0.890625, true); }
    pass pass58 { furPass(0.906250, true); }
    pass pass59 { furPass(0.921875, true); }
    pass pass60 { furPass(0.937500, true); }
    pass pass61 { furPass(0.953125, true); }
    pass pass62 { furPass(0.968750, true); }
    pass pass63 { furPass(0.984375, true); }
    pass pass64 { furPass(1.000000, true); }
}

technique FurLod1
{
    pass pass00 { furPass(0.0, false); }

    pass pass01 { furPass(0.031250, true); }
    pass pass02 { furPass(0.062500, true); }
    pass pass03 { furPass(0.093750, true); }
    pass pass04 { furPass(0.125000, true); }
    pass pass05 { furPass(0.156250, true); }
    pass pass06 { furPass(0.187500, true); }
    pass pass07 { furPass(0.218750, true); }
    pass pass08 { furPass(0.250000, true); }
    pass pass09 { furPass(0.281250, true); }
    pass pass10 { furPass(0.312500, true); }
    pass pass11 { furPass(0.343750, true); }
    pass pass12 { furPass(0.375000, true); }
    pass pass13 { furPass(0.406250, true); }
    pass pass14 { furPass(0.437500, true); }
    pass pass15 { furPass(0.468750, true); }
    pass pass16 { furPass(0.500000, true); }
    pass pass17 { furPass(0.531250, true); }
    pass pass18 { furPass(0.562500, true); }
    pass pass19 { furPass(0.593750, true); }
    pass pass20 { furPass(0.625000, true); }
    pass pass21 { furPass(0.656250, true); }
    pass pass22 { furPass(0.687500, true); }
    pass pass23 { furPass(0.718750, true); }
    pass pass24 { furPass(0.750000, true); }
    pass pass25 { furPass(0.781250, true); }
    pass pass26 { furPass(0.812500, true); }
    pass pass27 { furPass(0.843750, true); }
    pass pass28 { furPass(0.875000, true); }
    pass pass29 { furPass(0.906250, true); }
    pass pass30 { furPass(0.937500, true); }
    pass pass31 { furPass(0.968750, true); }
    pass pass32 { furPass(1.000000, true); }
}

technique FurLod2
{
    pass pass00 { furPass(0.0, false); }

    pass pass01 { furPass(0.062500, true); }
    pass pass02 { furPass(0.125000, true); }
    pass pass03 { furPass(0.187500, true); }
    pass pass04 { furPass(0.250000, true); }
    pass pass05 { furPass(0.312500, true); }
    pass pass06 { furPass(0.375000, true); }
    pass pass07 { furPass(0.437500, true); }
    pass pass08 { furPass(0.500000, true); }
    pass pass09 { furPass(0.562500, true); }
    pass pass10 { furPass(0.625000, true); }
    pass pass11 { furPass(0.687500, true); }
    pass pass12 { furPass(0.750000, true); }
    pass pass13 { furPass(0.812500, true); }
    pass pass14 { furPass(0.875000, true); }
    pass pass15 { furPass(0.937500, true); }
    pass pass16 { furPass(1.000000, true); }
}