#version 330

uniform sampler2D u_RandomSampler;
uniform float u_Time;
uniform vec2 u_Offset;
uniform vec4 u_Color;

vec3 grad(vec3 p) {
	const float texture_width = 256.0;
	vec4 v = texture(u_RandomSampler, vec2((p.x+p.z) / texture_width, (p.y-p.z) / texture_width));
    return normalize(v.xyz*2.0 - vec3(1.0));
}

/* S-shaped curve for 0 <= t <= 1 */
float fade(float t) {
  return t*t*t*(t*(t*6.0 - 15.0) + 10.0);
}

/* 3D noise */
float noise(vec3 p) {
  /* Calculate lattice points. */
  vec3 p0 = floor(p);
  vec3 p1 = p0 + vec3(1.0, 0.0, 0.0);
  vec3 p2 = p0 + vec3(0.0, 1.0, 0.0);
  vec3 p3 = p0 + vec3(1.0, 1.0, 0.0);
  vec3 p4 = p0 + vec3(0.0, 0.0, 1.0);
  vec3 p5 = p4 + vec3(1.0, 0.0, 0.0);
  vec3 p6 = p4 + vec3(0.0, 1.0, 0.0);
  vec3 p7 = p4 + vec3(1.0, 1.0, 0.0);
    
  /* Look up gradients at lattice points. */
  vec3 g0 = grad(p0);
  vec3 g1 = grad(p1);
  vec3 g2 = grad(p2);
  vec3 g3 = grad(p3);
  vec3 g4 = grad(p4);
  vec3 g5 = grad(p5);
  vec3 g6 = grad(p6);
  vec3 g7 = grad(p7);
    
  float t0 = p.x - p0.x;
  float fade_t0 = fade(t0); /* Used for interpolation in horizontal direction */

  float t1 = p.y - p0.y;
  float fade_t1 = fade(t1); /* Used for interpolation in vertical direction. */
    
  float t2 = p.z - p0.z;
  float fade_t2 = fade(t2);

  /* Calculate dot products and interpolate.*/
  float p0p1 = (1.0 - fade_t0) * dot(g0, (p - p0)) + fade_t0 * dot(g1, (p - p1)); /* between upper two lattice points */
  float p2p3 = (1.0 - fade_t0) * dot(g2, (p - p2)) + fade_t0 * dot(g3, (p - p3)); /* between lower two lattice points */

  float p4p5 = (1.0 - fade_t0) * dot(g4, (p - p4)) + fade_t0 * dot(g5, (p - p5)); /* between upper two lattice points */
  float p6p7 = (1.0 - fade_t0) * dot(g6, (p - p6)) + fade_t0 * dot(g7, (p - p7)); /* between lower two lattice points */

  float y1 = (1.0 - fade_t1) * p0p1 + fade_t1 * p2p3;
  float y2 = (1.0 - fade_t1) * p4p5 + fade_t1 * p6p7;

  /* Calculate final result */
  return (1.0 - fade_t2) * y1 + fade_t2 * y2;
}

float expFilter(float n, float cover, float sharpness) {
  return (1 - n-0.2) * 2;
}

out vec4 fragColor[2];

void main()
{
    float n =
        noise(vec3(u_Offset + gl_FragCoord.xy, u_Time * 15.0)/(128.0*2))/ 1.0 +
        noise(vec3(u_Offset + gl_FragCoord.xy, u_Time * 15.0)/(64.0*4)) / 1.0 +
        noise(vec3(u_Offset + gl_FragCoord.xy, u_Time * 15.0)/(32.0*1)) / 6.0;

  //n = clamp(n, 0, 1);
  n = expFilter(n, 0.5, 1);

  vec3 color = mix(u_Color.xyz, vec3(0,0,0.5), n);
  //color *= mix(u_Color.xyz, vec3(0,1,1), 1.0 - n);
  fragColor[0] = vec4(color, 0.5);
  fragColor[1] = vec4(0,0,0,0);
}