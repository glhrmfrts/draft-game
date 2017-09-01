// Copyright

#include "shaders.cpp"

static void
EnableVertexAttribute(vertex_attribute Attr)
{
    glEnableVertexAttribArray(Attr.Location);
    glVertexAttribPointer(Attr.Location, Attr.Size, Attr.Type, false, Attr.Stride, (void *)Attr.Offset);
}

static void
vInitBuffer(vertex_buffer &Buffer, size_t AttrCount, va_list Args)
{
    glGenVertexArrays(1, &Buffer.VAO);
    glGenBuffers(1, &Buffer.VBO);

    glBindVertexArray(Buffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    for (size_t i = 0; i < AttrCount; i++)
    {
        vertex_attribute Attr = va_arg(Args, vertex_attribute);
        EnableVertexAttribute(Attr);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#define InitialVertexBufferSize 16
static void
EnsureCapacity(vertex_buffer &Buffer, size_t Size)
{
    if (Buffer.RawIndex + Size > Buffer.Vertices.size())
    {
        if (Buffer.Vertices.size() == 0)
        {
            Buffer.Vertices.resize(InitialVertexBufferSize * Buffer.VertexSize);
        }
        else
        {
            Buffer.Vertices.resize(Buffer.Vertices.size() * 2);
        }
    }
}

static void
ResetBuffer(vertex_buffer &Buffer)
{
    Buffer.VertexCount = 0;
    Buffer.RawIndex = 0;
}

// InitBuffer accepts a variadic number of vertex_attributes, it
// initializes the buffer and enable the passed attributes
static void
InitBuffer(vertex_buffer &Buffer, size_t VertexSize, size_t AttrCount, ...)
{
    Buffer.VertexSize = VertexSize;
    ResetBuffer(Buffer);

    va_list Args;
    va_start(Args, AttrCount);
    vInitBuffer(Buffer, AttrCount, Args);
    va_end(Args);
}

// PushVertex pushes a single vertex to the buffer, the vertex size
// is known by the VertexSize field
inline static void
PushVertex(vertex_buffer &Buffer, const vector<float> &Verts)
{
    EnsureCapacity(Buffer, Buffer.VertexSize);

    size_t Idx = Buffer.RawIndex;
    for (size_t i = 0; i < Buffer.VertexSize; i++) {
        Buffer.Vertices[Idx+i] = Verts[i];
    }
    Buffer.RawIndex += Buffer.VertexSize;
    Buffer.VertexCount++;
}

struct mesh_vertex
{
    vec3 Position;
    vec2 Uv;
    color Color;
    vec3 Normal;
};
inline static void
PushVertex(vertex_buffer &Buffer, const mesh_vertex &Vertex)
{
    vec3 p = Vertex.Position;
    vec2 uv = Vertex.Uv;
    color c = Vertex.Color;
    vec3 n = Vertex.Normal;
    PushVertex(Buffer, {p.x, p.y, p.z, uv.x, uv.y, c.r, c.g, c.b, c.a, n.x, n.y, n.z});
}

void UploadVertices(vertex_buffer &Buffer, GLenum Usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, Buffer.VertexCount * Buffer.VertexSize * sizeof(float), &Buffer.Vertices[0], Usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint CompileShader(const char *Source, GLint Type)
{
    GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &Source, NULL);
    glCompileShader(Shader);

    int Status = 0;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);
    if (Status == GL_FALSE)
    {
        int logLength = 0;
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &logLength);

        char *infoLog = new char[logLength + 1];
        glGetShaderInfoLog(Shader, logLength, NULL, infoLog);
        infoLog[logLength] = '\0';

        printf("failed to compile %s\n%s\n", Source, infoLog);
        exit(EXIT_FAILURE);
    }
    return Shader;
}

void CompileShaderProgram(shader_program &Prog, const char *VertexSource, const char *FragmentSource)
{
    GLuint VertexShader = CompileShader(VertexSource, GL_VERTEX_SHADER);
    GLuint FragmentShader = CompileShader(FragmentSource, GL_FRAGMENT_SHADER);

    Prog.ID = glCreateProgram();
    glAttachShader(Prog.ID, VertexShader);
    glAttachShader(Prog.ID, FragmentShader);
    glLinkProgram(Prog.ID);

    int Status;
    glGetProgramiv(Prog.ID, GL_LINK_STATUS, &Status);
    if (Status == GL_FALSE)
    {
      int LogLength = 0;
      glGetProgramiv(Prog.ID, GL_INFO_LOG_LENGTH, &LogLength);

      char *InfoLog = new char[LogLength + 1];
      glGetProgramInfoLog(Prog.ID, LogLength, NULL, InfoLog);
      InfoLog[LogLength] = '\0';

      printf("failed to link program: %s\n", InfoLog);
      exit(EXIT_FAILURE);
    }

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
}

void SetUniform(GLuint Location, int Value)
{
    glUniform1i(Location, Value);
}

void SetUniform(GLuint Location, float Value)
{
    glUniform1fv(Location, 1, &Value);
}

void SetUniform(GLuint Location, const vec2 &Value)
{
    glUniform2fv(Location, 1, &Value[0]);
}

void SetUniform(GLuint Location, const vec3 &Value)
{
    glUniform3fv(Location, 1, &Value[0]);
}

void SetUniform(GLuint Location, const vec4 &Value)
{
    glUniform4fv(Location, 1, &Value[0]);
}

#define MatrixRowMajor false
void SetUniform(GLuint Location, const mat4 &Value)
{
    glUniformMatrix4fv(Location, 1, MatrixRowMajor, (float *)&Value);
}

void Bind(shader_program &Prog)
{
    glUseProgram(Prog.ID);
}

void UnbindShaderProgram()
{
    glUseProgram(0);
}

void Bind(texture &Texture, int TextureUnit)
{
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindTexture(Texture.Target, Texture.ID);
}

void Unbind(texture &Texture, int TextureUnit)
{
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindTexture(Texture.Target, 0);
}

void ApplyTextureParameters(texture &Texture, int TextureUnit)
{
    if (!Texture.ID)
    {
        glGenTextures(1, &Texture.ID);
    }

    Bind(Texture, TextureUnit);
    glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, Texture.Filters.Min);
    glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, Texture.Filters.Mag);
    glTexParameteri(Texture.Target, GL_TEXTURE_WRAP_S, Texture.Wrap.WrapS);
    glTexParameteri(Texture.Target, GL_TEXTURE_WRAP_T, Texture.Wrap.WrapT);

    if (Texture.Mipmap)
    {
        glGenerateMipmap(Texture.Target);
    }

    Unbind(Texture, TextureUnit);
}

