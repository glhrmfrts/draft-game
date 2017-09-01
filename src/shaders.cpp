// Copyright

static string ModelVertexShader = R"FOO(
#version 330

// In this game we use only one forward directional light
// so these simple defines will do the job
#define AmbientLight vec4(0.5f)
#define LightColor vec4(1.0f)
#define LightIntensity 1
#define M_PI 3.1415926535897932384626433832795

    layout (location = 0) in vec3  a_Position;
    layout (location = 1) in vec2  a_Uv;
    layout (location = 2) in vec4  a_Color;
    layout (location = 3) in vec3  a_Normal;

    uniform mat4 u_ProjectionView;
    uniform mat4 u_Transform;
    uniform mat4 u_NormalTransform;
    uniform int u_Materials;

    smooth out vec2  v_Uv;
    smooth out vec4  v_Color;

    void main() {
      vec4 WorldPos = u_Transform * vec4(a_Position, 1.0);
      gl_Position = u_ProjectionView * WorldPos;

      vec3 Normal = (u_NormalTransform * vec4(a_Normal, 1.0)).xyz;

      vec4 Lighting = AmbientLight;
      Lighting += LightColor * dot(-vec3(0, 0, -1), Normal) * LightIntensity;
      Lighting.a = 1.0f;

      v_Uv = a_Uv;
      v_Color = a_Color * clamp(Lighting, 0, 1);
    }
)FOO";

static string ModelFragmentShader = R"FOO(
#version 330

    uniform sampler2D u_Sampler;
    uniform vec4 u_DiffuseColor;
    uniform float u_Emission;
    uniform float u_TexWeight;

    smooth in vec2  v_Uv;
    smooth in vec4  v_Color;

    layout (location = 0) out vec4 BlendUnitColor[2];

    void main() {
      vec4 TexColor = texture(u_Sampler, v_Uv);
      vec4 Color = mix(vec4(1.0f), TexColor, u_TexWeight);
      Color *= v_Color;
      Color *= u_DiffuseColor;

      //float fog = abs(v_worldPos.z - u_camPos.z);
      //fog = (u_fogEnd - fog) / (u_fogEnd - u_fogStart);
      //fog = clamp(fog, 0.0, 1.0);

      vec4 Emit = (Color * u_Emission);

      BlendUnitColor[0] = Emit + Color;//mix(color, u_fogColor, 1.0 - fog);
      BlendUnitColor[1] = Emit;
    }
)FOO";

static string BlitVertexShader = R"FOO(
#version 330

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;

smooth out vec2 v_uv;

void main() {
  gl_Position = vec4(a_position, 0.0, 1.0);
  v_uv = a_uv;
}
)FOO";

static string BlitFragmentShader = R"FOO(
#version 330

uniform sampler2D u_Sampler;

smooth in vec2 v_uv;

out vec4 BlendUnitColor;

void main() {
    BlendUnitColor = texture2D(u_Sampler, v_uv);
}
)FOO";

static string BlurFragmentShader = R"FOO(
#version 330

uniform sampler2D u_sampler;
uniform vec2 u_texelSize;
uniform int u_orientation;
uniform int u_amount;
uniform float u_scale;
uniform float u_strength;

smooth in vec2 v_uv;

out vec4 outColor;

float gaussian(float x, float deviation) {
	return (1.0 / sqrt(2.0 * 3.141592 * deviation)) * exp(-((x * x) / (2.0 * deviation)));
}

void main() {
	float halfBlur = float(u_amount) * 0.5;
	vec4 colour = vec4(0.0);
	vec4 texColour = vec4(0.0);

	// gaussian deviation
	float deviation = halfBlur * 0.35;
	deviation *= deviation;
	float strength = 1.0 - u_strength;

	if (u_orientation == 0) {
		// horizontal blur
		for (int i = 0; i < 10; ++i) {
			if (i >= u_amount)
				break;

			float offset = float(i) - halfBlur;
			texColour = texture2D(u_sampler, v_uv + vec2(offset * u_texelSize.x * u_scale, 0.0)) * gaussian(offset * strength, deviation);
			colour += texColour;
		}
	}
	else {
		// vertical blur
		for (int i = 0; i < 10; ++i) {
			if (i >= u_amount)
				break;

			float offset = float(i) - halfBlur;
			texColour = texture2D(u_sampler, v_uv + vec2(0.0, offset * u_texelSize.y * u_scale)) * gaussian(offset * strength, deviation);
			colour += texColour;
		}
	}

	// apply colour
	outColor = colour;
}
)FOO";

