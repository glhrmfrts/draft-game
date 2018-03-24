#ifndef DRAFT_RENDER_H
#define DRAFT_RENDER_H

#define Color_white   color(1, 1, 1, 1)
#define Color_black   color(0, 0, 0, 1)
#define Color_blue    color(0, 0, 0.5, 1)
#define Color_green   color(0, 1, 0, 1)
#define Color_red     color(1, 0, 0, 1)
#define Color_gray    color(0.5, 0.5, 0.5, 1)
#define Color_yellow  color(1, 1, 0, 1)

#define DEFAULT_LINE_WIDTH 2

struct color_palette
{
    int Colors[5];
};

#define SPECIAL_YELLOW 0
#define SHIP_BLUE      0
#define SHIP_ORANGE    1
#define SHIP_RED       2

static color_palette FirstPalette = {{0x21042D, 0x6F0C74, 0x25006D, 0x0E00CA, 0xEE1F97}};
static color_palette SecondPalette = {{0x86008A, 0x4B0034, 0x2703EC, 0x00BDFF, 0xB8C9EE}};
static color_palette SpecialPalette = {{0xFCF195, 0, 0, 0, 0}};
static color_palette ShipPalette = { { 0x00BDFF, 0xFD6533, 0xFF0000, 0, 0 } };

inline static color
IntColor(int c, float alpha = 1.0f)
{
    int b = c & 0xFF;
    int g = (c >> 8) & 0xFF;
    int r = (c >> 16) & 0xFF;
    return color(r/255.0f, g/255.0f, b/255.0f, alpha);
}

struct mesh_vertex
{
	union
	{
		struct
		{
			vec3 Position;
			vec2 Uv;
			color Color;
			vec3 Normal;
		} Attributes;

		float Data[12];
	};

	~mesh_vertex() {}
};

struct vertex_buffer
{
    std::vector<float> Vertices;
	std::vector<int> Indices;
    GLuint VAO, VBO, IBO;
    size_t RawIndex;
    size_t VertexCount;
    size_t VertexSize;
	float *MappedData = NULL;
};

struct vertex_attribute
{
    GLuint Location;
    size_t Size;
    GLenum Type;
    size_t Stride;
    size_t Offset;
};

struct texture_filters
{
    GLint Min, Mag;
};

struct texture_wrap
{
    GLint WrapS, WrapT;
};

#define TextureFlag_Mipmap 0x1
#define TextureFlag_Anisotropic 0x2
#define TextureFlag_WrapRepeat 0x4
#define TextureFlag_Nearest 0x8
struct texture
{
    string Filename;
    GLuint ID = 0;
    GLuint Target;
    uint32 Width;
    uint32 Height;
    texture_filters Filters;
    texture_wrap Wrap;
    uint32 Flags = 0;
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
    vec3 Up = vec3{0,0,1};
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

#define MaterialFlag_PolygonLines     0x1
#define MaterialFlag_ForceTransparent 0x2
#define MaterialFlag_TransformUniform 0x4
struct material
{
    color DiffuseColor = Color_white;
    float Emission = 0;
    float TexWeight = 0;
    float FogWeight = 1;
    texture *Texture = NULL;
    uint32 Flags = 0;
    vec2 UvScale = vec2{ 1, 1 };

    material() {}
    material(color c, float e, float tw, texture *t, uint32 f = 0, vec2 us = vec2{ 1, 1 })
        : DiffuseColor(c), Emission(e), TexWeight(tw), Texture(t), Flags(f), UvScale(us) {}
};
static material BlankMaterial{Color_white, 0, 0, NULL};

struct model_program;

struct mesh_part
{
    material Material;
    size_t Offset;
    size_t Count;
    model_program *Program = NULL;
    GLuint PrimitiveType;
    float LineWidth = DEFAULT_LINE_WIDTH;

    mesh_part() {}
    mesh_part(material m, size_t o, size_t c, GLuint p, float lw = DEFAULT_LINE_WIDTH)
        : Material(m), Offset(o), Count(c), PrimitiveType(p), LineWidth(lw) {}
};

struct mesh_flags
{
    typedef uint32 type;