void UploadTexture(texture &Texture, GLenum SrcFormat, GLenum DstFormat, GLenum Type, uint8 *Data, int MaxMultiSampleCount = 0)
{
    if (Texture.Target == GL_TEXTURE_2D_MULTISAMPLE)
    {
        glTexImage2DMultisample(Texture.Target, MaxMultiSampleCount, SrcFormat,
                                Texture.Width, Texture.Height, GL_FALSE);
    }
    else
    {
        glTexImage2D(Texture.Target, 0, SrcFormat, Texture.Width, Texture.Height, 0, DstFormat, Type, Data);
    }
}

vector<texture_rect> SplitTexture(texture &Texture, int Width, int Height, bool FlipV = false)
{
    vector<texture_rect> Result;
    float tw = 1.0f / float(Texture.Width / Width);
    float th = 1.0f / float(Texture.Height / Height);
    for (int y = 0; y < Height; y++)
    {
        for (int x = 0; x < Width; x++)
        {
            float u = x * Width / (float)Texture.Width;
            float v = y * Height / (float)Texture.Height;

            texture_rect Rect;
            Rect.u = u + tw*0.01f; // bleeding correction
            Rect.v = v + th*0.01f;
            Rect.u2 = u + tw + tw*0.01f;
            Rect.v2 = v + th + th*0.01f;
            if (FlipV)
            {
                Rect.v = v + th;
                Rect.v2 = v;
            }
            Result.push_back(Rect);
        }
    }
    return Result;
}

