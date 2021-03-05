uniform mat4 matViewProj;
uniform mat4 matView;
uniform mat4 matWorld;
uniform mat4 matWorldIT;
uniform mat4 matWorldView;
uniform mat4 matWorldViewProj;
uniform mat4 invMatView;

uniform vec3 camForward;
uniform vec4 diffuseColor;
varying vec4 outDiffuse;
varying vec3 outPosition;
varying float outDepth;

varying vec3 outNormal;
varying vec3 outTangent;
varying vec3 outBinormal;

uniform vec3 mainLightDir;

attribute vec3 inPosition;

#ifdef SKINNING
	attribute vec4 inWeights;
	attribute vec4 inIndices;
	uniform mat4 matBones[200];
#endif


	attribute vec3 inNormal;
	attribute vec4 inTangent;
	varying   vec3 outHalfAngle;
	
	uniform vec3 eyePoint;
	uniform vec3 rimLightDir;

	uniform vec4 skyColor;
	uniform vec4 groundColor;

	attribute vec2 inUV;

#ifdef SHADOWS
	varying vec4 outShadowUV;
	uniform mat4 matLightViewProj;
#endif



void main()
{
	vec4 position = vec4(0.0);
	vec3 normal   = vec3(0.0);
	vec3 tangent  = vec3(0.0);

	gl_TexCoord[0].xy = inUV;
	
#ifdef SKINNING
	for (int i=0; i<4; i++)
	{
		int index = int(inIndices[i]);
		position += (vec4(inPosition, 1.0) * matBones[index]) * inWeights[i];
		normal   += ((vec4(inNormal, 0.0) * matBones[index]) * inWeights[i]).xyz;
		tangent  += ((vec4(inTangent.xyz, 0.0) * matBones[index]) * inWeights[i]).xyz;
	}
	
	normal = (vec4(normal, 0.0) * matWorldIT).xyz;
	position.w = 1.0;
	
#else
	position = vec4(inPosition, 1.0);	
	normal = (vec4(inNormal, 0.0) * matWorldIT).xyz;
	tangent = inTangent.xyz;
#endif	
	
	//position = position * matWorld;
	position.w = 1.0;
	gl_Position = position * matWorldViewProj;
	outDiffuse = diffuseColor;
	
#ifdef LIGHTING
	normal		= normalize( normal );
	
	if (length(tangent) > 0.0001)
		tangent	= normalize( tangent );
	else
		tangent	= vec3(1,0,0);
	
//	tangent		= normalize( vec4(tangent, 0.0) * matWorldIT ).xyz;
	
    vec3 binormal	= cross(normal, tangent) * inTangent.w;                                                   				
	
	outNormal	= normal.xyz;
	outTangent	= tangent.xyz;
	outBinormal	= binormal.xyz;
	
	vec3 E = normalize( eyePoint - position.xyz );
	vec3 L = normalize( -mainLightDir ); 	// light vector
	vec3 H = normalize(E + L);				// half angle vector	
	outHalfAngle = H;
#else
	outBinormal	= vec3(0.0);
	outTangent	= vec3(0.0);
	outNormal	= vec3(0.0);
	outHalfAngle	= vec3(0.0);
#endif

#ifdef SHADOWS
	vec4 shadowUV	= position * matLightViewProj;
	shadowUV.xy		= shadowUV.xy * 0.5 + 0.5;	
	outShadowUV		= shadowUV;
#endif

	outDiffuse.a = 1.0;
	outPosition = (position.xyzw * matWorld).xyz;
	outDepth	= (position * matView).z;	
}
