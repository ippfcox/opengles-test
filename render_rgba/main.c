#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
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

int main()
{
    set_log_level(LOG_LEVEL_DEBUG);

    ////////////////////////////////////////////////////////////////////////////
    //                           X11 initialize                               //
    ////////////////////////////////////////////////////////////////////////////

    Display *x11_display = XOpenDisplay(NULL);
    if (!x11_display)
        logfatal("XOpenDisplay failed");

    Window root = XDefaultRootWindow(x11_display);
    XSetWindowAttributes swa = {.event_mask = ExposureMask | PointerMotionMask | KeyPressMask};
    Window x11_window = XCreateWindow(
        x11_display, root,
        0, 0, 960, 540, 0,
        CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask,
        &swa);

    Atom s_wm_delete_message = XInternAtom(x11_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x11_display, x11_window, &s_wm_delete_message, 1);

    XSetWindowAttributes xattr = {.override_redirect = 0};
    XChangeWindowAttributes(x11_display, x11_window, CWOverrideRedirect, &xattr);

    XWMHints hints = {.input = 1, .flags = InputHint};
    XSetWMHints(x11_display, x11_window, &hints);

    // show window
    XMapWindow(x11_display, x11_window);
    XStoreName(x11_display, x11_window, "TITLE");

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(x11_display, "_NET_WM_STATE", 0);

    XEvent xev = {.type = ClientMessage};
    xev.xclient.window = x11_window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[0] = 0;
    XSendEvent(x11_display, DefaultRootWindow(x11_display), 0, SubstructureNotifyMask, &xev);

    ////////////////////////////////////////////////////////////////////////////
    //                          EGL initialize                                //
    ////////////////////////////////////////////////////////////////////////////

    EGLNativeDisplayType egl_native_display = x11_display;
    EGLNativeWindowType egl_native_window = x11_window;

    EGLDisplay egl_display = eglGetDisplay(egl_native_display);
    if (egl_display == EGL_NO_DISPLAY)
    {
        logerror("eglGetDisplay failed");
        return -1;
    }

    // initialize EGL
    EGLint major_version;
    EGLint minor_version;
    if (!eglInitialize(egl_display, &major_version, &minor_version))
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
        EGL_RENDERABLE_TYPE, get_context_render_type(egl_display),
        EGL_NONE};

    // choose config
    EGLConfig config;
    if (!eglChooseConfig(egl_display, attrib_list, &config, 1, &num_configs) || num_configs < 1)
    {
        logerror("eglChooseConfig failed");
        return -1;
    }

    // create a surface
    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, config, egl_native_window, NULL);
    if (egl_surface == EGL_NO_SURFACE)
    {
        logerror("eglCreateWindowSurface failed");
        return -1;
    }

    // create a GL context
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT)
    {
        logerror("eglCreateContext failed");
        return -1;
    }

    // make context current
    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context))
    {
        logerror("eglMakeCurrent failed");
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////
    //                              shader                                    //
    ////////////////////////////////////////////////////////////////////////////

    char vertex_shader_src[] =
        "#version 300 es                          \n"
        "layout(location = 0) in vec4 vPosition;  \n"
        "void main()                              \n"
        "{                                        \n"
        "   gl_Position = vPosition;              \n"
        "}                                        \n";
    char fragment_shader_src[] =
        "#version 300 es                              \n"
        "precision mediump float;                     \n"
        "out vec4 fragColor;                          \n"
        "void main()                                  \n"
        "{                                            \n"
        "   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
        "}                                            \n";

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

    GLuint program = link_program(vertex_shader, fragment_shader);
    if (program == 0)
    {
        logerror("link_program failed");
        return -1;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    ////////////////////////////////////////////////////////////////////////////
    //                           X11 loop                                     //
    ////////////////////////////////////////////////////////////////////////////

    XEvent xev2;
    int stop = 0;
    size_t time_ms_ckpt = 0;
    size_t seq = 0, seq_ckpt = 0;

    while (!stop)
    {
        // event handle
        while (XPending(x11_display))
        {
            XNextEvent(x11_display, &xev2);
            switch (xev2.type)
            {
            case KeyPress:
            {
                KeySym key;
                char key_char;
                if (XLookupString(&xev2.xkey, &key_char, 1, &key, 0))
                {
                    loginfo("keypress: %c", key_char);
                }
            }
            break;
            case ClientMessage:
            {
                if (xev2.xclient.data.l[0] == s_wm_delete_message)
                {
                    stop = 1;
                }
            }
            break;
            case DestroyNotify:
            {
                stop = 1;
            }
            break;
            }
        }

        // fps calc
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
        size_t time_ms_curr = tp.tv_sec * 1e3 + tp.tv_nsec / 1e6;
        if (time_ms_curr - time_ms_ckpt > 1000)
        {
            loginfo("fps: %d", seq - seq_ckpt);
            seq_ckpt = seq;
            time_ms_ckpt = time_ms_curr;
        }

        ++seq;

        // draw a triangle
        GLfloat vertices[] = {
            0.0f, 0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f};

        // set viewport
        glViewport(0, 0, 960, 540);

        // clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // use program object
        glUseProgram(program);

        // load vertex data
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        eglSwapBuffers(egl_display, egl_surface);
    }
}