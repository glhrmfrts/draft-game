#version 330

uniform sampler2D u_Sampler;

smooth in vec2 v_uv;

out vec4 BlendUnitColor;

void main() {
    BlendUnitColor = texture2D(u_Sampler, v_uv);
}
