varying vec3 ls;

void main()
{
  vec4 npos = gl_ModelViewMatrix * gl_Vertex;

  ls = normalize(gl_LightSource[0].position.xyz - npos.xyz);

  gl_Position = ftransform();

  gl_TexCoord[0]  = gl_Vertex;

  gl_FrontColor = gl_Color;
  gl_BackColor = gl_Color;
}