void SetAnimation(animated_sprite &Sprite, const sprite_animation &Animation, bool Reset = false)
{
    assert(Animation.Frames.size());

    Sprite.Indices = Animation.Frames;
    Sprite.Interval = Animation.Interval;
    if (Reset)
    {
        Sprite.CurrentIndex = 0;
    }
}

void UpdateAnimation(animated_sprite &Sprite, float DeltaTime)
{
    Sprite.Timer += DeltaTime;
    if (Sprite.Timer >= Sprite.Interval)
    {
        Sprite.Timer = 0;

        int NextIndex = Sprite.CurrentIndex + 1;
        if (NextIndex >= (int)Sprite.Indices.size())
        {
            NextIndex = 0;
        }
        Sprite.CurrentIndex = NextIndex;
    }

    Sprite.CurrentFrame = &Sprite.Frames[Sprite.Indices[Sprite.CurrentIndex]];
}

void MakeCameraOrthographic(camera &Camera, float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    Camera.Type = Camera_orthographic;
    Camera.Near = Near;
    Camera.Far = Far;
    Camera.Ortho = {Left, Right, Bottom, Top};
}

void MakeCameraPerspective(camera &Camera, float Width, float Height, float Fov, float Near, float Far)
{
    Camera.Type = Camera_perspective;
    Camera.Near = Near;
    Camera.Far = Far;
    Camera.Perspective = {Width, Height, Fov};
}

void UpdateProjectionView(camera &Camera)
{
    Camera.Updated = true;
    Camera.Up = vec3(0, 0, 1);

    switch (Camera.Type)
    {
    case Camera_orthographic: {
        Camera.ProjectionView = glm::ortho(Camera.Ortho.Left,
                                            Camera.Ortho.Right,
                                            Camera.Ortho.Bottom,
                                            Camera.Ortho.Top,
                                            Camera.Near,
                                            Camera.Far);
        break;
    }

    case Camera_perspective: {
        auto &Persp = Camera.Perspective;
        Camera.Projection = glm::perspective(glm::radians(Persp.Fov), Persp.Width / Persp.Height, Camera.Near, Camera.Far);
        Camera.View = glm::lookAt(Camera.Position, Camera.LookAt, Camera.Up);
        //Camera.View = glm::rotate(Camera.View, 90.0f, vec3(1.0f, 0.0f, 0.0f));
        Camera.ProjectionView = Camera.Projection * Camera.View;
        break;
    }
    }
}

#define OPENGL_DEPTH_COMPONENT_TYPE GL_DEPTH_COMPONENT32F

static GLenum FramebufferColorAttachments[] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4,
    GL_COLOR_ATTACHMENT5,
    GL_COLOR_ATTACHMENT6,
    GL_COLOR_ATTACHMENT7,
    GL_COLOR_ATTACHMENT8,
    GL_COLOR_ATTACHMENT9,
    GL_COLOR_ATTACHMENT10,
    GL_COLOR_ATTACHMENT11,
    GL_COLOR_ATTACHMENT12,
    GL_COLOR_ATTACHMENT13,
    GL_COLOR_ATTACHMENT14,
    GL_COLOR_ATTACHMENT15,
};

static void
CreateFramebufferTexture(render_state &RenderState, texture &Texture, GLuint Target, uint32 Width, uint32 Height, GLint Filter, GLuint Format)
{
    Texture.Target = Target;
    Texture.Width = Width;
    Texture.Height = Height;
    Texture.Filters = {Filter, Filter};
    Texture.Wrap = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
    ApplyTextureParameters(Texture, 0);
    Bind(Texture, 0);
    UploadTexture(Texture, Format,
                  (Format == OPENGL_DEPTH_COMPONENT_TYPE) ? GL_DEPTH_COMPONENT : GL_BGRA_EXT,
                  GL_UNSIGNED_BYTE, NULL,
                  RenderState.MaxMultiSampleCount);
    Unbind(Texture, 0);
}

