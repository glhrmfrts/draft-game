#include "luna_render.h"

static void EnableVertexAttribute(vertex_attribute Attr)
{
  glEnableVertexAttribArray(Attr.Location);
  glVertexAttribPointer(Attr.Location, Attr.Size, Attr.Type, false, Attr.Stride, (void *)Attr.Offset);
}

static void vInitBuffer(vertex_buffer *Buffer, size_t AttrCount, va_list Args)
{
    glGenVertexArrays(1, &Buffer->VAO);
    glGenBuffers(1, &Buffer->VBO);

    glBindVertexArray(Buffer->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer->VBO);
    for (size_t i = 0; i < AttrCount; i++) {
        vertex_attribute Attr = va_arg(Args, vertex_attribute);
        EnableVertexAttribute(Attr);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#define InitialVertexBufferSize 16
static void EnsureCapacity(vertex_buffer *Buffer, size_t Size)
{
    if (Buffer->RawIndex + Size > Buffer->Vertices.size()) {
        if (Buffer->Vertices.size() == 0) {
            Buffer->Vertices.resize(InitialVertexBufferSize * Buffer->VertexSize);
        }
        else {
            Buffer->Vertices.resize(Buffer->Vertices.size() * 2);
        }
    }
}

void InitBuffer(vertex_buffer *Buffer, size_t VertexSize, size_t AttrCount, ...)
{
    Buffer->VertexSize = VertexSize;
    ResetBuffer(Buffer);

    va_list Args;
    va_start(Args, AttrCount);
    vInitBuffer(Buffer, AttrCount, Args);
    va_end(Args);
}

void ResetBuffer(vertex_buffer *Buffer)
{
    Buffer->VertexCount = 0;
    Buffer->RawIndex = 0;
}

void PushVertex(vertex_buffer *Buffer, ...)
{
    EnsureCapacity(Buffer, Buffer->VertexSize);

    va_list Args;
    va_start(Args, Buffer);

    size_t Idx = Buffer->RawIndex;
    for (size_t i = 0; i < Buffer->VertexSize; i++) {
        Buffer->Vertices[Idx+i] = (float)va_arg(Args, double);
    }
    Buffer->RawIndex += Buffer->VertexSize;
    Buffer->VertexCount++;

    va_end(Args);
}

void UploadVertices(vertex_buffer *Buffer, GLenum Usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, Buffer->VBO);
    glBufferData(GL_ARRAY_BUFFER, Buffer->VertexCount * Buffer->VertexSize * sizeof(float), &Buffer->Vertices[0], Usage);
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

void CompileShaderProgram(shader_program *Prog, const char *VertexSource, const char *FragmentSource)
{
    GLuint VertexShader = CompileShader(VertexSource, GL_VERTEX_SHADER);
    GLuint FragmentShader = CompileShader(FragmentSource, GL_FRAGMENT_SHADER);

    Prog->ID = glCreateProgram();
    glAttachShader(Prog->ID, VertexShader);
    glAttachShader(Prog->ID, FragmentShader);
    glLinkProgram(Prog->ID);

    int Status;
    glGetProgramiv(Prog->ID, GL_LINK_STATUS, &Status);
    if (Status == GL_FALSE) {
      int LogLength = 0;
      glGetProgramiv(Prog->ID, GL_INFO_LOG_LENGTH, &LogLength);

      char *InfoLog = new char[LogLength + 1];
      glGetProgramInfoLog(Prog->ID, LogLength, NULL, InfoLog);
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

void Bind(shader_program *Prog)
{
    glUseProgram(Prog->ID);
}

void UnbindShaderProgram()
{
    glUseProgram(0);
}

void Bind(texture *Texture, int TextureUnit)
{
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindTexture(Texture->Target, Texture->ID);
}

void Unbind(texture *Texture, int TextureUnit)
{
    glActiveTexture(GL_TEXTURE0 + TextureUnit);
    glBindTexture(Texture->Target, 0);
}

void ApplyTextureParameters(texture *Texture, int TextureUnit)
{
    if (!Texture->ID) {
        glGenTextures(1, &Texture->ID);
    }

    Bind(Texture, TextureUnit);
    glTexParameteri(Texture->Target, GL_TEXTURE_MIN_FILTER, Texture->Filters.Min);
    glTexParameteri(Texture->Target, GL_TEXTURE_MAG_FILTER, Texture->Filters.Mag);
    glTexParameteri(Texture->Target, GL_TEXTURE_WRAP_S, Texture->Wrap.WrapS);
    glTexParameteri(Texture->Target, GL_TEXTURE_WRAP_T, Texture->Wrap.WrapT);

    if (Texture->Mipmap) {
        glGenerateMipmap(Texture->Target);
    }

    Unbind(Texture, TextureUnit);
}

void UploadTexture(texture *Texture, GLenum SrcFormat, GLenum DstFormat, GLenum Type, uint8 *Data)
{
    glTexImage2D(Texture->Target, 0, SrcFormat, Texture->Width, Texture->Height, 0, DstFormat, Type, Data);
}

#include <iostream>

void MakeCameraOrthographic(camera *Camera, float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    Camera->Type = Camera_orthographic;
    Camera->Near = Near;
    Camera->Far = Far;
    Camera->Ortho = {Left, Right, Bottom, Top};
}

void MakeCameraPerspective(camera *Camera, float Width, float Height, float Fov, float Near, float Far)
{
    Camera->Type = Camera_perspective;
    Camera->Near = Near;
    Camera->Far = Far;
    Camera->Perspective = {Width, Height, Fov};
}

void UpdateProjectionView(camera *Camera)
{
    Camera->Up = vec3(0, 1, 0);
    switch (Camera->Type) {
    case Camera_orthographic: {
        Camera->ProjectionView = glm::ortho(Camera->Ortho.Left,
                                            Camera->Ortho.Right,
                                            Camera->Ortho.Bottom,
                                            Camera->Ortho.Top,
                                            Camera->Near,
                                            Camera->Far);
        break;
    }

    case Camera_perspective: {
        auto Persp = &Camera->Perspective;
        Camera->Projection = glm::perspective(Persp->Fov, (float)Persp->Width / (float)Persp->Height, Camera->Near, Camera->Far);
        Camera->View = glm::lookAt(Camera->Position, Camera->LookAt, Camera->Up);
        Camera->ProjectionView = Camera->Projection * Camera->View;
        break;
    }
    }
}
