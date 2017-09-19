#version 330

uniform sampler2D u_sampler;
uniform float u_pixelSize;

smooth in vec2 v_uv;

out vec4 BlendUnitColor;

void main()
{
  vec4 sum = texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,-5))*0.01222447;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,-4))*0.02783468;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,-3))*0.06559061;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,-2))*0.12097757;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,-1))*0.17466632;
  sum += texture2D(u_sampler,v_uv)*0.19741265;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,1))*0.17466632;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,2))*0.12097757;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,3))*0.06559061;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,4))*0.02783468;
  sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(0,5))*0.01222447;
  BlendUnitColor = sum;
}
