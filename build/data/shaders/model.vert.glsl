#version 330

#define MaterialFlag_TransformUniform 0x4

layout (location = 0) in vec3  a_Position;
layout (location = 1) in vec2  a_Uv;
layout (location = 2) in vec4  a_Color;
layout (location = 3) in vec3  a_Normal;

uniform mat4 u_ProjectionView;
uniform mat4 u_Transform;
uniform mat4 u_NormalTransform;
uniform int  u_MaterialFlags;

smooth out vec2 v_Uv;
smooth out vec4 v_Color;
smooth out vec3 v_Normal;
smooth out vec4 v_WorldPos;

vec3 WorldToRenderTransform(in vec3 pos) {
  float d = pos.y*0.005f;
  float r = 500.0f - pos.z;
  vec3 result = pos;
  result.y = cos(d) * r;
  result.z = sin(d) * r;
  return result;
}

void main() {
  mat4 transformMatrix = u_Transform;
  vec3 transformPos = u_Transform[3].xyz;
  if ((u_MaterialFlags & MaterialFlag_TransformUniform) > 0)
  {
    transformMatrix[3] = vec4(WorldToRenderTransform(transformPos), 1.0);
  }

  vec4 worldPos = transformMatrix * vec4(a_Position, 1.0);
  if ((u_MaterialFlags & MaterialFlag_TransformUniform) == 0)
  {
    worldPos = vec4(WorldToRenderTransform(worldPos.xyz), 1.0);
  }

  gl_Position = u_ProjectionView * worldPos;

  v_Normal = (u_NormalTransform * vec4(a_Normal, 1.0)).xyz;
  v_Uv = a_Uv;
  v_Color = a_Color;
  v_WorldPos = worldPos;
}