static void
InitFramebuffer(render_state &RenderState, framebuffer &Framebuffer, uint32 Width, uint32 Height, uint32 Flags, size_t ColorTextureCount)
{
    Framebuffer.Width = Width;
    Framebuffer.Height = Height;
    Framebuffer.Flags = Flags;
    Framebuffer.ColorTextureCount = ColorTextureCount;

    glGenFramebuffers(1, &Framebuffer.ID);
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.ID);

    bool Multisampled = Flags & Framebuffer_Multisampled;
    GLuint Target = Multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    GLint Filter = (Flags & Framebuffer_Filtered) ? GL_LINEAR : GL_NEAREST;
    GLuint Format = (Flags & Framebuffer_IsFloat) ? GL_RGBA16F : GL_RGBA8;
    for (size_t i = 0; i < ColorTextureCount; i++)
    {
        CreateFramebufferTexture(RenderState, Framebuffer.ColorTextures[i], Target, Width, Height, Filter, Format);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, Target, Framebuffer.ColorTextures[i].ID, 0);
    }
    glDrawBuffers(ColorTextureCount, FramebufferColorAttachments);

    if (Flags & Framebuffer_HasDepth)
    {
        CreateFramebufferTexture(RenderState, Framebuffer.DepthTexture, Target, Width, Height, Filter, OPENGL_DEPTH_COMPONENT_TYPE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Target, Framebuffer.DepthTexture.ID, 0);
    }

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    printf("0x%x\n", Status);
    assert(Status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void
DestroyFramebuffer(framebuffer &Framebuffer)
{
    //DestroyTexture(Framebuffer.DepthTexture);
    for (size_t i = 0; i < Framebuffer.ColorTextureCount; i++)
    {
        //DestroyTexture(Framebuffer.ColorTextures[i]);
    }
    glDeleteFramebuffers(1, &Framebuffer.ID);
}

static void
BindFramebuffer(framebuffer &Framebuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.ID);
    glViewport(0, 0, Framebuffer.Width, Framebuffer.Height);
}

static void
UnbindFramebuffer(render_state &RenderState)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, RenderState.Width, RenderState.Height);
}

#define MeshVertexSize 12

// InitMeshBuffer initializes a vertex_buffer with the common attributes
// used by all meshes in the game
void InitMeshBuffer(vertex_buffer &Buffer)
{
    size_t Stride = MeshVertexSize*sizeof(float);
    InitBuffer(Buffer, 12, 4,
               vertex_attribute({0, 3, GL_FLOAT, Stride, 0}), // position
               vertex_attribute({1, 2, GL_FLOAT, Stride, 3*sizeof(float)}), // uv
               vertex_attribute({2, 4, GL_FLOAT, Stride, 5*sizeof(float)}), // color
               vertex_attribute({3, 3, GL_FLOAT, Stride, 9*sizeof(float)})); // normal
}

void EndMesh(mesh &Mesh, GLenum Usage, bool ComputeBounds = true)
{
    auto &b = Mesh.Buffer;
    UploadVertices(b, Usage);

    if (!ComputeBounds) return;

    // calculate bounding box
    vec3 Min(0.0f);
    vec3 Max(0.0f);
    for (size_t i = 0; i < b.VertexCount; i++)
    {
        size_t VertexIndex = i * MeshVertexSize;
        vec3 Pos = {b.Vertices[VertexIndex], b.Vertices[VertexIndex+1], b.Vertices[VertexIndex+2]};

        Min.x = std::min(Min.x, Pos.x);
        Min.y = std::min(Min.y, Pos.y);
        Min.z = std::min(Min.z, Pos.z);

        Max.x = std::max(Max.x, Pos.x);
        Max.y = std::max(Max.y, Pos.y);
        Max.z = std::max(Max.z, Pos.z);
    }

    Mesh.Min = Min;
    Mesh.Max = Max;
}

static void
CompileModelProgram(model_program &Program)
{
    CompileShaderProgram(Program.ShaderProgram, ModelVertexShader.c_str(), ModelFragmentShader.c_str());
    Program.MaterialFlags = glGetUniformLocation(Program.ShaderProgram.ID, "u_MaterialFlags");
    Program.ProjectionView = glGetUniformLocation(Program.ShaderProgram.ID, "u_ProjectionView");
    Program.Transform = glGetUniformLocation(Program.ShaderProgram.ID, "u_Transform");
    Program.NormalTransform = glGetUniformLocation(Program.ShaderProgram.ID, "u_NormalTransform");
    Program.DiffuseColor = glGetUniformLocation(Program.ShaderProgram.ID, "u_DiffuseColor");
    Program.TexWeight = glGetUniformLocation(Program.ShaderProgram.ID, "u_TexWeight");
    Program.Emission = glGetUniformLocation(Program.ShaderProgram.ID, "u_Emission");
    Program.Sampler = glGetUniformLocation(Program.ShaderProgram.ID, "u_Sampler");

    Bind(Program.ShaderProgram);
    SetUniform(Program.Sampler, 0);
    UnbindShaderProgram();
}

