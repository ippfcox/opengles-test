#include <stdlib.h>
#include "util.h"
#include "log.h"

static int check_error(GLuint x)
{
    void (*glGetiv)(GLuint x, GLenum pname, GLint *params);
    void (*glGetInfoLog)(GLuint x, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    void (*glDelete)(GLuint x);
    GLenum pname_status;

    if (glIsShader(x))
    {
        glGetiv = glGetShaderiv;
        glGetInfoLog = glGetShaderInfoLog;
        glDelete = glDeleteShader;
        pname_status = GL_COMPILE_STATUS;
    }
    else if (glIsProgram(x))
    {
        glGetiv = glGetProgramiv;
        glGetInfoLog = glGetProgramInfoLog;
        glDelete = glDeleteProgram;
        pname_status = GL_LINK_STATUS;
    }
    else
    {
        logerror("invalid x");
        return -1;
    }

    GLint success;
    glGetiv(x, pname_status, &success);
    if (!success)
    {
        GLint info_len = 0;
        glGetiv(x, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
        {
            char *info_log = calloc(1, info_len);
            glGetInfoLog(x, info_len, NULL, info_log);
            logerror("failed: %s", info_log);
            free(info_log);
        }
        glDelete(x);
        return -1;
    }

    return 0;
}

GLuint load_shader(GLenum type, const char *shader_src)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        logerror("glCreateShader failed");
        return 0;
    }

    // load the shader source
    glShaderSource(shader, 1, &shader_src, NULL);

    // compile the shader
    glCompileShader(shader);

    if (check_error(shader) != 0)
    {
        logerror("error accured");
        return 0;
    }

    return shader;
}

GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    if (program == 0)
    {
        logerror("glCreateProgram failed");
        return 0;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    if (check_error(program) != 0)
    {
        logerror("error occured");
        return 0;
    }

    return program;
}

void load_texture(GLenum texture_id, GLuint texture, GLint format, GLsizei width, GLsizei height, void *buffer)
{
    glActiveTexture(texture_id);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
    // glGenerateMipmap(texture);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

void update_texture(GLenum texture_id, GLuint texture, GLint format, GLsizei width, GLsizei height, void *buffer)
{
    glActiveTexture(texture_id);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);
}