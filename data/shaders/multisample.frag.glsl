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
