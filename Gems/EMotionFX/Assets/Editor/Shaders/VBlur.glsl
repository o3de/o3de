uniform sampler2D inputMap;

uniform float	sigma		= 4.0;
uniform vec2	direction	= vec2(0.0, 1.0);
uniform float	level		= 0.0;	//level to take the texture sample from
uniform vec2	inputSize;


void main()
{
	vec2	size	= inputSize; // vec2( textureSize2D( inputMap, 0));
	vec2	offset	= exp2(level) / size; 
	
	int		radius	= int( floor(3.0*sigma) - 1.0);
	vec4	accum	= vec4(0.0);
	
	// precompute factors used every iteration
	float sigmaSqx2 = sigma * sigma * 2.0;
	float sigmaxSqrt2PI = sqrt(2.0 * 3.1415926) * sigma;
	
	// separable Gaussian
    for (int ii = -radius; ii <= radius; ii++)
    {
        float r = float(ii);
	    float factor = pow( 2.71828, -(r*r) / sigmaSqx2 ) / sigmaxSqrt2PI;
		accum += factor * texture2D( inputMap, gl_TexCoord[0].xy + r * direction * offset);
	}
		
	gl_FragColor = vec4( accum.rgb, 1.0 );
}

