#include <string.h>
#include <stdlib.h>
#include "egl.h"
#include "log.h"

static EGLint get_context_render_type(EGLDisplay egl_display)
{
    const char *extensions = eglQueryString(egl_display, EGL_EXTENSIONS);
    if (extensions != NULL && strstr(extensions, "EGL_KHR_create_context"))
        return EGL_OPENGL_ES3_BIT_KHR;
    return EGL_OPENGL_ES2_BIT;
}

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

static GLuint load_shader(GLenum type, const char *shader_src)
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

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
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

int egl_window_create(struct egl_context *ctx, EGLNativeDisplayType egl_native_display, EGLNativeWindowType egl_native_window)
{
    if (!ctx)
    {
        logerror("invalid egl_context");
        return -1;
    }

    ctx->display = eglGetDisplay(egl_native_display);
    if (ctx->display == EGL_NO_DISPLAY)
    {
        logerror("eglGetDisplay failed");
        return -1;
    }

    // initialize EGL
    EGLint major_version;
    EGLint minor_version;
    if (!eglInitialize(ctx->display, &major_version, &minor_version))
    {
        logerror("eglInitialize failed");
        return -1;
    }

    EGLint num_configs = 0;
    EGLint attrib_list[] = {
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_BLUE_SIZE, 5,
        EGL_ALPHA_SIZE, EGL_DONT_CARE,
        EGL_DEPTH_SIZE, EGL_DONT_CARE,
        EGL_STENCIL_SIZE, EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS, 0,
        EGL_RENDERABLE_TYPE, get_context_render_type(ctx->display),
        EGL_NONE};

    // choose config
    EGLConfig config;
    if (!eglChooseConfig(ctx->display, attrib_list, &config, 1, &num_configs) || num_configs < 1)
    {
        logerror("eglChooseConfig failed");
        return -1;
    }

    // create a surface
    ctx->surface = eglCreateWindowSurface(ctx->display, config, egl_native_window, NULL);
    if (ctx->surface == EGL_NO_SURFACE)
    {
        logerror("eglCreateWindowSurface failed");
        return -1;
    }

    // create a GL context
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext egl_context = eglCreateContext(ctx->display, config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT)
    {
        logerror("eglCreateContext failed");
        return -1;
    }

    // make context current
    if (!eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, egl_context))
    {
        logerror("eglMakeCurrent failed");
        return -1;
    }

    return 0;
}

int egl_load_shader(struct egl_context *ctx, const char *vertex_shader_src, const char *fragment_shader_src)
{
    GLuint vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader == 0)
    {
        logerror("load_shader VERTEX failed");
        return -1;
    }

    GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    if (fragment_shader == 0)
    {
        logerror("load_shader FRAGMENT failed");
        return -1;
    }

    ctx->program = link_program(vertex_shader, fragment_shader);
    if (ctx->program == 0)
    {
        logerror("link_program failed");
        return -1;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    return 0;
}

void egl_draw(struct egl_context *ctx)
{
    // draw a triangle
    GLfloat vertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f};

    // set viewport
    glViewport(0, 0, 100, 100);

    // clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // use program object
    glUseProgram(ctx->program);

    // load vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(ctx->display, ctx->surface);
}