static void
CompileBlurProgram(blur_program &Program)
{
    CompileShaderProgram(Program.ShaderProgram, BlitVertexShader.c_str(), BlurFragmentShader.c_str());
    Program.Orientation = glGetUniformLocation(Program.ShaderProgram.ID, "u_orientation");
    Program.Offset = glGetUniformLocation(Program.ShaderProgram.ID, "u_offset");

    Bind(Program.ShaderProgram);

    vec3 Kernel = {5, 6, 5};
    SetUniform(glGetUniformLocation(Program.ShaderProgram.ID, "u_coefficients"), glm::normalize(Kernel));
    SetUniform(glGetUniformLocation(Program.ShaderProgram.ID, "u_sampler"), 0);

    UnbindShaderProgram();
}

static void
CompileBlendProgram(shader_program &Program)
{
    CompileShaderProgram(Program, BlitVertexShader.c_str(), BlendFragmentShader.c_str());
    Bind(Program);
    for (int i = 0; i < BloomBlurPassCount; i++)
    {
        char Name[] = "u_Pass#";
        Name[7] = (char)('0' + i);
        SetUniform(glGetUniformLocation(Program.ID, Name), 0);
    }
    SetUniform(glGetUniformLocation(Program.ID, "u_Scene"), BloomBlurPassCount);
    UnbindShaderProgram();
}

static void
CompileBlitProgram(shader_program &Program)
{
    CompileShaderProgram(Program, BlitVertexShader.c_str(), BlitFragmentShader.c_str());
    Bind(Program);
    SetUniform(glGetUniformLocation(Program.ID, "u_Sampler"), 0);
    UnbindShaderProgram();
}

static void
CompileResolveMultisampleProgram(resolve_multisample_program &Program)
{
    CompileShaderProgram(Program.ShaderProgram, BlitVertexShader.c_str(), ResolveMultisampleFragmentShader.c_str());
    Program.SampleCount = glGetUniformLocation(Program.ShaderProgram.ID, "u_SampleCount");

    Bind(Program.ShaderProgram);
    SetUniform(glGetUniformLocation(Program.ShaderProgram.ID, "u_SurfaceReflectSampler"), 0);
    SetUniform(glGetUniformLocation(Program.ShaderProgram.ID, "u_EmitSampler"), 1);
    UnbindShaderProgram();
}

// returns the index of the next available renderable
static size_t
NextRenderable(render_state &RenderState)
{
    size_t Size = RenderState.Renderables.size();
    if (RenderState.RenderableCount >= Size)
    {
        size_t NewSize = Size*2;
        if (!NewSize)
        {
            NewSize = 4;
        }
        RenderState.Renderables.resize(NewSize);
    }
    return RenderState.RenderableCount++;
}

