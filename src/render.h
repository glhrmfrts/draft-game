#ifndef DRAFT_RENDER_H
#define DRAFT_RENDER_H

#define Color_white color(1, 1, 1, 1)
#define Color_black color(0, 0, 0, 1)
#define Color_blue  color(0, 0, 0.5, 1)
#define Color_green  color(0, 1, 0, 1)
#define Color_red  color(1, 0, 0, 1)
#define Color_gray  color(0.5, 0.5, 0.5, 1)
#define Color_yellow  color(1, 1, 0, 1)

struct color_palette
{
    int Colors[5];
};

static color_palette FirstPalette = {{0x21042D, 0x6F0C74, 0x25006D, 0x0E00CA, 0xEE1F97}};
static color_palette SecondPalette = {{0x86008A, 0x4B0034, 0x2703EC, 0x00BDFF, 0xB8C9EE}};

inline static color
IntColor(int c, float alpha = 1.0f)
{
    int b = c & 0xFF;
    int g = (c >> 8) & 0xFF;
    int r = (c >> 16) & 0xFF;
    return color(r/255.0f, g/255.0f, b/255.0f, alpha);
}

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

#define Material_NoLight 0x1
#define Material_PolygonLines 0x2
struct material
{
    color DiffuseColor = Color_white;
    float Emission = 0;
    float TexWeight = 0;
    texture *Texture = NULL;
    uint32 Flags = 0;

    material() {}
    material(color c, float e, float tw, texture *t, uint32 f = 0)
        : DiffuseColor(c), Emission(e), TexWeight(tw), Texture(t), Flags(f) {}
};

struct mesh_part
{
    material Material;
    size_t Offset;
    size_t Count;
    GLuint PrimitiveType;

    mesh_part() {}
    mesh_part(material m, size_t o, size_t c, GLuint p)
        : Material(m), Offset(o), Count(c), PrimitiveType(p) {}
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
    vector<material *> Materials;
    mesh *Mesh;
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

struct blur_program
{
    shader_program ShaderProgram;
    int TexelSize;
    int Orientation;
    int Amount;
    int Scale;
    int Strength;
    int Sampler;
};

struct fxaa_program
{
    shader_program ShaderProgram;
    int Resolution;
};

enum color_texture_type
{
    ColorTexture_SurfaceReflect,
    ColorTexture_Emit,
    ColorTexture_Count,
};

#define Framebuffer_HasDepth 0x1
#define Framebuffer_IsFloat  0x2
#define Framebuffer_Filtered 0x4
struct framebuffer
{
    uint32 Flags;
    uint32 Width, Height;
    size_t ColorTextureCount;
    GLuint ID;
    texture DepthTexture;
    texture ColorTextures[ColorTexture_Count];
};

struct renderable
{
    bounding_box Bounds;
    vec3 Position;
    vec3 Scale;
    vec3 Rotation;
    material *Material;
    size_t VertexOffset;
    size_t VertexCount;
    GLint VAO;
    GLuint PrimitiveType;
};

#define BloomBlurPassCount 3
struct render_state
{
    model_program ModelProgram;
    blur_program BlurProgram;
    fxaa_program FXAAProgram;
    shader_program BlendProgram;

    framebuffer SceneFramebuffer;
    framebuffer FXAAFramebuffer[2];
    framebuffer BlurHorizontalFramebuffer[BloomBlurPassCount];
    framebuffer BlurVerticalFramebuffer[BloomBlurPassCount];

    vertex_buffer SpriteBuffer;
    vertex_buffer ScreenBuffer;

    vector<renderable> Renderables;
    size_t RenderableCount;

    vector<size_t> FrameSolidRenderables;
    vector<size_t> FrameTransparentRenderables;

    uint32 Width;
    uint32 Height;

    GLint LastVAO;

#ifdef DRAFT_DEBUG
    vertex_buffer DebugBuffer;
#endif
};

#endif
