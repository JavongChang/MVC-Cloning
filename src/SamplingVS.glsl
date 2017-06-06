#version 330 compatibility
#extension GL_texture_rectangle_ARB:enable


uniform sampler2DRect tex0;
uniform sampler2DRect tex1;
uniform sampler2DRect tex2;
uniform sampler2DRect tex3;
varying vec4 membrane;

void main()
{
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_TexCoord[1] = gl_MultiTexCoord1;
  gl_TexCoord[2] = gl_MultiTexCoord2;
  gl_TexCoord[3] = gl_MultiTexCoord3;
  gl_TexCoord[4] = gl_MultiTexCoord4;

  float x, y, weight;
  vec4 source, target, err;
  vec4 xy;
  int boundarySize = int(texture2DRect(tex2,gl_TexCoord[2].st).x);
  vec4 WID;
  membrane = vec4(0.0);

  for (int t=1; t<boundarySize+1; t++)
  {
    WID = texture2DRect(tex2,gl_TexCoord[2].st+vec2(t,0.0));

    x = texture2DRect(tex3,vec2(WID.y,0.0)).x;
    y = texture2DRect(tex3,vec2(WID.y,1.0)).x;
    xy = gl_ModelViewMatrix * vec4(x,y,0,1);
    target = texture2DRect(tex0,vec2(xy.x,xy.y));
    source = texture2DRect(tex1,vec2(x,y));
    err = target-source;
    membrane += err*WID.x;
  }
  membrane=membrane/700;
  gl_Position = ftransform();
}
