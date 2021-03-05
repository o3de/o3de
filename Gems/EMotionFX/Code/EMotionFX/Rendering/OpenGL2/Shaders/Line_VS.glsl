
uniform mat4 matViewProj;

attribute vec3 inPosition;
attribute vec4 inColor;

varying vec4 outColor;


void main()
{
	vec4 pos = vec4(inPosition, 1);
	
	pos = matViewProj * pos;
	
	gl_Position = pos;

	outColor = inColor;
}