void InitRenderState(render_state &RenderState, uint32 Width, uint32 Height)
{
    glLineWidth(2);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &RenderState.MaxMultiSampleCount);
    if (RenderState.MaxMultiSampleCount > 8)
    {
        RenderState.MaxMultiSampleCount = 8;
    }

    RenderState.Width = Width;
    RenderState.Height = Height;

    CompileModelProgram(RenderState.ModelProgram);
    CompileBlurProgram(RenderState.BlurProgram);
    CompileBlendProgram(RenderState.BlendProgram);
    CompileBlitProgram(RenderState.BlitProgram);
    CompileResolveMultisampleProgram(RenderState.ResolveMultisampleProgram);

    InitMeshBuffer(RenderState.SpriteBuffer);
    InitBuffer(RenderState.ScreenBuffer, 4, 2,
               vertex_attribute({0, 2, GL_FLOAT, 4*sizeof(float), 0}),
               vertex_attribute({1, 2, GL_FLOAT, 4*sizeof(float), 2*sizeof(float)}));
    PushVertex(RenderState.ScreenBuffer, {-1, -1, 0, 0});
    PushVertex(RenderState.ScreenBuffer, {1, -1, 1, 0});
    PushVertex(RenderState.ScreenBuffer, {-1, 1, 0, 1});
    PushVertex(RenderState.ScreenBuffer, {1, -1, 1, 0});
    PushVertex(RenderState.ScreenBuffer, {1, 1, 1, 1});
    PushVertex(RenderState.ScreenBuffer, {-1, 1, 0, 1});
    UploadVertices(RenderState.ScreenBuffer, GL_STATIC_DRAW);

    InitFramebuffer(RenderState, RenderState.MultisampledSceneFramebuffer, Width, Height, Framebuffer_HasDepth | Framebuffer_Multisampled, ColorTexture_Count);
    InitFramebuffer(RenderState, RenderState.SceneFramebuffer, Width, Height, Framebuffer_HasDepth, ColorTexture_Count);
    for (int i = 0; i < BloomBlurPassCount; i++)
    {
        // @TODO: maybe it is not necessary to scale down
        size_t BlurWidth = Width >> i;
        size_t BlurHeight = Height >> i;

        InitFramebuffer(RenderState, RenderState.BlurHorizontalFramebuffers[i], BlurWidth, BlurHeight, Framebuffer_Filtered, 1);
        InitFramebuffer(RenderState, RenderState.BlurVerticalFramebuffers[i], BlurWidth, BlurHeight, Framebuffer_Filtered, 1);
    }

#ifdef DRAFT_DEBUG
    InitMeshBuffer(RenderState.DebugBuffer);
#endif
}

void RenderBegin(render_state &RenderState)
{
    RenderState.LastVAO = -1;
    RenderState.RenderableCount = 0;
    RenderState.FrameSolidRenderables.clear();
    RenderState.FrameTransparentRenderables.clear();

#ifdef DRAFT_DEBUG
    ResetBuffer(RenderState.DebugBuffer);
#endif
}

static void
RenderRenderable(render_state &RenderState, camera &Camera, renderable &r)
{
    if (RenderState.LastVAO != r.VAO)
    {
        glBindVertexArray(RenderState.LastVAO = r.VAO);
    }

    auto &Program = RenderState.ModelProgram;
    Bind(Program.ShaderProgram);
    SetUniform(Program.ProjectionView, Camera.ProjectionView);

    mat4 TransformMatrix = glm::translate(mat4(1.0f), r.Position);
    TransformMatrix = glm::scale(TransformMatrix, r.Scale);
    if (r.Rotation.x != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(r.Rotation.x), vec3(1,0,0));
    }
    if (r.Rotation.y != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(r.Rotation.y), vec3(0,1,0));
    }
    if (r.Rotation.z != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(r.Rotation.z), vec3(0,0,1));
    }
    SetUniform(Program.Transform, TransformMatrix);

    // @TODO: check why this breaks the floor model
    SetUniform(Program.NormalTransform, mat4(1.0f));

    if (r.Material->Texture)
    {
        Bind(*r.Material->Texture, 0);
    }
    if (r.Material->Flags & Material_PolygonLines)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    SetUniform(Program.MaterialFlags, (int)r.Material->Flags);
    SetUniform(Program.DiffuseColor, r.Material->DiffuseColor);
    SetUniform(Program.TexWeight, r.Material->TexWeight);
    SetUniform(Program.Emission, r.Material->Emission);
    glDrawArrays(r.PrimitiveType, r.VertexOffset, r.VertexCount);
    if (r.Material->Flags & Material_PolygonLines)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (r.Material->Texture)
    {
        Unbind(*r.Material->Texture, 0);
    }
}

