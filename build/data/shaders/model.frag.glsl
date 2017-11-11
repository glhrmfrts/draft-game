#version 330

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
uniform int u_MaterialFlags;
uniform vec2 u_UvScale;
uniform vec4 u_ExplosionLightColor;
uniform float u_ExplosionLightTimer;
uniform vec3 u_CamPos;
uniform vec4 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

smooth in vec2  v_Uv;
smooth in vec4  v_Color;
smooth in vec3  v_Normal;
smooth in vec4  v_WorldPos;

layout (location = 0) out vec4 BlendUnitColor[2];

void main() {
  vec4 TexColor = texture(u_Sampler, v_Uv * u_UvScale);
  vec4 Color = mix(vec4(1.0f), TexColor, u_TexWeight);
  Color *= v_Color;
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
