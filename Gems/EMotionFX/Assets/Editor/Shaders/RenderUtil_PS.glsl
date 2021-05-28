uniform vec4 diffuseColor;
uniform vec3 specularColor;
uniform float specularPower;

varying vec3 outWorldNormal;
varying vec3 outHalfAngle;
varying vec3 outLightVec;


void main()
{
	vec3 normal = normalize(outWorldNormal);
	vec4 ambient = diffuseColor * 0.3;

	float diffuseDot = dot(outLightVec, normal);
	if (diffuseDot < 0.0)
	{
		diffuseDot = dot(outLightVec, -normal);
		diffuseDot = abs(diffuseDot);
		
		float hdn = pow(max(0.0, dot(outHalfAngle, -normal)), specularPower);
		vec4 finalSpecular = vec4(hdn * specularColor, 1.0);
		
		gl_FragColor = (diffuseDot * diffuseColor + finalSpecular + ambient) * 0.25;	// back lighting, darkened
	}
	else
	{
		float hdn = pow(max(0.0, dot(outHalfAngle, normal)), specularPower);
		vec4 finalSpecular = vec4(hdn * specularColor, 1.0);
		gl_FragColor = diffuseDot * diffuseColor + finalSpecular + ambient;
	}
}
