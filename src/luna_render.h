#ifndef LUNA_RENDER_H
#define LUNA_RENDER_H

#include "luna_common.h"

struct vertex_buffer
{
    vector<float> Vertices;
    GLuint VAO, VBO;
    size_t RawIndex;
    size_t VertexCount;
    size_t VertexSize;
};

struct vertex_attribute
{
    GLuint Location;
    size_t Size;
    GLenum Type;
    size_t Stride;
    size_t Offset;
};

struct shader_program
{
    GLuint ID;
};

struct texture_filters
{
    GLint Min, Mag;
};

struct texture_wrap
{
    GLint WrapS, WrapT;
};

struct texture
{
    string Filename;
    GLuint ID;
    GLuint Target;
    uint32 Width;
    uint32 Height;
    texture_filters Filters;
    texture_wrap Wrap;
    bool Mipmap = false;
};

struct texture_rect
{
    float u, v;
    float u2, v2;
};

struct animated_sprite
{
    vec3 Scale = vec3(1.0f);
    vec3 Offset = vec3(0.0f);
    vector<texture_rect> Frames;
    vector<int> Indices;
    direction Direction = Direction_right;
    float Interval = 0;
    float Timer = 0;
    int CurrentIndex = 0;
    texture_rect *CurrentFrame;
    texture *Texture;
};

struct sprite_animation
{
    vector<int> Frames;
    float Interval;
};

struct projection_orthographic
{
    float Left;
    float Right;
    float Bottom;
    float Top;
};

struct projection_perspective
{
    float Width;
    float Height;
    float Fov;
};

enum camera_type
{
    Camera_orthographic,
    Camera_perspective,
};

struct camera
{
    mat4 ProjectionView;
    mat4 Projection;
    mat4 View;
    vec3 Position;
    vec3 LookAt;
    vec3 Up;
    float Near;
    float Far;
    camera_type Type;
    bool Updated = false;

    union
    {
        projection_orthographic Ortho;
        projection_perspective Perspective;
    };
};

struct material
{
    color DiffuseColor = Color_white;
    float Emission = 0;
    float TexWeight = 0;
    texture *Texture = NULL;
};

struct mesh_part
{
    material Material;
    size_t Offset;
    size_t Count;
    GLuint PrimitiveType;
};

struct mesh
{
    vertex_buffer Buffer;
    vector<mesh_part> Parts;
};

struct model_program
{
    shader_program ShaderProgram;
    int ProjectionView;
    int Transform;
    int NormalTransform;
    int DiffuseColor;
    int TexWeight;
    int Sampler;
};

struct render_state
{
#ifdef LUNA_DEBUG
    vertex_buffer DebugBuffer;
#endif
    vertex_buffer SpriteBuffer;
    model_program ModelProgram;
};

void InitBuffer(vertex_buffer &Buffer, size_t VertexSize, size_t AttrCount, ...);
void ResetBuffer(vertex_buffer &Buffer);
void PushVertex(vertex_buffer &Buffer, const vector<float> &Verts);
void UploadVertices(vertex_buffer &Buffer, GLenum Usage);

void CompileShaderProgram(shader_program &Prog, const char *VertexSource, const char *FragmentSource);
void SetUniform(GLuint Location, int Value);
void SetUniform(GLuint Location, float Value);
void SetUniform(GLuint Location, const vec2 &Value);
void SetUniform(GLuint Location, const vec3 &Value);
void SetUniform(GLuint Location, const vec4 &Value);
void SetUniform(GLuint Location, const mat4 &Value);
void Bind(shader_program &Prog);
void UnbindShaderProgram();

void Bind(texture &Texture, int TextureUnit);
void Unbind(texture &Texture, int TextureUnit);
void ApplyTextureParameters(texture &Texture, int TextureUnit);
void UploadTexture(texture &Texture, GLenum SrcFormat, GLenum DstFormat, GLenum Type, uint8 *Data);
vector<texture_rect> SplitTexture(texture &Texture, int Width, int Height, bool FlipV = false);

void SetAnimation(animated_sprite &Sprite, const sprite_animation &Animation, bool Reset = false);
void UpdateAnimation(animated_sprite &Sprite, float DeltaTime);

void MakeCameraOrthographic(camera &Camera, float Left, float Right, float Bottom, float Top, float Near = -1, float Far = 1);
void MakeCameraPerspective(camera &Camera, float Width, float Height, float Fov, float Near, float Far);
void UpdateProjectionView(camera &Camera);

void InitMeshBuffer(vertex_buffer &Buffer);

void InitRenderState(render_state &RenderState);
void RenderMesh(render_state &RenderState, camera &Camera, mesh &Mesh, const mat4 &TransformMatrix);
void RenderSprite(render_state &RenderState, camera &Camera, animated_sprite &Sprite, vec3 Position);

#ifdef LUNA_DEBUG
void DebugRenderAABB(render_state &RenderState, camera &Cam, shape_aabb *Shape, bool Colliding);
#endif

#endif
