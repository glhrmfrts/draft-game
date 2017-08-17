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
    float U, V;
    float U2, V2;
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

    union
    {
        projection_orthographic Ortho;
        projection_perspective Perspective;
    };
};

void InitBuffer(vertex_buffer *Buffer, size_t VertexSize, size_t AttrCount, ...);
void ResetBuffer(vertex_buffer *Buffer);
void PushVertex(vertex_buffer *Buffer, ...);
void UploadVertices(vertex_buffer *Buffer, GLenum Usage);

void CompileShaderProgram(shader_program *Prog, const char *VertexSource, const char *FragmentSource);
void SetUniform(GLuint Location, int Value);
void SetUniform(GLuint Location, float Value);
void SetUniform(GLuint Location, const vec2 &Value);
void SetUniform(GLuint Location, const vec3 &Value);
void SetUniform(GLuint Location, const vec4 &Value);
void SetUniform(GLuint Location, const mat4 &Value);
void Bind(shader_program *Prog);
void UnbindShaderProgram();

void Bind(texture *Texture, int TextureUnit);
void Unbind(texture *Texture, int TextureUnit);
void ApplyTextureParameters(texture *Texture, int TextureUnit);
void UploadTexture(texture *Texture, GLenum SrcFormat, GLenum DstFormat, GLenum Type, uint8 *Data);

void MakeCameraOrthographic(camera *Camera, float Left, float Right, float Bottom, float Top, float Near = -1, float Far = 1);
void MakeCameraPerspective(camera *Camera, float Width, float Height, float Fov, float Near, float Far);
void UpdateProjectionView(camera *Camera);

#endif
