//////////////////////////////////////////////////////////////
//simple Duffuse bump - adapted from the nVidia cgFX samples
//This sample supports Render To Texture and is setup to use
//Mapping channel 3 - it just needs to be uncommented.
//////////////////////////////////////////////////////////////

float4x4 worldMatrix : 		World;	// World or Model matrix
float4x4 WorldIMatrix : 	WorldI;	// World Inverse or Model Inverse matrix
float4x4 mvpMatrix : 		WorldViewProj;	// Model*View*Projection
float4x4 worldViewMatrix : 	WorldView;
float4x4 viewInverseMatrix :	ViewI;

// tweakables

int texcoord0 : Texcoord
<
	int Texcoord = 0;
	int MapChannel = 1;
	string UIWidget = "None";
>;

int texcoord1 : Texcoord
<
	int Texcoord = 1;
	int MapChannel = 0;
	string UIWidget = "None";
>;

int texcoord2 : Texcoord
<
	int Texcoord = 2;
	int MapChannel = -2;
	string UIWidget = "None";
>;

texture diffuseTexture1 : DiffuseMap1 < 
	string UIName = "DiffuseMap 1";
	int Texcoord = 0;
	>;
	
texture diffuseTexture2 : DiffuseMap2 < 
	string UIName = "DiffuseMap 2";
	int Texcoord = 0;
	>;
	
texture normalMap : NormalMap < 
	string UIName = "NormalMap";
	int Texcoord = 0;
	>;

bool useLight : UseLight <
	string UIName = "Use Light";
	> = 0;

float4 lightDir : Direction <  
	string UIName = "Light Direction"; 
	string Object = "TargetLight";
	int RefID = 0;
	> = {-0.577, -0.577, 0.577,1.0};

float3 lightCol : LIGHTCOLOR <
	string UIName = "Light Color";
	string Object = "TargetLight";
	int RefID = 0;
	> = {1.0, 1.0, 1.0};

float3 dirtTint : DirtTint <
	string UIName = "Dirt Tint";
	> = {1.0, 1.0, 1.0};
	
float dirtStrength : DirtStrength <
	string UIName = "Dirt Strength";
	float UIMin = 0.0f;
	float UIMax = 1.0f;
	float UIStep = 0.01f;
	> = 1.0f;

float dirtMapAlpha : DirtMapAlpha <
	string UIName = "Dirt Map Alpha";
	float UIMin = 0.0f;
	float UIMax = 1.0f;
	float UIStep = 0.01f;
	> = 1.0f;

float dirtTile : DirtTile <
	string UIName = "Dirt Tiling";
	float UIMin = 0.0f;
	float UIMax = 16.0f;
	float UIStep = 0.01f;
	> = 1.0f;

struct a2v {
	float4 Position : POSITION; //in object space
	float2 UV : TEXCOORD0; // in uv coords
	float3 vCol : TEXCOORD1; // in Vertex alpha
	float3 Alpha : TEXCOORD2; // in Vertex alpha
	float3 Normal : NORMAL; //in object space
	float3 T : TANGENT; //in object space
	float3 B : BINORMAL; //in object space
};

struct v2f {
	float4 Position : POSITION; //in projection space
	float4 uv : TEXCOORD0;
	float4 vCol : TEXCOORD1;
	float3 LightVector : TEXCOORD2;
	float3 Normal: TEXCOORD3;
	float3 LightCol : TEXCOORD4;
};

struct f2fb {
	float4 col : COLOR;
};

v2f DiffuseBumpVS(a2v IN,
		uniform float4x4 WorldViewProj,
        	uniform float4x4 WorldIMatrix,
		uniform float4 LightDir,
		uniform float3 LightCol)
{
	v2f OUT;

	// pass texture coordinates for fetching the diffuse1 map
	float2 uv = IN.UV;
	float2 dirtUV = uv * dirtTile;
	dirtUV.y -= 1.0f;
	
	OUT.uv = float4(uv, dirtUV);


	OUT.vCol = float4(IN.vCol, IN.Alpha.x);

	//OUT.vCol = float4(0,0,0,IN.Alpha.x);

	// compute the 3x3 tranform from tangent space to object space
	float3x3 objToTangentSpace;
	objToTangentSpace[0] = IN.B;
	objToTangentSpace[1] = IN.T;
	objToTangentSpace[2] = IN.Normal;

	// transform normal from object space to tangent space and pass it as a color
	OUT.Normal.xyz = 0.5 * mul(IN.Normal,objToTangentSpace) + 0.5;

	float4 vertnormLightVec = normalize(LightDir);

	// transform light vector from object space to tangent space and pass it as a color 
	OUT.LightVector.xyz = mul(objToTangentSpace, vertnormLightVec.xyz );
	OUT.LightCol = LightCol;

	// transform position to projection space
	OUT.Position = mul(IN.Position,WorldViewProj).xyzw;

	return OUT;

}

f2fb DiffuseBumpPS(v2f IN,
		uniform sampler2D DiffuseMap1,
		uniform sampler2D DiffuseMap2,
		uniform sampler2D NormalMap) 
{
	f2fb OUT;

	//fetch base color1
	float4 color = tex2D(DiffuseMap1,IN.uv.xy );

	//fetch base color2
	float4 color2 = tex2D(DiffuseMap2,IN.uv.zw );

	//color = lerp(color, color2, (1-IN.vCol.w) * lerp(1, color2.a, dirtAlpha));
	
	float blendFac = lerp(1.0f, color2.a, dirtMapAlpha) * dirtStrength * (1.0f - IN.vCol.w);

	color.rgb = lerp(color.rgb, color2.rgb * dirtTint, blendFac);
	
	//compute final color
	OUT.col = color * float4(IN.vCol.xyz, 1);

	if (useLight)
	{
		//fetch bump normal

		float3 bumpNormal = 2 * (tex2D(NormalMap,IN.uv.xy).xyz-0.5);

		//expand iterated light vector to [-1,1]
		float3 lightVector = 2 * (IN.LightVector - 0.5 );
		lightVector = normalize(lightVector);
		float3 bump = dot(bumpNormal,lightVector.xyz);
		OUT.col *= float4(bump,1) * float4(IN.LightCol,1);
	}
	
	OUT.col.a = 1.0;
	
	return OUT;
}

sampler2D diffuseSampler1 = sampler_state
{
	Texture = <diffuseTexture1>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler2D diffuseSampler2 = sampler_state
{
	Texture = <diffuseTexture2>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

sampler2D normalSampler = sampler_state 
{
	Texture = <normalMap>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
};

//////// techniques ////////////////////////////

technique Default
{

	pass p0
	{
		ZEnable = true;
		ZWriteEnable = true;
		CullMode = None;
		VertexShader = compile vs_1_1 DiffuseBumpVS(mvpMatrix,WorldIMatrix,lightDir,lightCol);
        	PixelShader = compile ps_2_0 DiffuseBumpPS(diffuseSampler1,diffuseSampler2,normalSampler);
	}
}