    static constexpr type HasUvs = 0x1;
    static constexpr type HasNormals = 0x2;
    static constexpr type HasColors = 0x4;
    static constexpr type HasIndices = 0x8;
};

struct mesh
{
    vertex_buffer Buffer;
    vec3 Min;
    vec3 Max;
    vector<mesh_part> Parts;
};

struct shader_program;
struct shader_asset_param;

typedef void shader_asset_callback_func(shader_asset_param *);

struct shader_asset_param
{
    GLint Type;
    string Path;
    shader_program *ShaderProgram;
    shader_asset_callback_func *Callback;
};

struct shader_program
{
    shader_asset_param VertexShaderParam;
    shader_asset_param FragmentShaderParam;
    GLuint ID = 0;
    GLuint VertexShader = 0;
    GLuint FragmentShader = 0;
};

struct model_program : shader_program
{
    shader_asset_param VertexShaderParam;
    shader_asset_param FragmentShaderParam;
    int ProjectionView;
    int Transform;
    int NormalTransform;
    int DiffuseColor;
    int TexWeight;
    int FogWeight;
    int Emission;
    int UvScale;
    int Sampler;
    int MaterialFlags;
    int ExplosionLightColor;
    int ExplosionLightTimer;
    int CamPos;
    int FogColor;
    int FogStart;
    int FogEnd;
	int BendRadius;
    int RoadTangentPoint;
};

struct blur_program : shader_program
{
    int PixelSize;
};

struct resolve_multisample_program : shader_program
{
    int SampleCount;
};

struct perlin_noise_program : shader_program
{
	int RandomSampler;
	int Time;
	int Offset;
	int Color;
};

enum color_texture_type
{
    ColorTextureFlag_SurfaceReflect,
    ColorTextureFlag_Emit,
    ColorTextureFlag_Count,
};

#define FramebufferFlag_HasDepth 0x1
#define FramebufferFlag_IsFloat  0x2
#define FramebufferFlag_Filtered 0x4
#define FramebufferFlag_Multisampled 0x8
struct framebuffer
{
    uint32 Flags;
    uint32 Width, Height;
    size_t ColorTextureCount;
    GLuint ID;
    texture DepthTexture;
    texture ColorTextures[ColorTextureFlag_Count];
};

struct transform
{
    vec3 Position;
    vec3 Velocity;
    vec3 Scale = vec3(1.0f);
    vec3 Rotation = vec3(0.0f);
};

struct renderable
{
    transform Transform;
    bounding_box Bounds;
    model_program *Program;
    material *Material;
    size_t VertexOffset;
    size_t VertexCount;
    int SortNumber;
    GLint VAO;
    GLuint PrimitiveType;
    GLfloat LineWidth;
};

#define BloomBlurPassCount 3
struct render_state
{
    memory_arena Arena;

    model_program ModelProgram;
    blur_program BlurHorizontalProgram;
    blur_program BlurVerticalProgram;
    shader_program BlendProgram;
    shader_program BlitProgram;
    resolve_multisample_program ResolveMultisampleProgram;
	perlin_noise_program PerlinNoiseProgram;

    framebuffer MultisampledSceneFramebuffer;
    framebuffer SceneFramebuffer;
    framebuffer BlurHorizontalFramebuffers[BloomBlurPassCount];
    framebuffer BlurVerticalFramebuffers[BloomBlurPassCount];

    vertex_buffer SpriteBuffer;
    vertex_buffer ScreenBuffer;

    std::vector<renderable> Renderables;
    size_t RenderableCount;

    fixed_array<size_t, 128> FrameSolidRenderables;
    fixed_array<size_t, 128> FrameTransparentRenderables;

    uint32 Width;
    uint32 Height;
	uint32 ViewportWidth;
	uint32 ViewportHeight;

    GLint MaxMultiSampleCount;
    GLint LastVAO;

    color FogColor;
    color ExplosionLightColor;
    float ExplosionLightTimer = 0;
	float DeltaTime = 0;
	float BendRadius = 500;

    float *RoadTangentPoint = NULL;

#ifdef DRAFT_DEBUG
    vertex_buffer DebugBuffer;
#endif
};

static void AddCubeWithRotation(vertex_buffer &Buffer, color c = Color_white,
                                bool NoLight = false, vec3 Center = vec3(0.0f),
                                vec3 Scale = vec3(1.0f), float Angle = 0.0f);

#endif
