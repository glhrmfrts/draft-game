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
uniform int u_orientation;
uniform float u_offset;
uniform vec3 u_coefficients;

smooth in vec2 v_uv;

out vec4 outColor;

void main() {
	vec2 offset = vec2(0, 0);

	if (u_orientation == 0) {
		offset.x = u_offset;
	}
	else {
		offset.y = u_offset;
	}

  vec4 c;
	c  = u_coefficients.x * texture2D(u_sampler, v_uv - offset);
  c += u_coefficients.y * texture2D(u_sampler, v_uv);
  c += u_coefficients.z * texture2D(u_sampler, v_uv + offset);

  outColor = c;
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