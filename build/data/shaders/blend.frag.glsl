#version 330

uniform sampler2D u_Pass0;
uniform sampler2D u_Pass1;
uniform sampler2D u_Pass2;
uniform sampler2D u_Scene;

smooth in vec2 v_uv;

out vec4 outColor;

void main() {
  vec4 pass0 = texture(u_Pass0, v_uv);
  vec4 pass1 = texture(u_Pass1, v_uv);
  vec4 pass2 = texture(u_Pass2, v_uv);
  vec4 scene = texture(u_Scene, v_uv);

  outColor = scene + pass0 + pass1 + pass2;
}