static string BlendFragmentShader = R"FOO(
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
)FOO";

static string FXAAFragmentShader = R"FOO(
#version 330

uniform sampler2D u_sampler;
uniform vec2 u_resolution;

smooth in vec2 v_uv;

#ifndef FXAA_REDUCE_MIN
#define FXAA_REDUCE_MIN   (1.0/ 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
#define FXAA_REDUCE_MUL   (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
#define FXAA_SPAN_MAX     8.0
#endif

//optimized version for mobile, where dependent
//texture reads can be a bottleneck
vec4 fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution,
          vec2 v_rgbNW, vec2 v_rgbNE,
          vec2 v_rgbSW, vec2 v_rgbSE,
          vec2 v_rgbM)
{
  vec4 color;
  vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);
  vec3 rgbNW = texture2D(tex, v_rgbNW).xyz;
  vec3 rgbNE = texture2D(tex, v_rgbNE).xyz;
  vec3 rgbSW = texture2D(tex, v_rgbSW).xyz;
  vec3 rgbSE = texture2D(tex, v_rgbSE).xyz;
  vec4 texColor = texture2D(tex, v_rgbM);
  vec3 rgbM  = texColor.xyz;
  vec3 luma = vec3(0.299, 0.587, 0.114);
  float lumaNW = dot(rgbNW, luma);
  float lumaNE = dot(rgbNE, luma);
  float lumaSW = dot(rgbSW, luma);
  float lumaSE = dot(rgbSE, luma);
  float lumaM  = dot(rgbM,  luma);
  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

  vec2 dir;
  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
  dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                        (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
  dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                dir * rcpDirMin)) * inverseVP;

  vec3 rgbA = 0.5 * (
                     texture2D(tex, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
                     texture2D(tex, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
  vec3 rgbB = rgbA * 0.5 + 0.25 * (
                                   texture2D(tex, fragCoord * inverseVP + dir * -0.5).xyz +
                                   texture2D(tex, fragCoord * inverseVP + dir * 0.5).xyz);

  float lumaB = dot(rgbB, luma);
  if ((lumaB < lumaMin) || (lumaB > lumaMax))
    color = vec4(rgbA, texColor.a);
  else
    color = vec4(rgbB, texColor.a);
  return color;
}

out vec4 BlendUnitColor;

void main()
{
  vec2 v_rgbNW;
  vec2 v_rgbNE;
  vec2 v_rgbSW;
  vec2 v_rgbSE;
  vec2 v_rgbM;

  vec2 fragCoord = v_uv * u_resolution;
  vec2 inverseVP = 1.0 / u_resolution.xy;
  v_rgbNW = (fragCoord + vec2(-1.0, -1.0)) * inverseVP;
  v_rgbNE = (fragCoord + vec2(1.0, -1.0)) * inverseVP;
  v_rgbSW = (fragCoord + vec2(-1.0, 1.0)) * inverseVP;
  v_rgbSE = (fragCoord + vec2(1.0, 1.0)) * inverseVP;
  v_rgbM = vec2(fragCoord * inverseVP);

  BlendUnitColor = fxaa(u_sampler, fragCoord, u_resolution, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
}
)FOO";

static string ResolveMultisampleFragmentShader = R"FOO(
#version 330

uniform sampler2DMS u_SurfaceReflectSampler;
uniform sampler2DMS u_EmitSampler;
uniform int u_SampleCount;

smooth in vec2 v_uv;

layout (location = 0) out vec4 BlendUnitColor[2];

void main()
{
  vec4 CombinedColor = vec4(0.0f);
  vec4 CombinedEmit = vec4(0.0f);
  for (int i = 0; i < u_SampleCount; i++)
  {
    CombinedColor += texelFetch(u_SurfaceReflectSampler, ivec2(gl_FragCoord.xy), i);
    CombinedEmit += texelFetch(u_EmitSampler, ivec2(gl_FragCoord.xy), i);
  }
  float InvSampleCount = 1.0f / float(u_SampleCount);
  BlendUnitColor[0] = InvSampleCount * CombinedColor;
  BlendUnitColor[1] = InvSampleCount * CombinedEmit;
}
)FOO";