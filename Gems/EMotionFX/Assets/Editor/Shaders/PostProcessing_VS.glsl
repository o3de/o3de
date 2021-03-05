


varying vec4 outColor;


void main()
{
	vec4 pos = vec4(inPosition, 1);
	
	pos = matViewProj * pos;
	
	gl_Position = pos;

	outColor = inColor;
}
