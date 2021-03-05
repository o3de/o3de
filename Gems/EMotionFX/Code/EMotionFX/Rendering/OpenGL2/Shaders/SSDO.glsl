#extension GL_ARB_draw_buffers : require
#extension GL_EXT_gpu_shader4 : require

uniform sampler2D inputMap;
uniform sampler2D normalMap;
uniform sampler2D eyeNormalMap;
uniform sampler2D depthColorMap;
uniform sampler2D positionMap;
uniform sampler2D shadedMap;
uniform sampler2D randomVectorMap;
uniform float	  radius = 6.0;
uniform vec2      inputSize;

uniform vec3	  lightDir;

const int numSamples = 32;
uniform vec3 sampleOffsets[numSamples];


void main()
{
	// sample the position and normal map
	vec4 finalColor = vec4(0.0);
	vec3 pixelPos = texture2D( positionMap, gl_TexCoord[0].xy ).xyz;
	vec3 normal	  = texture2D( normalMap,   gl_TexCoord[0].xy ).xyz;
	vec3 eyeNormal = texture2D( eyeNormalMap,   gl_TexCoord[0].xy ).xyz;
	

		// for all samples we want to generate
		for (int i=0; i<numSamples; i++)
		{	
			// extract a random vector from the random vector texture
			vec3 randomDir = texture2D( randomVectorMap, gl_TexCoord[0].xy ).xyz;
			vec3 newDir = radius * reflect(sampleOffsets[i], randomDir);
			vec3 samplePos = pixelPos + sign( dot(newDir, -eyeNormal) ) * newDir;
			
			if (samplePos.z <= 0)
			{
				//gl_FragColor = texture2D(shadedMap, gl_TexCoord[0].xy);		
				return;
			}
				
			// backproject sample point into texture coordinate space
			vec3 newPos = samplePos;
			float sampleDepth = 1.0 - (texture2DProj( depthColorMap, newPos*0.5+newPos.z*vec3(0.5) ).r);
//			samplePos.z = sampleDepth;
			
			float dist = distance(vec3(samplePos.xy, sampleDepth), pixelPos);
			float attenuation = 1.0f - (1.0f / (dist * 0.5));
			float attenuation2 = 1.0f / (dist * 0.5);
			
			vec3 lightVec = normalize(samplePos - pixelPos);
			
			if (samplePos.z <= sampleDepth)
				finalColor += vec4( 1.0 );
				//finalColor += vec4( /*dot(normal, lightVec)*//* * dot(lightVec, lightDir)*/ );
			//else
				//finalColor += vec4( attenuation * 0.5 );
	/*						
			// calculate the occlusion factor
			//finalColor += backProjected;
			float zd = 50.0*max( backProjected.z-backProjected.x, 0.0 );
			finalColor += 1.0/(1.0+zd*zd);*/
		}
		finalColor /= float(numSamples);
	//	finalColor *= texture2D(inputMap, gl_TexCoord[0].xy);
	
	gl_FragColor = finalColor;// * texture2D(shadedMap, gl_TexCoord[0].xy);
}

