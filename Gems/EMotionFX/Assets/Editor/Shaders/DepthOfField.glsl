uniform sampler2D blurredMap;
uniform sampler2D sharpMap;
uniform sampler2D dofInfoMap;

void main()
{
	vec3	blurredSample	= texture2D( blurredMap, gl_TexCoord[0].xy ).rgb;
	vec3	sharpSample		= texture2D( sharpMap, gl_TexCoord[0].xy ).rgb;
	float	blurFactor		= clamp(1.0 - texture2D( dofInfoMap, gl_TexCoord[0].xy ).a, 0.0, 1.0);
	
//	gl_FragColor = mix(blurredSample, sharpSample, blurFactor);
	gl_FragColor = vec4( mix(blurredSample, sharpSample, blurFactor), 1.0 );
}