enum blur_orientation
{
    Blur_Horizontal,
    Blur_Vertical,
};
static framebuffer *
RenderBlur(render_state &RenderState, blur_program &Program, blur_orientation Orientation)
{
    framebuffer *Sources = RenderState.BlurHorizontalFramebuffers;
    framebuffer *Dests = RenderState.BlurVerticalFramebuffers;
    if (Orientation == Blur_Vertical)
    {
        auto Tmp = Sources;
        Sources = Dests;
        Dests = Tmp;
    }
    SetUniform(Program.Orientation, int(Orientation));
    for (int i = 0; i < BloomBlurPassCount; i++)
    {
        SetUniform(Program.Offset, 1.2f / float(Sources[i].Width));
        BindFramebuffer(Dests[i]);

        glClear(GL_COLOR_BUFFER_BIT);
        Bind(Sources[i].ColorTextures[0], 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        Unbind(Sources[i].ColorTextures[0], 0);

        UnbindFramebuffer(RenderState);
    }
    return Dests;
}

static material BlankMaterial{Color_white, 0, 0, NULL};

void RenderEnd(render_state &RenderState, camera &Camera)
{
    assert(Camera.Updated);

#ifdef DRAFT_DEBUG
    // push debug renderable
    {
        size_t i = NextRenderable(RenderState);
        auto &r = RenderState.Renderables[i];
        r.VAO = RenderState.DebugBuffer.VAO;
        r.VertexOffset = 0;
        r.VertexCount = RenderState.DebugBuffer.VertexCount;
        r.Material = &BlankMaterial;
        r.Position = vec3(0.0f);
        r.Scale = vec3(1.0f);
        r.Rotation = vec3(0.0f);
        r.PrimitiveType = GL_LINES;
        RenderState.FrameSolidRenderables.push_back(i);
        UploadVertices(RenderState.DebugBuffer, GL_DYNAMIC_DRAW);
    }
#endif

    if (Global_Renderer_DoPostFX)
    {
        BindFramebuffer(RenderState.MultisampledSceneFramebuffer);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    for (auto RenderableIndex : RenderState.FrameSolidRenderables)
    {
        RenderRenderable(RenderState, Camera, RenderState.Renderables[RenderableIndex]);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto RenderableIndex : RenderState.FrameTransparentRenderables)
    {
        RenderRenderable(RenderState, Camera, RenderState.Renderables[RenderableIndex]);
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    if (Global_Renderer_DoPostFX)
    {
        UnbindFramebuffer(RenderState);
        glBindVertexArray(RenderState.ScreenBuffer.VAO);

        // resolve multisample
        BindFramebuffer(RenderState.SceneFramebuffer);
        Bind(RenderState.ResolveMultisampleProgram.ShaderProgram);
        SetUniform(RenderState.ResolveMultisampleProgram.SampleCount, RenderState.MaxMultiSampleCount);
        for (size_t i = 0; i < ColorTexture_Count; i++)
        {
            Bind(RenderState.MultisampledSceneFramebuffer.ColorTextures[i], i);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);
        UnbindFramebuffer(RenderState);

        // downsample scene for blur
        Bind(RenderState.BlitProgram);
        Bind(RenderState.SceneFramebuffer.ColorTextures[ColorTexture_Emit], 0);
        for (int i = 0; i < BloomBlurPassCount; i++)
        {
            BindFramebuffer(RenderState.BlurHorizontalFramebuffers[i]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        UnbindFramebuffer(RenderState);
        UnbindShaderProgram();

        // perform blur
        Bind(RenderState.BlurProgram.ShaderProgram);      
        RenderBlur(RenderState, RenderState.BlurProgram, Blur_Horizontal);
        framebuffer *BlurOutput = RenderBlur(RenderState, RenderState.BlurProgram, Blur_Vertical);

        // blend scene with blur results
        Bind(RenderState.BlendProgram);
        for (int i = 0; i < BloomBlurPassCount; i++)
        {
            Bind(BlurOutput[i].ColorTextures[0], i);
        }
        Bind(RenderState.SceneFramebuffer.ColorTextures[ColorTexture_SurfaceReflect], BloomBlurPassCount);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        UnbindShaderProgram();
    }
    // @TODO: cleanup textures
}

void PushModel(render_state &RenderState, model &Model, const vec3 &Position, const vec3 &Scale, const vec3 &Rotation)
{
    mesh *Mesh = Model.Mesh;
    for (auto &Part : Mesh->Parts)
    {
        size_t i = &Part - &Mesh->Parts[0];
        auto Material = &Part.Material;
        if (i < Model.Materials.size() && Model.Materials[i])
        {
            Material = Model.Materials[i];
        }

        size_t Index = NextRenderable(RenderState);
        auto &r = RenderState.Renderables[Index];
        r.PrimitiveType = Part.PrimitiveType;
        r.VertexOffset = Part.Offset;
        r.VertexCount = Part.Count;
        r.VAO = Mesh->Buffer.VAO;
        r.Material = Material;
        r.Position = Position;
        r.Scale = Scale;
        r.Rotation = Rotation;
        r.Bounds = BoundsFromMinMax(Mesh->Min*Scale, Mesh->Max*Scale);
        r.Bounds.Center += r.Position;
        if (Material->DiffuseColor.a < 1.0f)
        {
            RenderState.FrameTransparentRenderables.push_back(Index);
        }
        else
        {
            RenderState.FrameSolidRenderables.push_back(Index);
        }
    }
}

#ifdef DRAFT_DEBUG
void PushDebugBounds(render_state &RenderState, const bounding_box &Box, bool Colliding)
{
    color Color = Color_green;
    if (Colliding)
    {
        Color = Color_red;
    }
    vec3 a = Box.Center - Box.Half;
    vec3 b = Box.Center + Box.Half;

    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});

    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});

    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, a.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{b.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, a.y, b.z}, {0, 0}, Color, {0, 0, 0}});
    PushVertex(RenderState.DebugBuffer, {{a.x, b.y, b.z}, {0, 0}, Color, {0, 0, 0}});
}
#endif

