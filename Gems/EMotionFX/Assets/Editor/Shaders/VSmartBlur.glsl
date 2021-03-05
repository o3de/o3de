#extension GL_ARB_draw_buffers : require
#extension GL_EXT_gpu_shader4 : require

uniform sampler2D inputMap;
uniform sampler2D depthColorMap;
uniform sampler2D normalMap;

uniform float sigma = 4.0;
uniform vec2 direction = vec2( 0.0, 1.0);
uniform float level = 0.0;	//level to take the texture sample from
uniform float depthThreshold = 0.9;


void main()
{
	vec2 size = vec2( textureSize2D( inputMap, 0));
	vec2 offset = exp2(level) / size; 
	
	//compute the radius across the kernel
	int radius = int( floor(3.0*sigma) - 1.0);
	
	float accum = float(0.0);
	vec4 accum2 = vec4(0.0);
	
	//precompute factors used every iteration
	float sigmaSqx2 = sigma * sigma * 2.0;
	float sigmaxSqrt2PI = sqrt(2.0 * 3.1415926) * sigma;
	
    vec3 pixelNormal = texture2D( normalMap, gl_TexCoord[0].xy ).rgb;
    float pixelDepth = texture2D( depthColorMap, gl_TexCoord[0].xy).r;
	
	// separable Gaussian
    for ( int ii = -radius; ii <= radius; ii++)
    {
        float r = float(ii);
	    float factor = pow( 2.71828, -(r*r) / sigmaSqx2 ) / sigmaxSqrt2PI;
	    vec2 sampleUV = gl_TexCoord[0].xy + r * direction * offset;

	    vec3 sampleNormal = texture2D( normalMap, sampleUV ).rgb;
	    float sampleDepth = texture2D( depthColorMap, sampleUV ).r;
	    
	    float depthDiff  = 1.0 / abs(sampleDepth - pixelDepth);
	    float normalDiff = abs( acos( dot(sampleNormal, pixelNormal) ) );
	    if (depthDiff > depthThreshold && normalDiff < 0.25)
	    {
			accum += factor;
			accum2 += factor * texture2DLod( inputMap, sampleUV, level);
		}
	}
		
	gl_FragColor = mix( accum2, texture2D(inputMap, gl_TexCoord[0].xy), 1.0 - accum );
}

