// Copyright

static string ModelVertexShader = R"FOO(
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
)FOO";

static string ModelFragmentShader = R"FOO(
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

	  Color = mix(Color, u_FogColor, 1.0 - fog);
      vec4 Emit = (Color * u_Emission);
      BlendUnitColor[0] = Color;
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

static string BlurHorizontalFragmentShader = R"FOO(
#version 330

uniform sampler2D u_sampler;
uniform float u_pixelSize;

smooth in vec2 v_uv;

out vec4 BlendUnitColor;

void main()
{
	vec4 sum = texture2D(u_sampler,v_uv+u_pixelSize*vec2(-5,0))*0.01222447;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(-4,0))*0.02783468;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(-3,0))*0.06559061;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(-2,0))*0.12097757;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(-1,0))*0.17466632;
	sum += texture2D(u_sampler,v_uv)*0.19741265;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(1,0))*0.17466632;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(2,0))*0.12097757;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(3,0))*0.06559061;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(4,0))*0.02783468;
	sum += texture2D(u_sampler,v_uv+u_pixelSize*vec2(5,0))*0.01222447;
	BlendUnitColor = sum;
}
)FOO";

static string BlurVerticalFragmentShader = R"FOO(
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

  vec4 c = vec4(0.0f);
  c += u_coefficients.x * 0.25f * texture2D(u_sampler, v_uv - offset);
  c += u_coefficients.x * 0.5f  * texture2D(u_sampler, v_uv - offset*0.75f);
  c += u_coefficients.x * 0.75f * texture2D(u_sampler, v_uv - offset*0.5f);
  c += u_coefficients.x * 1.0f  * texture2D(u_sampler, v_uv - offset*0.25f);

  c += u_coefficients.y * 1.0f  * texture2D(u_sampler, v_uv);

  c += u_coefficients.z * 1.0f  * texture2D(u_sampler, v_uv + offset*0.25f);
  c += u_coefficients.z * 0.75f * texture2D(u_sampler, v_uv + offset*0.5f);
  c += u_coefficients.z * 0.5f  * texture2D(u_sampler, v_uv + offset*0.75f);
  c += u_coefficients.z * 0.25f * texture2D(u_sampler, v_uv + offset);

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
