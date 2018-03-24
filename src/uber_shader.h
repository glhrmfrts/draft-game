#ifndef DRAFT_UBER_SHADER_H
#define DRAFT_UBER_SHADER_H

static const char *UberVertexShaderSource = R"EOF(

#define MaterialFlag_TransformUniform 0x4

#define MY_PI 3.14159265359

layout (location = 0) in vec3  a_Position;

#ifdef A_UV
layout (location = A_UV) in vec2  a_Uv;
#endif

#ifdef A_COLOR
layout (location = A_COLOR) in vec4 a_Color;
#endif

layout (location = A_NORMAL) in vec3 a_Normal;

uniform mat4 u_ProjectionView;
uniform mat4 u_Transform;
uniform mat4 u_NormalTransform;
uniform int  u_MaterialFlags;
uniform float u_BendRadius;
uniform float u_RoadTangentPoint;

#ifdef A_UV
smooth out vec2 v_Uv;
#endif

#ifdef A_COLOR
smooth out vec4 v_Color;
#endif

smooth out vec3 v_Normal;
smooth out vec4 v_WorldPos;

vec3 WorldToRenderTransformInner(in vec3 pos) {
  float d = pos.y*0.005f;
  float r = u_BendRadius - pos.z;
  vec3 result = pos;
  result.y = cos(d) * r;
  result.z = sin(d) * r;
  return result;
}

vec3 WorldToRenderTransform(in vec3 pos) {
  vec3 worldPos = pos;
  vec3 tangentWorldPoint = vec3(pos.x, u_RoadTangentPoint, pos.z);
  vec3 result = WorldToRenderTransformInner(pos);

  if (pos.y >= tangentWorldPoint.y) {
    vec3 tangentPoint = WorldToRenderTransformInner(tangentWorldPoint);
    vec3 tangentDir = normalize(vec3( 0, -tangentPoint.z, tangentPoint.y ));
    result = tangentPoint + (tangentDir * ((pos.y - tangentWorldPoint.y) * (u_BendRadius * 0.005f)));
  }

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
  v_WorldPos = worldPos;

#ifdef A_UV
v_Uv = a_Uv;
#endif

#ifdef A_COLOR
v_Color = a_Color;
#endif
}

)EOF";

static const char *UberFragmentShaderSource = R"EOF(

// In this game we use only one forward directional light
// so these simple defines will do the job
#define AmbientLight 0.5f
#define LightColor vec4(1, 1, 1, 1)
#define LightIntensity 1.0f
#define M_PI 3.1415926535897932384626433832795

uniform sampler2D u_Sampler;
uniform vec4 u_DiffuseColor;
uniform float u_Emission;
uniform float u_TexWeight;
uniform float u_FogWeight;
uniform vec2 u_UvScale;
uniform vec4 u_ExplosionLightColor;
uniform float u_ExplosionLightTimer;
uniform vec3 u_CamPos;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

#ifdef A_UV
smooth in vec2  v_Uv;
#endif

#ifdef A_COLOR
smooth in vec4  v_Color;
#endif

smooth in vec3  v_Normal;
smooth in vec4  v_WorldPos;

layout (location = 0) out vec4 BlendUnitColor[2];

void main() {

#ifdef A_UV
  vec4 TexColor = texture(u_Sampler, v_Uv * u_UvScale);
#else
  vec4 TexColor = vec4(1.0f);
#endif

  vec4 Color = mix(vec4(1.0f), TexColor, u_TexWeight);

#ifdef A_COLOR
  Color *= v_Color;
#endif

  Color *= u_DiffuseColor;

  vec4 Lighting = LightColor * AmbientLight;
  Lighting += LightColor * clamp(dot(-vec3(0, 0, -1), v_Normal), 0, 1) * LightIntensity;
  Lighting.a = 1.0f;
  Color *= Lighting;

  float Amount = u_ExplosionLightTimer / 1.0f;
  Color += u_ExplosionLightColor * Amount * 0.5f;

  float fog = abs(v_WorldPos.y - u_CamPos.y);
  fog = (u_FogEnd - fog) / (u_FogEnd - u_FogStart);
  fog = clamp(fog, 0.0, 1.0);

  Color = mix(Color, u_FogColor, (1.0 - fog) * u_FogWeight);
  vec4 Emit = (Color * u_Emission);
  BlendUnitColor[0] = Color;
  BlendUnitColor[1] = Emit;
}

)EOF";

#endif
