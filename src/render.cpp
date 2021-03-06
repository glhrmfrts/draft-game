// Copyright

#include "uber_shader.h"

static void EnableVertexAttribute(vertex_attribute Attr)
{
    glEnableVertexAttribArray(Attr.Location);
    glVertexAttribPointer(Attr.Location, Attr.Size, Attr.Type, false, Attr.Stride, (void *)Attr.Offset);
}

void ResetBuffer(vertex_buffer &Buffer, size_t Index = 0)
{
    Buffer.VertexCount = Index;
    Buffer.RawIndex = Index * Buffer.VertexSize;
}

// InitBuffer accepts a variadic number of vertex_attributes, it
// initializes the buffer and enable the passed attributes
void InitBuffer(vertex_buffer &buffer, size_t vertexSize, const std::vector<vertex_attribute> &attributes)
{
	buffer.VertexSize = vertexSize;
	ResetBuffer(buffer);

	glGenVertexArrays(1, &buffer.VAO);
	glGenBuffers(1, &buffer.VBO);

	glBindVertexArray(buffer.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.VBO);
	for (size_t i = 0; i < attributes.size(); i++)
	{
		EnableVertexAttribute(attributes[i]);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// InitMeshBuffer initializes a vertex_buffer with the common attributes
// used by all meshes in the game
void InitMeshBuffer(vertex_buffer &Buffer)
{
    size_t Stride = sizeof(mesh_vertex);
	InitBuffer(Buffer, 12, {
			   vertex_attribute{0, 3, GL_FLOAT, Stride, 0}, // position
			   vertex_attribute{1, 2, GL_FLOAT, Stride, 3 * sizeof(float)}, // uv
			   vertex_attribute{2, 4, GL_FLOAT, Stride, 5 * sizeof(float)}, // color
			   vertex_attribute{3, 3, GL_FLOAT, Stride, 9 * sizeof(float)} }); // normal
}

#define InitialVertexBufferSize 16
static void EnsureCapacity(vertex_buffer &Buffer, size_t Size)
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

inline void SetIndices(vertex_buffer &buf, std::vector<uint32> &indices)
{
	buf.IsIndexed = true;
    glGenBuffers(1, &buf.IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), &indices[0], GL_STATIC_DRAW);
}

// PushVertex pushes a single vertex to the buffer, the vertex size
// is known by the VertexSize field
inline size_t PushVertex(vertex_buffer &buf, const float *data)
{
    EnsureCapacity(buf, buf.VertexSize);

    size_t Idx = buf.RawIndex;
    for (size_t i = 0; i < buf.VertexSize; i++) {
		buf.Vertices[Idx+i] = data[i];
    }
	buf.RawIndex += buf.VertexSize;
	buf.VertexCount++;
    return buf.VertexCount - 1;
}

inline size_t PushVertex(vertex_buffer &buf, const mesh_vertex &vertex)
{
    return PushVertex(buf, vertex.Data);
}

inline float *MapBuffer(vertex_buffer &buf, GLenum usage)
{
	if (buf.MappedData != NULL) return buf.MappedData;

	glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
	buf.MappedData = (float *)glMapBuffer(GL_ARRAY_BUFFER, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return buf.MappedData;
}

inline void UnmapBuffer(vertex_buffer &buf)
{
	glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	buf.MappedData = NULL;
}

void UploadVertices(vertex_buffer &Buffer, GLenum Usage)
{
    void *Data = NULL;
    if (Buffer.VertexCount > 0)
    {
        Data = &Buffer.Vertices[0];
    }
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, Buffer.VertexCount * Buffer.VertexSize * sizeof(float), Data, Usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UploadVertices(vertex_buffer &Buffer, size_t Index, size_t Count)
{
    if (Count == 0) return;

    size_t ByteIndex = Index * Buffer.VertexSize * sizeof(float);
    size_t ByteCount = Count * Buffer.VertexSize * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, ByteIndex, ByteCount, &Buffer.Vertices[Index * Buffer.VertexSize]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ReserveVertices(vertex_buffer &Buffer, size_t Size, GLenum Usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, Size * Buffer.VertexSize * sizeof(float), NULL, Usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void CompileShader(GLuint Shader, const char *Source)
{
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
}

static void LinkShaderProgram(shader_program &Prog)
{
    if (!Prog.ID)
    {
        Prog.ID = glCreateProgram();
    }
    glAttachShader(Prog.ID, Prog.VertexShader);
    glAttachShader(Prog.ID, Prog.FragmentShader);
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

    glDeleteShader(Prog.VertexShader);
    glDeleteShader(Prog.FragmentShader);
}

inline void SetUniform(GLuint Location, int Value)
{
    glUniform1i(Location, Value);
}

inline void SetUniform(GLuint Location, float Value)
{
    glUniform1fv(Location, 1, &Value);
}

inline void SetUniform(GLuint Location, const vec2 &Value)
{
    glUniform2fv(Location, 1, &Value[0]);
}

inline void SetUniform(GLuint Location, const vec3 &Value)
{
    glUniform3fv(Location, 1, &Value[0]);
}

inline void SetUniform(GLuint Location, const vec4 &Value)
{
    glUniform4fv(Location, 1, &Value[0]);
}

#define MatrixRowMajor false
inline void SetUniform(GLuint Location, const mat4 &Value)
{
    glUniformMatrix4fv(Location, 1, MatrixRowMajor, (float *)&Value);
}

inline void Bind(shader_program &Prog)
{
    glUseProgram(Prog.ID);
}

inline void UnbindShaderProgram()
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
    assert(Texture.ID);

    Bind(Texture, TextureUnit);
    glTexParameteri(Texture.Target, GL_TEXTURE_MIN_FILTER, Texture.Filters.Min);
    glTexParameteri(Texture.Target, GL_TEXTURE_MAG_FILTER, Texture.Filters.Mag);
    glTexParameteri(Texture.Target, GL_TEXTURE_WRAP_S, Texture.Wrap.WrapS);
    glTexParameteri(Texture.Target, GL_TEXTURE_WRAP_T, Texture.Wrap.WrapT);

    if (Texture.Flags & TextureFlag_Anisotropic)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(Texture.Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    if (Texture.Flags & TextureFlag_Mipmap)
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

static void CreateFramebufferTexture(render_state &RenderState, texture &Texture, GLuint Target, uint32 Width, uint32 Height, GLint Filter, GLuint Format)
{
    Texture.Target = Target;
    Texture.Width = Width;
    Texture.Height = Height;
    Texture.Filters = {Filter, Filter};
    Texture.Wrap = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
    glGenTextures(1, &Texture.ID);
    ApplyTextureParameters(Texture, 0);
    Bind(Texture, 0);
    UploadTexture(Texture, Format,
                  (Format == OPENGL_DEPTH_COMPONENT_TYPE) ? GL_DEPTH_COMPONENT : GL_BGRA_EXT,
                  GL_UNSIGNED_BYTE, NULL,
                  RenderState.MaxMultiSampleCount);
    Unbind(Texture, 0);
}

static void InitFramebuffer(render_state &RenderState, framebuffer &Framebuffer, uint32 Width, uint32 Height, uint32 Flags, size_t ColorTextureCount)
{
    Framebuffer.Width = Width;
    Framebuffer.Height = Height;
    Framebuffer.Flags = Flags;
    Framebuffer.ColorTextureCount = ColorTextureCount;

    glGenFramebuffers(1, &Framebuffer.ID);
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.ID);

    bool Multisampled = Flags & FramebufferFlag_Multisampled;
    GLuint Target = Multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    GLint Filter = (Flags & FramebufferFlag_Filtered) ? GL_LINEAR : GL_NEAREST;
    GLuint Format = (Flags & FramebufferFlag_IsFloat) ? GL_RGBA16F : GL_RGBA8;
    for (size_t i = 0; i < ColorTextureCount; i++)
    {
        CreateFramebufferTexture(RenderState, Framebuffer.ColorTextures[i], Target, Width, Height, Filter, Format);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, Target, Framebuffer.ColorTextures[i].ID, 0);
    }
    glDrawBuffers(ColorTextureCount, FramebufferColorAttachments);

    if (Flags & FramebufferFlag_HasDepth)
    {
        CreateFramebufferTexture(RenderState, Framebuffer.DepthTexture, Target, Width, Height, Filter, OPENGL_DEPTH_COMPONENT_TYPE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Target, Framebuffer.DepthTexture.ID, 0);
    }

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    printf("%x\n", Status);
	if (Status != GL_FRAMEBUFFER_COMPLETE)
	{
		assert(!(Status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT));
		assert(Status == GL_FRAMEBUFFER_COMPLETE);
	}

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void DestroyFramebuffer(framebuffer &Framebuffer)
{
    //DestroyTexture(Framebuffer.DepthTexture);
    for (size_t i = 0; i < Framebuffer.ColorTextureCount; i++)
    {
        //DestroyTexture(Framebuffer.ColorTextures[i]);
    }
    glDeleteFramebuffers(1, &Framebuffer.ID);
}

static void BindFramebuffer(framebuffer &Framebuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.ID);
    glViewport(0, 0, Framebuffer.Width, Framebuffer.Height);
}

static void UnbindFramebuffer(render_state &rs)
{
    // @TODO: un-hardcode this
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, rs.ViewportWidth, rs.ViewportHeight);
}

void EndMesh(mesh *Mesh, GLenum Usage, bool ComputeBounds = true)
{
    auto &b = Mesh->Buffer;
    UploadVertices(b, Usage);

    if (!ComputeBounds) return;

    // calculate bounding box
    vec3 Min(0.0f);
    vec3 Max(0.0f);
    for (size_t i = 0; i < b.VertexCount; i++)
    {
		size_t VertexIndex = i * Mesh->Buffer.VertexSize;
        vec3 Pos = {b.Vertices[VertexIndex], b.Vertices[VertexIndex+1], b.Vertices[VertexIndex+2]};

        Min.x = std::min(Min.x, Pos.x);
        Min.y = std::min(Min.y, Pos.y);
        Min.z = std::min(Min.z, Pos.z);

        Max.x = std::max(Max.x, Pos.x);
        Max.y = std::max(Max.y, Pos.y);
        Max.z = std::max(Max.z, Pos.z);
    }

    Mesh->Min = Min;
    Mesh->Max = Max;
}

// returns the index of the next available renderable
static size_t NextRenderable(render_state &RenderState)
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

#define IsLinkable(Program) (Program->VertexShader && Program->FragmentShader)

static void ModelProgramCallback(shader_asset_param *p)
{
    auto *program = (model_program *)p->ShaderProgram;
    if (IsLinkable(program))
    {
        LinkShaderProgram(*program);
        program->MaterialFlags = glGetUniformLocation(program->ID, "u_MaterialFlags");
        program->ProjectionView = glGetUniformLocation(program->ID, "u_ProjectionView");
        program->Transform = glGetUniformLocation(program->ID, "u_Transform");
        program->NormalTransform = glGetUniformLocation(program->ID, "u_NormalTransform");
        program->DiffuseColor = glGetUniformLocation(program->ID, "u_DiffuseColor");
        program->TexWeight = glGetUniformLocation(program->ID, "u_TexWeight");
        program->FogWeight = glGetUniformLocation(program->ID, "u_FogWeight");
        program->Emission = glGetUniformLocation(program->ID, "u_Emission");
        program->UvScale = glGetUniformLocation(program->ID, "u_UvScale");
        program->Sampler = glGetUniformLocation(program->ID, "u_Sampler");
        program->ExplosionLightColor = glGetUniformLocation(program->ID, "u_ExplosionLightColor");
        program->ExplosionLightTimer = glGetUniformLocation(program->ID, "u_ExplosionLightTimer");
        program->CamPos = glGetUniformLocation(program->ID, "u_CamPos");
        program->FogColor = glGetUniformLocation(program->ID, "u_FogColor");
        program->FogStart = glGetUniformLocation(program->ID, "u_FogStart");
        program->FogEnd = glGetUniformLocation(program->ID, "u_FogEnd");
		program->BendRadius = glGetUniformLocation(program->ID, "u_BendRadius");
        program->RoadTangentPoint = glGetUniformLocation(program->ID, "u_RoadTangentPoint");
		program->SpecularColor = glGetUniformLocation(program->ID, "u_SpecularColor");
		program->EmissiveColor = glGetUniformLocation(program->ID, "u_EmissiveColor");
		program->Shininess = glGetUniformLocation(program->ID, "u_Shininess");

        Bind(*program);
        SetUniform(program->Sampler, 0);
        UnbindShaderProgram();
    }
}

static void BlurProgramCallback(shader_asset_param *Param)
{
    auto *Program = (blur_program *)Param->ShaderProgram;
    if (IsLinkable(Program))
    {
        LinkShaderProgram(*Program);
        Program->PixelSize = glGetUniformLocation(Program->ID, "u_pixelSize");

        Bind(*Program);
        SetUniform(glGetUniformLocation(Program->ID, "u_sampler"), 0);
        UnbindShaderProgram();
    }
}

static void BlendProgramCallback(shader_asset_param *Param)
{
    auto *Program = Param->ShaderProgram;
    if (IsLinkable(Program))
    {
        LinkShaderProgram(*Program);
        Bind(*Program);
        for (int i = 0; i < BloomBlurPassCount; i++)
        {
            char Name[] = "u_Pass#";
            Name[6] = (char)('0' + i);
            SetUniform(glGetUniformLocation(Program->ID, Name), 0);
        }
        SetUniform(glGetUniformLocation(Program->ID, "u_Scene"), BloomBlurPassCount);
        UnbindShaderProgram();
    }
}

static void BlitProgramCallback(shader_asset_param *Param)
{
    auto *Program = Param->ShaderProgram;
    if (IsLinkable(Program))
    {
        LinkShaderProgram(*Program);
        Bind(*Program);
        SetUniform(glGetUniformLocation(Program->ID, "u_Sampler"), 0);
        UnbindShaderProgram();
    }
}

static void ResolveMultisampleProgramCallback(shader_asset_param *Param)
{
    auto *Program = (resolve_multisample_program *)Param->ShaderProgram;
    if (IsLinkable(Program))
    {
        LinkShaderProgram(*Program);
        Program->SampleCount = glGetUniformLocation(Program->ID, "u_SampleCount");

        Bind(*Program);
        SetUniform(glGetUniformLocation(Program->ID, "u_SurfaceReflectSampler"), 0);
        SetUniform(glGetUniformLocation(Program->ID, "u_EmitSampler"), 1);
        UnbindShaderProgram();
    }
}

static void PerlinNoiseProgramCallback(shader_asset_param *param)
{
	auto program = (perlin_noise_program *)param->ShaderProgram;
	if (IsLinkable(program))
	{
		LinkShaderProgram(*program);
		program->RandomSampler = glGetUniformLocation(program->ID, "u_RandomSampler");
		program->Time = glGetUniformLocation(program->ID, "u_Time");
		program->Offset = glGetUniformLocation(program->ID, "u_Offset");
		program->Color = glGetUniformLocation(program->ID, "u_Color");

		Bind(*program);
		SetUniform(program->RandomSampler, 0);
		UnbindShaderProgram();
	}
}

static void InitShaderProgram(shader_program &Program, const string &vsPath, const string &fsPath,
                              shader_asset_callback_func *Callback)
{
    Program.VertexShaderParam = {
        GL_VERTEX_SHADER,
        vsPath,
        &Program,
        Callback,
    };
    Program.FragmentShaderParam = {
        GL_FRAGMENT_SHADER,
        fsPath,
        &Program,
        Callback,
    };
}

model_program *CompileUberModelProgram(allocator *alloc, std::unordered_map<std::string, std::string> &defines)
{
    static std::unordered_map<std::string, model_program *> shaderCache;
    std::string key = "";
    for (auto it : defines)
    {
        key.append(it.first);
    }
    if (shaderCache.find(key) != shaderCache.end())
    {
        return shaderCache[key];
    }

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    auto result = PushStruct<model_program>(alloc);
	result->VertexShader = vertexShader;
	result->FragmentShader = fragmentShader;

    std::string vertexSource = "#version 330\n";
    std::string fragmentSource = "#version 330\n";
    for (auto it : defines)
    {
        std::string str = "#define " + it.first + " " + it.second + "\n";
        vertexSource.append(str);
        fragmentSource.append(str);
    }

    vertexSource.append(UberVertexShaderSource);
    fragmentSource.append(UberFragmentShaderSource);

    CompileShader(vertexShader, vertexSource.c_str());
    CompileShader(fragmentShader, fragmentSource.c_str());

    shader_asset_param param = {};
    param.ShaderProgram = result;
    ModelProgramCallback(&param);

    shaderCache[key] = result;
    return result;
}

static void InitRenderState(render_state &r, uint32 width, uint32 height, uint32 viewportWidth, uint32 viewportHeight)
{
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &r.MaxMultiSampleCount);
    if (r.MaxMultiSampleCount > 8)
    {
        r.MaxMultiSampleCount = 8;
    }
    r.Width = width;
    r.Height = height;
	r.ViewportWidth = viewportWidth;
	r.ViewportHeight = viewportHeight;
    r.FogColor = Color_black;

    InitShaderProgram(
        r.ModelProgram,
        "data/shaders/model.vert.glsl",
        "data/shaders/model.frag.glsl",
        ModelProgramCallback
    );
    InitShaderProgram(
        r.BlurHorizontalProgram,
        "data/shaders/blit.vert.glsl",
        "data/shaders/blurh.frag.glsl",
        BlurProgramCallback
    );
    InitShaderProgram(
        r.BlurVerticalProgram,
        "data/shaders/blit.vert.glsl",
        "data/shaders/blurv.frag.glsl",
        BlurProgramCallback
    );
    InitShaderProgram(
        r.BlendProgram,
        "data/shaders/blit.vert.glsl",
        "data/shaders/blend.frag.glsl",
        BlendProgramCallback
    );
    InitShaderProgram(
        r.BlitProgram,
        "data/shaders/blit.vert.glsl",
        "data/shaders/blit.frag.glsl",
        BlitProgramCallback
    );
    InitShaderProgram(
        r.ResolveMultisampleProgram,
        "data/shaders/blit.vert.glsl",
        "data/shaders/multisample.frag.glsl",
        ResolveMultisampleProgramCallback
    );
	InitShaderProgram(
		r.PerlinNoiseProgram,
		"data/shaders/blit.vert.glsl",
		"data/shaders/perlin.frag.glsl",
		PerlinNoiseProgramCallback
	);

    InitMeshBuffer(r.SpriteBuffer);
	InitBuffer(r.ScreenBuffer, 4, {
		vertex_attribute{0, 2, GL_FLOAT, 4 * sizeof(float), 0},
		vertex_attribute{1, 2, GL_FLOAT, 4 * sizeof(float), 2 * sizeof(float)}
	});

	float data[24] = {
		-1, -1, 0, 0,
		1, -1, 1, 0,
		-1, 1, 0, 1,
		1, -1, 1, 0,
		1, 1, 1, 1,
		-1, 1, 0, 1
	};
    PushVertex(r.ScreenBuffer, data);
    PushVertex(r.ScreenBuffer, data + 4);
    PushVertex(r.ScreenBuffer, data + 8);
    PushVertex(r.ScreenBuffer, data + 12);
    PushVertex(r.ScreenBuffer, data + 16);
    PushVertex(r.ScreenBuffer, data + 20);
    UploadVertices(r.ScreenBuffer, GL_STATIC_DRAW);

    InitFramebuffer(r, r.MultisampledSceneFramebuffer, width, height, FramebufferFlag_HasDepth | FramebufferFlag_Multisampled, ColorTextureFlag_Count);
    InitFramebuffer(r, r.SceneFramebuffer, width, height, FramebufferFlag_HasDepth, ColorTextureFlag_Count);
    for (int i = 0; i < BloomBlurPassCount; i++)
    {
        // @TODO: maybe it is not necessary to scale down
        size_t BlurWidth = width >> i;
        size_t BlurHeight = height >> i;

        InitFramebuffer(r, r.BlurHorizontalFramebuffers[i], BlurWidth, BlurHeight, FramebufferFlag_Filtered, 1);
        InitFramebuffer(r, r.BlurVerticalFramebuffers[i], BlurWidth, BlurHeight, FramebufferFlag_Filtered, 1);
    }

#ifdef DRAFT_DEBUG
    InitMeshBuffer(r.DebugBuffer);
#endif
}

inline void RenderClear(render_state &rs)
{
    color c = rs.FogColor;
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessBegin(render_state &rs)
{
    if (Global_Renderer_DoPostFX)
    {
        BindFramebuffer(rs.MultisampledSceneFramebuffer);
    }
    RenderClear(rs);
}

enum blur_orientation
{
    Blur_Horizontal,
    Blur_Vertical,
};
static framebuffer *RenderBlur(render_state &RenderState, blur_program *Program)
{
    Bind(*Program);
    framebuffer *Sources = RenderState.BlurHorizontalFramebuffers;
    framebuffer *Dests = RenderState.BlurVerticalFramebuffers;
    if (Program == &RenderState.BlurVerticalProgram)
    {
        auto Tmp = Sources;
        Sources = Dests;
        Dests = Tmp;
    }
    for (int i = 0; i < BloomBlurPassCount; i++)
    {
        if (Program == &RenderState.BlurVerticalProgram)
        {
            SetUniform(Program->PixelSize, Global_Renderer_BloomBlurOffset / float(Sources[i].Height));
        }
        else
        {
            SetUniform(Program->PixelSize, Global_Renderer_BloomBlurOffset / float(Sources[i].Width));
        }

        BindFramebuffer(Dests[i]);

        glClear(GL_COLOR_BUFFER_BIT);
        Bind(Sources[i].ColorTextures[0], 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        Unbind(Sources[i].ColorTextures[0], 0);

        UnbindFramebuffer(RenderState);
    }
    UnbindShaderProgram();
    return Dests;
}

void PostProcessEnd(render_state &r)
{
    if (Global_Renderer_DoPostFX)
    {
        UnbindFramebuffer(r);
        glBindVertexArray(r.ScreenBuffer.VAO);

        // resolve multisample
        if (Global_Renderer_BloomEnabled)
        {
            BindFramebuffer(r.SceneFramebuffer);
        }

        Bind(r.ResolveMultisampleProgram);
        SetUniform(r.ResolveMultisampleProgram.SampleCount, r.MaxMultiSampleCount);
        for (size_t i = 0; i < ColorTextureFlag_Count; i++)
        {
            Bind(r.MultisampledSceneFramebuffer.ColorTextures[i], i);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (Global_Renderer_BloomEnabled)
        {
            UnbindFramebuffer(r);

            // downsample scene for blur
            Bind(r.BlitProgram);
            Bind(r.SceneFramebuffer.ColorTextures[ColorTextureFlag_Emit], 0);
            for (int i = 0; i < BloomBlurPassCount; i++)
            {
                BindFramebuffer(r.BlurHorizontalFramebuffers[i]);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            UnbindFramebuffer(r);
            UnbindShaderProgram();

            // perform blur
            RenderBlur(r, &r.BlurHorizontalProgram);
            framebuffer *BlurOutput = RenderBlur(r, &r.BlurVerticalProgram);

            // blend scene with blur results
            Bind(r.BlendProgram);
            for (int i = 0; i < BloomBlurPassCount; i++)
            {
                Bind(BlurOutput[i].ColorTextures[0], i);
            }
            Bind(r.SceneFramebuffer.ColorTextures[ColorTextureFlag_SurfaceReflect], BloomBlurPassCount);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            UnbindShaderProgram();
        }
    }
    // @TODO: cleanup textures
}

void RenderBackground(render_state &rs, const background_state &bg)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Bind(rs.PerlinNoiseProgram);
	Bind(*bg.RandomTexture, 0);
	SetUniform(rs.PerlinNoiseProgram.Time, bg.Time);
	glBindVertexArray(rs.ScreenBuffer.VAO);

	for (int i = 0; i < bg.Instances.size(); i++)
	{
		SetUniform(rs.PerlinNoiseProgram.Color, bg.Instances[i].Color);
		SetUniform(rs.PerlinNoiseProgram.Offset, bg.Offset + bg.Instances[i].Offset);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	glDisable(GL_BLEND);
}

void RenderBegin(render_state &rs, float deltaTime)
{
    if (rs.ExplosionLightTimer > 0)
    {
        rs.ExplosionLightTimer -= deltaTime;
    }
    else
    {
        rs.ExplosionLightTimer = 0;
    }

	rs.DeltaTime = deltaTime;
    rs.LastVAO = -1;
    rs.RenderableCount = 0;
    rs.FrameSolidRenderables.clear();
    rs.FrameTransparentRenderables.clear();

#ifdef DRAFT_DEBUG
    ResetBuffer(rs.DebugBuffer);
#endif
}

mat4 GetTransformMatrix(transform &t)
{
    mat4 TransformMatrix = glm::translate(mat4(1.0f), t.Position);
    if (t.Rotation.x != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(t.Rotation.x), vec3(1,0,0));
    }
    if (t.Rotation.y != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(t.Rotation.y), vec3(0,1,0));
    }
    if (t.Rotation.z != 0.0f)
    {
        TransformMatrix = glm::rotate(TransformMatrix, glm::radians(t.Rotation.z), vec3(0,0,1));
    }
    TransformMatrix = glm::scale(TransformMatrix, t.Scale);
    return TransformMatrix;
}

static void RenderRenderable(render_state &rs, camera &Camera, renderable &r)
{
    if (rs.LastVAO != r.VAO)
    {
        glBindVertexArray(rs.LastVAO = r.VAO);
    }

    if (r.PrimitiveType == GL_LINES || r.PrimitiveType == GL_LINE_LOOP)
    {
        glLineWidth(r.LineWidth);
    }

    static model_program *previousProgram = NULL;
    auto program = &rs.ModelProgram;
    if (r.Program)
    {
        program = r.Program;
    }

	Bind(*program);
    if (program != previousProgram)
    {
        previousProgram = program;
    }

    SetUniform(program->BendRadius, rs.BendRadius);
    SetUniform(program->RoadTangentPoint, *rs.RoadTangentPoint);
    SetUniform(program->FogStart, Global_Renderer_FogStart);
    SetUniform(program->FogEnd, Global_Renderer_FogEnd);
    SetUniform(program->FogColor, rs.FogColor);
    SetUniform(program->ProjectionView, Camera.ProjectionView);
    SetUniform(program->CamPos, Camera.Position);
    SetUniform(program->Transform, GetTransformMatrix(r.Transform));

    // @TODO: check why this breaks the floor model
    SetUniform(program->NormalTransform, mat4(1.0f));

    if (r.Material->Texture)
    {
        Bind(*r.Material->Texture, 0);
    }
    if (r.Material->Flags & MaterialFlag_PolygonLines)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    SetUniform(program->MaterialFlags, (int)r.Material->Flags);
    SetUniform(program->DiffuseColor, r.Material->DiffuseColor);
	SetUniform(program->SpecularColor, r.Material->SpecularColor);
	SetUniform(program->EmissiveColor, r.Material->EmissiveColor);
    SetUniform(program->TexWeight, r.Material->TexWeight);
    SetUniform(program->FogWeight, r.Material->FogWeight);
    SetUniform(program->Emission, r.Material->Emission);
	SetUniform(program->Shininess, r.Material->Shininess);
    SetUniform(program->UvScale, r.Material->UvScale);
    SetUniform(program->ExplosionLightColor, rs.ExplosionLightColor);
    SetUniform(program->ExplosionLightTimer, rs.ExplosionLightTimer);

	if (r.Material->Flags & MaterialFlag_CullFace)
	{
		glEnable(GL_CULL_FACE);
	}

	if (r.IsIndexed)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.IBO);
		glDrawElements(r.PrimitiveType, r.Count, GL_UNSIGNED_INT, (void *)(r.Offset*sizeof(uint32)));
	}
	else
	{
		glDrawArrays(r.PrimitiveType, r.Offset, r.Count);
	}

	if (r.Material->Flags & MaterialFlag_CullFace)
	{
		glDisable(GL_CULL_FACE);
	}

    if (r.Material->Flags & MaterialFlag_PolygonLines)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (r.Material->Texture)
    {
        Unbind(*r.Material->Texture, 0);
    }
}

static material DebugMaterial{Color_white, 0.0f, 0.0f, NULL, MaterialFlag_PolygonLines};

struct renderable_sorter
{
    render_state *rs;

    bool operator()(size_t a, size_t b)
    {
        return rs->Renderables[a].SortNumber < rs->Renderables[b].SortNumber;
    }
};

void RenderEnd(render_state &rs, camera &Camera)
{
    assert(Camera.Updated);

#ifdef DRAFT_DEBUG
    // push debug renderable
    {
        size_t i = NextRenderable(rs);
        auto &r = rs.Renderables[i];
        r.VAO = rs.DebugBuffer.VAO;
        r.Offset = 0;
        r.Count = rs.DebugBuffer.VertexCount;
        r.Material = &DebugMaterial;
        r.PrimitiveType = GL_LINES;
        r.Transform = transform{};
        rs.FrameSolidRenderables.push_back(i);
        UploadVertices(rs.DebugBuffer, GL_DYNAMIC_DRAW);
    }
#endif

    renderable_sorter sorter;
    sorter.rs = &rs;
    std::sort(rs.FrameSolidRenderables.begin(), rs.FrameSolidRenderables.end(), sorter);
    //std::sort(rs.FrameTransparentRenderables.begin(), rs.FrameTransparentRenderables.end(), sorter);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    for (auto renderableIndex : rs.FrameSolidRenderables)
    {
        RenderRenderable(rs, Camera, rs.Renderables[renderableIndex]);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto renderableIndex : rs.FrameTransparentRenderables)
    {
        RenderRenderable(rs, Camera, rs.Renderables[renderableIndex]);
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void AddRenderable(render_state &rs, size_t index, material *material)
{
    if (material->DiffuseColor.a < 1.0f || (material->Flags & MaterialFlag_ForceTransparent))
    {
        rs.FrameTransparentRenderables.push_back(index);
    }
    else
    {
        rs.FrameSolidRenderables.push_back(index);
    }
}

void ApplyExplosionLight(render_state &rs, color c)
{
    rs.ExplosionLightColor = c;
    rs.ExplosionLightTimer = Global_Game_ExplosionLightTime*0.5f;
}

void DrawMeshPart(render_state &rs, mesh &Mesh, mesh_part &part, const transform &Transform)
{
    size_t Index = NextRenderable(rs);
    auto &r = rs.Renderables[Index];
    r.SortNumber = rs.RenderableCount * 100;
    r.Program = part.Program;
    r.LineWidth = part.LineWidth;
    r.PrimitiveType = part.PrimitiveType;
    r.Offset = part.Offset;
    r.Count = part.Count;
    r.VAO = Mesh.Buffer.VAO;
	r.IBO = Mesh.Buffer.IBO;
	r.IsIndexed = Mesh.Buffer.IsIndexed;
    r.Material = part.Material;
    r.Transform = Transform;
    r.Bounds = BoundsFromMinMax(Mesh.Min*Transform.Scale, Mesh.Max*Transform.Scale);
    r.Bounds.Center += Transform.Position;
    AddRenderable(rs, Index, part.Material);
}

void DrawModel(render_state &rs, model &Model, const transform &Transform)
{
    mesh *Mesh = Model.Mesh;
    for (auto &Part : Mesh->Parts)
    {
        size_t i = &Part - &Mesh->Parts[0];
        auto Material = Part.Material;
        if (i < Model.Materials.size() && Model.Materials[i])
        {
            Material = Model.Materials[i];
        }

        size_t Index = NextRenderable(rs);
        auto &r = rs.Renderables[Index];
        if (Model.SortNumber > -1)
        {
            r.SortNumber = Model.SortNumber + i;
        }
        else
        {
            r.SortNumber = rs.RenderableCount * 100;
        }

        r.Program = Part.Program;
        r.LineWidth = Part.LineWidth;
        r.PrimitiveType = Part.PrimitiveType;
        r.Offset = Part.Offset;
        r.Count = Part.Count;
        r.VAO = Mesh->Buffer.VAO;
		r.IBO = Mesh->Buffer.IBO;
		r.IsIndexed = Mesh->Buffer.IsIndexed;
        r.Material = Material;
        r.Transform = Transform;
        r.Bounds = BoundsFromMinMax(Mesh->Min*Transform.Scale, Mesh->Max*Transform.Scale);
        r.Bounds.Center += Transform.Position;
        AddRenderable(rs, Index, Material);
    }
}

#ifdef DRAFT_DEBUG
inline static void DrawDebugCollider(render_state &rs, bounding_box &box, bool isColliding)
{
    color c = Color_green;
    if (isColliding)
    {
        c = Color_red;
    }
    AddCubeWithRotation(rs.DebugBuffer, c, true, box.Center, box.Half*2.0f, 0.0f);
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
