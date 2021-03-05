
uniform mat4 matBones[50];
uniform mat4 matLightViewProj;


attribute vec3 inPosition;
attribute vec4 inIndices;

#ifdef SKINNING
	attribute vec4 inWeights;
#endif

varying float outDepth;


void main()
{	
	vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
	ivec4 index = ivec4(inIndices);
	
#ifdef SKINNING
	for( int i = 0; i < 4; i++ )
		position += (vec4(inPosition, 1) * matBones[index[i]]) * inWeights[i];
#else
	position = vec4(inPosition, 1) * matBones[index[0]];	
#endif	

	position = position * matLightViewProj;
	
	gl_Position = position;
	outDepth = position.z;
}
