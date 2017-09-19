#version 330

layout (location = 0) in vec3  a_Position;
layout (location = 1) in vec2  a_Uv;
layout (location = 2) in vec4  a_Color;
layout (location = 3) in vec3  a_Normal;

uniform mat4 u_ProjectionView;
uniform mat4 u_Transform;
uniform mat4 u_NormalTransform;

smooth out vec2 v_Uv;
smooth out vec4 v_Color;
smooth out vec3 v_Normal;
smooth out vec4 v_WorldPos;

void main() {
  vec4 WorldPos = u_Transform * vec4(a_Position, 1.0);
  gl_Position = u_ProjectionView * WorldPos;

  v_Normal = (u_NormalTransform * vec4(a_Normal, 1.0)).xyz;
  v_Uv = a_Uv;
  v_Color = a_Color;
  v_WorldPos = WorldPos;
}
