#extension GL_ARB_draw_buffers : require

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

varying vec3  outHalfAngle;
uniform vec4  lightSpecular;
uniform float specularPower;
uniform sampler2D normalMap;
uniform vec3 mainLightDir;

varying vec4 outDiffuse;
varying vec3 outPosition;
varying float outDepth;

varying vec3 outNormal;
varying vec3 outTangent;
varying vec3 outBinormal;

uniform vec3 rimLightDir;
uniform float rimLightFactor;
uniform float rimWidth;
uniform vec4 rimLightColor;
uniform vec3 eyePoint;

uniform vec4 skyColor;
uniform vec4 groundColor;

#ifdef SHADOWS
	varying vec4 outShadowUV;	
	uniform sampler2D shadowMap;
	uniform float shadowMapSize;
	uniform float shadowMapBias;
#endif


uniform float glowThreshold;
uniform float focalPlaneDepth;
uniform float nearPlaneDepth;
uniform float farPlaneDepth;
uniform float blurCutoff;


float CalcDOFAmount(float depth)
{
	float f;
	
	if (depth < nearPlaneDepth)
		return 1.0 - pow(depth / nearPlaneDepth, 2.0);
	else
	{	
		if (depth < focalPlaneDepth)
			f = (depth - focalPlaneDepth) / abs(focalPlaneDepth - nearPlaneDepth);
		else
		{
			f = (depth - focalPlaneDepth) / abs(farPlaneDepth - focalPlaneDepth);
			f = clamp(f, 0.0, blurCutoff);
		}
	}
	
	return (f * 0.5) + 0.5;
}


void main()
{		
	vec3	diffuse		= outDiffuse.rgb;
	vec3	specular	= vec3(0.0, 0.0, 0.0);
	vec3	glowColor	= vec3(0.0, 0.0, 0.0);
	float	alpha		= 1.0;	
	
    mat3 tbnMatrix = mat3( normalize( vec3(outTangent.x,  outTangent.y,  outTangent.z) ),
                           normalize( vec3(outBinormal.x, outBinormal.y, outBinormal.z) ),
                           normalize( vec3(outNormal.x,   outNormal.y,   outNormal.z) ) );
		
	#ifdef LIGHTING
		vec3 normal = normalize( texture2D(normalMap, gl_TexCoord[0].xy).rgb  * 2.0 - vec3(1.0, 1.0, 1.0));
		float specDot = dot(normal, outHalfAngle * tbnMatrix );
		specDot = clamp(specDot, 0.001, 1.0);
		float specularFactor = pow( specDot, specularPower );
		specular = specularFactor * lightSpecular.rgb;
	#else
		diffuse = vec3(1.0);
	#endif
	

	#ifdef TEXTURING
		diffuse  *= texture2D(diffuseMap, gl_TexCoord[0].xy).rgb;
		specular *= texture2D(specularMap, gl_TexCoord[0].xy).rgb;
	#else
		//diffuse = vec3(1.0);
	#endif
	
	
	#ifdef SHADOWS
		vec2 shadowTexCoords[9];
		float shadowTexelSize = 1.0 / shadowMapSize;
		
		// 4 3 5
		// 1 0 2
		// 7 6 8
		shadowTexCoords[0] = outShadowUV.xy;
		shadowTexCoords[1] = outShadowUV.xy + vec2( -shadowTexelSize, 0.0 );
		shadowTexCoords[2] = outShadowUV.xy + vec2(  shadowTexelSize, 0.0 );
		
		shadowTexCoords[3] = outShadowUV.xy + vec2( 0.0, -shadowTexelSize );
		shadowTexCoords[6] = outShadowUV.xy + vec2( 0.0,  shadowTexelSize );
		
		shadowTexCoords[4] = outShadowUV.xy + vec2( -shadowTexelSize, -shadowTexelSize );
		shadowTexCoords[5] = outShadowUV.xy + vec2(  shadowTexelSize, -shadowTexelSize );
		
		shadowTexCoords[7] = outShadowUV.xy + vec2( -shadowTexelSize,  shadowTexelSize );
		shadowTexCoords[8] = outShadowUV.xy + vec2(  shadowTexelSize,  shadowTexelSize );
		
		// Check for shadowed
		float shadowTerm = 0.0;

		for( int i = 0; i < 9; i++ )
		{
			float A = texture2D( shadowMap, shadowTexCoords[i] ).r;
			float B = outShadowUV.z + 0.1;
			shadowTerm += A < B ? 0.05 : 0.0;
		}

		shadowTerm  *= 1.0/9.0;
		diffuse.rgb  = vec3(shadowTerm,shadowTerm,shadowTerm);
	//	diffuse.a    = min(shadowTerm*12.0,1.0);
		alpha = min(shadowTerm*12.0,1.0);
	#endif
	
	
// output shaded
#ifdef LIGHTING
	float dofAmount = CalcDOFAmount( outDepth );
	
	//-----------------------------
	
	float factor	= (dot(normal, (-mainLightDir) * tbnMatrix) + 1.0) * 0.5;
	factor          = clamp(factor, 0.0, 1.0);
	vec4 interCol	= mix(groundColor, skyColor, factor); 
	interCol		= interCol * factor;
	
	// secondary sky light
	factor			= (dot(normal, vec3(0.0, 0.0, 1.0) * tbnMatrix) + 1.0) * 0.5;
	factor          = clamp(factor, 0.0, 1.0);
	vec4 interCol2	= mix(groundColor, skyColor, factor); 
	interCol2		= interCol2 * factor;
	interCol		+= interCol2;

	vec3 skyResult = vec3(interCol.r, interCol.g, interCol.b);

	//----------------------------
	vec3 eyeDir = normalize(eyePoint - outPosition) * tbnMatrix;
	vec3 rimNormal = normal;
	//vec4 rim = RimLight(0.2, rimNormal, rimLightDir, rimColor, eyeDir);
	vec3 nf = normalize( faceforward(rimNormal, -eyeDir, rimNormal) );
	float stepValue = 1.0 - dot(nf, eyeDir);
	stepValue = clamp(stepValue, 0.0, 1.0);
    float rimAmount = smoothstep(1.0 - rimWidth, 1.0, 1.0 - dot(nf, eyeDir));
    //vec3 rimDir = normalize( outRimDir ); //rimLightDir * outTangentMatrix;
    vec3 rimDir = rimLightDir * tbnMatrix;
	float dotValue = dot(rimDir, rimNormal);
	dotValue = clamp(dotValue, 0.0, 1.0);
	rimAmount *= smoothstep(1.0 - rimWidth, 1.0, dotValue);
    vec4 rim = rimAmount * rimLightFactor * rimLightColor;
    //----------------------------

	vec3 finalColor = (diffuse * skyResult) + specular + rim.rgb;

	float colorIntensity = finalColor.r*0.212671 + finalColor.g*0.715160 + finalColor.b*0.072169;
	if (colorIntensity > glowThreshold)
		glowColor = finalColor;

	gl_FragData[0] = vec4(finalColor.rgb, dofAmount);
#else
	float dofAmount = CalcDOFAmount( outDepth );
	dofAmount = clamp( dofAmount, 0.0, 1.0 );
	gl_FragData[0] = vec4(diffuse, dofAmount);

	float colorIntensity = diffuse.r*0.212671 + diffuse.g*0.715160 + diffuse.b*0.072169;
	if (colorIntensity > glowThreshold)
		glowColor = diffuse.rgb;
#endif

	// glow
	float depthValue = clamp(1.0 - outDepth, 0.0, 1.0);
	gl_FragData[1] = vec4(glowColor, depthValue );
}
