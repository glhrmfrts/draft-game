#include "luna_render.h"

// forward declarations of shaders
extern string ModelVertexShader;
extern string ModelFragmentShader;

static void EnableVertexAttribute(vertex_attribute Attr)
{
  glEnableVertexAttribArray(Attr.Location);
  glVertexAttribPointer(Attr.Location, Attr.Size, Attr.Type, false, Attr.Stride, (void *)Attr.Offset);
}

static void vInitBuffer(vertex_buffer &Buffer, size_t AttrCount, va_list Args)
{
    glGenVertexArrays(1, &Buffer.VAO);
    glGenBuffers(1, &Buffer.VBO);

    glBindVertexArray(Buffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.VBO);
    for (size_t i = 0; i < AttrCount; i++) {
        vertex_attribute Attr = va_arg(Args, vertex_attribute);
        EnableVertexAttribute(Attr);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#define InitialVertexBufferSize 16
static void EnsureCapacity(vertex_buffer &Buffer, size_t Size)
{
    if (Buffer.RawIndex + Size > Buffer.Vertices.size()) {
        if (Buffer.Vertices.size() == 0) {
            Buffer.Vertices.resize(InitialVertexBufferSize * Buffer.VertexSize);
        }
        else {
            Buffer.Vertices.resize(Buffer.Vertices.size() * 2);
        }
    }
}

// InitBuffer accepts a variadic number of vertex_attributes, it
// initializes the buffer and enable the passed attributes
void InitBuffer(vertex_buffer &Buffer, size_t VertexSize, size_t AttrCount, ...)
{
    Buffer.VertexSize = VertexSize;
    ResetBuffer(Buffer);

    va_list Args;
    va_start(Args, AttrCount);
    vInitBuffer(Buffer, AttrCount, Args);
    va_end(Args);
}

void ResetBuffer(vertex_buffer &Buffer)
{
    Buffer.VertexCount = 0;
    Buffer.RawIndex = 0;
}

// PushVertex pushes a single vertex to the buffer, the vertex size
// is known by the VertexSize field
void PushVertex(vertex_buffer &Buffer, const vector<float> &Verts)
{
    EnsureCapacity(Buffer, Buffer.VertexSize);

    size_t Idx = Buffer.RawIndex;
    for (size_t i = 0; i < Buffer.VertexSize; i++) {
        Buffer.Vertices[Idx+i] = Verts[i];
    }
    Buffer.RawIndex += Buffer.VertexSize;
    Buffer.VertexCount++;
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
    if (Status == GL_FALSE) {
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
    if (Status == GL_FALSE) {
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

void UploadTexture(texture &Texture, GLenum SrcFormat, GLenum DstFormat, GLenum Type, uint8 *Data)
{
    glTexImage2D(Texture.Target, 0, SrcFormat, Texture.Width, Texture.Height, 0, DstFormat, Type, Data);
}

vector<texture_rect> SplitTexture(texture &Texture, int Width, int Height, bool FlipV)
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

void SetAnimation(animated_sprite &Sprite, const sprite_animation &Animation, bool Reset)
{
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
    Camera.Up = vec3(0, 1, 0);

    switch (Camera.Type) {
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
        Camera.ProjectionView = Camera.Projection * Camera.View;
        break;
    }
    }
}

// InitMeshBuffer initializes a vertex_buffer with the common attributes
// used by all meshes in the game
void InitMeshBuffer(vertex_buffer &Buffer)
{
    size_t Stride = 12*sizeof(float);
    InitBuffer(Buffer, 12, 4,
               (vertex_attribute){0, 3, GL_FLOAT, Stride, 0}, // position
               (vertex_attribute){1, 2, GL_FLOAT, Stride, 3*sizeof(float)}, // uv
               (vertex_attribute){2, 4, GL_FLOAT, Stride, 5*sizeof(float)}, // color
               (vertex_attribute){3, 3, GL_FLOAT, Stride, 9*sizeof(float)}); // normal
}

static void CompileModelProgram(model_program &Program)
{
    CompileShaderProgram(Program.ShaderProgram, ModelVertexShader.c_str(), ModelFragmentShader.c_str());
    Program.ProjectionView = glGetUniformLocation(Program.ShaderProgram.ID, "u_ProjectionView");
    Program.Transform = glGetUniformLocation(Program.ShaderProgram.ID, "u_Transform");
    Program.NormalTransform = glGetUniformLocation(Program.ShaderProgram.ID, "u_NormalTransform");
    Program.DiffuseColor = glGetUniformLocation(Program.ShaderProgram.ID, "u_DiffuseColor");
    Program.TexWeight = glGetUniformLocation(Program.ShaderProgram.ID, "u_TexWeight");
    Program.Sampler = glGetUniformLocation(Program.ShaderProgram.ID, "u_Sampler");

    Bind(Program.ShaderProgram);
    SetUniform(Program.Sampler, 0);
    UnbindShaderProgram();
}

static void BindCamera(model_program &Program, camera &Camera)
{
    SetUniform(Program.ProjectionView, Camera.ProjectionView);
}

void InitRenderState(render_state &RenderState)
{
    CompileModelProgram(RenderState.ModelProgram);
    InitMeshBuffer(RenderState.SpriteBuffer);

#ifdef LUNA_DEBUG
    InitMeshBuffer(RenderState.DebugBuffer);
    glLineWidth(2);
#endif
}

void RenderMesh(render_state &RenderState, camera &Camera, mesh &Mesh, const mat4 &TransformMatrix)
{
    assert(RenderState.ModelProgram.ShaderProgram.ID);
    assert(Camera.Updated);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBindVertexArray(Mesh.Buffer.VAO);

    auto &Program = RenderState.ModelProgram;
    Bind(Program.ShaderProgram);
    BindCamera(Program, Camera);

    SetUniform(Program.Transform, TransformMatrix);
    SetUniform(Program.NormalTransform, mat4(1.0f));
    for (const auto &Part : Mesh.Parts) {
        auto &Material = Part.Material;
        if (Material.Texture) {
            //Println(Material.Texture->Filename);
            Bind(*Material.Texture, 0);
        }

        SetUniform(Program.DiffuseColor, Material.DiffuseColor);
        SetUniform(Program.TexWeight, Material.TexWeight);
        glDrawArrays(Part.PrimitiveType, Part.Offset, Part.Count);

        if (Material.Texture) {
            Unbind(*Material.Texture, 0);
        }
    }

    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
}

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
    BindCamera(Program, Camera);
    SetUniform(Program.Transform, mat4(1.0));
    SetUniform(Program.TexWeight, 1.0f);
    SetUniform(Program.DiffuseColor, Color_white);

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

#ifdef LUNA_DEBUG
void DebugRenderAABB(render_state &RenderState, camera &Camera, shape_aabb *Shape, bool Colliding)
{
    assert(Camera.Updated);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBindVertexArray(RenderState.DebugBuffer.VAO);

    auto &Program = RenderState.ModelProgram;
    Bind(Program.ShaderProgram);
    BindCamera(Program, Camera);
    SetUniform(Program.Transform, mat4(1.0));
    SetUniform(Program.TexWeight, 0.0f);

    color DiffuseColor = Color_green;
    if (Colliding)
    {
        DiffuseColor = Color_red;
    }
    SetUniform(Program.DiffuseColor, DiffuseColor);

    vec3 a = Shape->Position - Shape->Half;
    vec3 b = Shape->Position + Shape->Half;
    ResetBuffer(RenderState.DebugBuffer);
    PushVertex(RenderState.DebugBuffer, {a.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});

    PushVertex(RenderState.DebugBuffer, {a.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});

    PushVertex(RenderState.DebugBuffer, {a.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});

    PushVertex(RenderState.DebugBuffer, {b.x, a.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, a.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});

    PushVertex(RenderState.DebugBuffer, {b.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {b.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});

    PushVertex(RenderState.DebugBuffer, {a.x, a.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    PushVertex(RenderState.DebugBuffer, {a.x, b.y, b.z, 0, 0, 1, 1, 1, 1, 0, 0, 0});
    UploadVertices(RenderState.DebugBuffer, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, RenderState.DebugBuffer.VertexCount);

    UnbindShaderProgram();
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
}
#endif

string ModelVertexShader = R"FOO(
#version 330

// In this game we use only one forward directional light
// so these simple defines will do the job
#define AmbientLight vec4(0.5f)
#define LightColor vec4(1.0f)
#define LightIntensity 1

    layout (location = 0) in vec3  a_Position;
    layout (location = 1) in vec2  a_Uv;
    layout (location = 2) in vec4  a_Color;
    layout (location = 3) in vec3  a_Normal;

    uniform mat4 u_ProjectionView;
    uniform mat4 u_Transform;
    uniform mat4 u_NormalTransform;

    smooth out vec2  v_Uv;
    smooth out vec4  v_Color;

    void main() {
      vec4 WorldPos = u_Transform * vec4(a_Position, 1.0);
      gl_Position = u_ProjectionView * WorldPos;

      vec3 Normal = (u_NormalTransform * vec4(a_Normal, 1.0)).xyz;

      vec4 Lighting = AmbientLight;
      Lighting += LightColor * dot(-vec3(0, -1, 0), Normal) * LightIntensity;
      Lighting.a = 1.0f;

      v_Uv = a_Uv;
      v_Color = a_Color * clamp(Lighting, 0, 1);
    }
)FOO";

string ModelFragmentShader = R"FOO(
#version 330

    uniform sampler2D u_Sampler;
    uniform vec4 u_DiffuseColor;
    uniform float u_Emission;
    uniform float u_TexWeight;

    smooth in vec2  v_Uv;
    smooth in vec4  v_Color;

    out vec4 BlendUnitColor;

    void main() {
      vec4 TexColor = texture(u_Sampler, v_Uv);
      vec4 Color = mix(vec4(1.0f), TexColor, u_TexWeight);
      Color *= v_Color;
      Color *= u_DiffuseColor;

      //float fog = abs(v_worldPos.z - u_camPos.z);
      //fog = (u_fogEnd - fog) / (u_fogEnd - u_fogStart);
      //fog = clamp(fog, 0.0, 1.0);

      //float emitSpread = 1.0f;
      //vec3 emit = (color * u_emission).rgb;

      BlendUnitColor = Color;//mix(color, u_fogColor, 1.0 - fog);
    }
)FOO";
