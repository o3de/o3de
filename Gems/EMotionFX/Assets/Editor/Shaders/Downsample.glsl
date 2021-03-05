uniform sampler2D inputMap;

void main()
{	
	gl_FragColor = texture2D( inputMap, gl_TexCoord[0].xy );
}