#if 0
void RenderSprite(render_state &RenderState, camera &Camera, animated_sprite &Sprite, vec3 Position)
{
    assert(Camera.Updated);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(RenderState.SpriteBuffer.VAO);

    auto &Program = RenderState.ModelProgram;
    Bind(*Sprite.Texture, 0);
    Bind(Program.ShaderProgram);
    SetUniform(Program.ProjectionView, Camera.ProjectionView);
    SetUniform(Program.Transform, mat4(1.0));
    SetUniform(Program.TexWeight, 1.0f);
    SetUniform(Program.DiffuseColor, Color_white);
    SetUniform(Program.Emission, 0.0f);

    auto Frame = Sprite.CurrentFrame;
    assert(Frame);

    vec3 Scale = Sprite.Scale;
    mat4 InverseViewMatrix = glm::inverse(Camera.View);
    vec4 Right4 = InverseViewMatrix * vec4(1, 0, 0, 0);
    vec4 Up4 = InverseViewMatrix * vec4(0, 1, 0, 0);
    vec3 Right = vec3(Right4);
    vec3 Up = vec3(Up4);

    Position += Sprite.Offset;
    vec3 a = Position - ((Right + Up) * Scale);
    vec3 b = Position + ((Right - Up) * Scale);
    vec3 c = Position + ((Right + Up) * Scale);
    vec3 d = Position - ((Right - Up) * Scale);

    float u = Frame->u;
    float v = Frame->v;
    float u2 = Frame->u2;
    float v2 = Frame->v2;
    if (Sprite.Direction == Direction_left)
    {
        u = u2;
        u2 = Frame->u;
    }
    ResetBuffer(RenderState.SpriteBuffer);
    PushVertex(RenderState.SpriteBuffer, {a.x, a.y, a.z, u, v2,  1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.SpriteBuffer, {b.x, b.y, b.z, u2, v2,  1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.SpriteBuffer, {d.x, d.y, d.z, u, v,  1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.SpriteBuffer, {b.x, b.y, b.z, u2, v2,  1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.SpriteBuffer, {c.x, c.y, c.z, u2, v,  1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.SpriteBuffer, {d.x, d.y, d.z, u, v,  1, 1, 1, 1, 0, 0, 0});
    UploadVertices(RenderState.SpriteBuffer, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, RenderState.SpriteBuffer.VertexCount);

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    Unbind(*Sprite.Texture, 0);
    UnbindShaderProgram();
}
#endif
