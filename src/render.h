#ifndef DRAFT_RENDER_H
#define DRAFT_RENDER_H

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
    GLuint ID = 0;
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

#define MaterialFlag_NoLight 0x1
#define MaterialFlag_PolygonLines 0x2
struct material
{
    color DiffuseColor = Color_white;
    float Emission = 0;
    float TexWeight = 0;
    texture *Texture = NULL;
    uint32 Flags = 0;
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
    vec3 Min;
    vec3 Max;
    vector<mesh_part> Parts;
};

struct model
{
    mesh *Mesh;
    material *Material = NULL;
    size_t MaterialOverrideIndex = 0;
};

struct model_program
{
    shader_program ShaderProgram;
    int ProjectionView;
    int Transform;
    int NormalTransform;
    int DiffuseColor;
    int TexWeight;
    int Emission;
    int Sampler;
    int MaterialFlags;
};

struct render_state
{
#ifdef DRAFT_DEBUG
    vertex_buffer DebugBuffer;
#endif
    vertex_buffer SpriteBuffer;
    model_program ModelProgram;
};

#endif
