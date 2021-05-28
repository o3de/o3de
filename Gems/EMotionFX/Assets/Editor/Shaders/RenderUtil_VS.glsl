uniform mat4 worldViewProjectionMatrix;
uniform mat4 worldMatrix;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

attribute vec3 inPosition;
attribute vec3 inNormal;

varying vec3 outWorldNormal;
varying vec3 outHalfAngle;
varying vec3 outLightVec;


void main()
{
	vec4 position		= vec4(inPosition, 1.0);
	vec4 worldPosition	= vec4(inPosition, 1.0) * worldMatrix;
	outWorldNormal		= normalize( (vec4(inNormal, 0.0) * worldMatrix).xyz );

	// half-angle vector
	vec3 eyeVec	 = normalize( cameraPosition - worldPosition.xyz );
	outLightVec	 = normalize( -lightDirection );
	outHalfAngle = normalize( eyeVec + outLightVec );

	gl_Position = position * worldViewProjectionMatrix;
}

