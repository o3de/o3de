

varying float outDepth;


void main()
{	
	//float l = (input.depth + 1.0f) * 0.5f;
	float l = outDepth;
	
	gl_FragColor = vec4( l, l, l, 1.0 );
}
