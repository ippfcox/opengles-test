#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "log.h"

static Display *x_display = NULL;
static Atom s_wm_delete_message;

EGLint GetContextRenderType(EGLDisplay egl_display)
{
    const char *extensions = eglQueryString(egl_display, EGL_EXTENSIONS);
    if (extensions != NULL && strstr(extensions, "EGL_KHR_create_context"))
        return EGL_OPENGL_ES3_BIT_KHR;
    return EGL_OPENGL_ES2_BIT;
}

GLuint LoaderShader(GLenum type, const char *shader_src)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        return 0;
    }

    // load the shader source
    glShaderSource(shader, 1, &shader_src, NULL);

    // compile the shader
    glCompileShader(shader);

    // check the compile status
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint info_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
        {
            char *info_log = calloc(1, info_len);
            glGetShaderInfoLog(shader, info_len, NULL, info_log);
            logerror("compile shader failed: %s", info_log);
            free(info_log);
        }

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

int main()
{
    //////////////////////////////////
    // initialize native X11 display and window for EGL
    //////////////////////////////////

    // X11 native display initialization
    x_display = XOpenDisplay(NULL);
    if (x_display == NULL)
    {
        logfatal("XOpenDisplay failed");
    }

    Window root = DefaultRootWindow(x_display);
    XSetWindowAttributes swa = {.event_mask = ExposureMask | PointerMotionMask | KeyPressMask};
    Window win = XCreateWindow(
        x_display, root,
        0, 0, 100, 100, 0,
        CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask,
        &swa);
    s_wm_delete_message = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x_display, win, &s_wm_delete_message, 1);

    XSetWindowAttributes xattr = {.override_redirect = 0};
    XChangeWindowAttributes(x_display, win, CWOverrideRedirect, &xattr);

    XWMHints hints = {.input = 1, .flags = InputHint};
    XSetWMHints(x_display, win, &hints);

    // show window
    XMapWindow(x_display, win);
    XStoreName(x_display, win, "TITLE");

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(x_display, "_NET_WM_STATE", 0);

    XEvent xev = {.type = ClientMessage};
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[0] = 0;
    XSendEvent(x_display, DefaultRootWindow(x_display), 0, SubstructureNotifyMask, &xev);

    EGLNativeWindowType egl_native_window = win;
    EGLNativeDisplayType egl_native_display = x_display;

    ///////////////////////////////
    ///////////////////////////////

    struct EGLDisplay *egl_display = eglGetDisplay(egl_native_display);
    if (egl_display == EGL_NO_DISPLAY)
    {
        logfatal("eglGetDisplay failed");
    }

    // initialize EGL
    EGLint major_version;
    EGLint minor_version;
    if (!eglInitialize(egl_display, &major_version, &minor_version))
    {
        logfatal("eglInitialize failed");
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
        EGL_RENDERABLE_TYPE, GetContextRenderType(egl_display),
        EGL_NONE};

    // choose config
    EGLConfig config;
    if (!eglChooseConfig(egl_display, attrib_list, &config, 1, &num_configs))
    {
        logfatal("eglChooseConfig failed");
    }

    if (num_configs < 1)
    {
        logfatal("failed");
    }

    // create a surface
    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, config, egl_native_window, NULL);
    if (egl_surface == EGL_NO_SURFACE)
    {
        logfatal("eglCreateWindowSurface failed");
    }

    // create a GL context
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT)
    {
        logfatal("eglCreateContext failed");
    }

    // make context current
    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context))
    {
        logfatal("eglMakeCurrent failed");
    }

    //////////////////////////////////
    // initialize shader and program object
    //////////////////////////////////
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

    // load vertex & fragment shaders
    GLuint vertex_shader = LoaderShader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = LoaderShader(GL_FRAGMENT_SHADER, fragment_shader_src);
    if (vertex_shader == 0 || fragment_shader == 0)
    {
        logfatal("compile shader failed");
    }

    // create program object
    GLuint program_object = glCreateProgram();
    if (program_object == 0)
    {
        logfatal("glCreateProgram failed");
    }

    glAttachShader(program_object, vertex_shader);
    glAttachShader(program_object, fragment_shader);

    // link program
    glLinkProgram(program_object);

    // check link status
    GLint linked;
    glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint info_len = 0;
        glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
        {
            char *info_log = calloc(1, info_len);
            glGetProgramInfoLog(program_object, info_len, NULL, info_log);
            logerror("link program failed: %s", info_log);
            free(info_log);
        }

        glDeleteProgram(program_object);

        exit(-1);
    }

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    struct timeval t1, t2;
    struct timezone tz;
    float delta_time;

    gettimeofday(&t1, &tz);

    while (1)
    {
        gettimeofday(&t2, &tz);
        delta_time = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
        t1 = t2;

        usleep(1000000);

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
        glUseProgram(program_object);

        // load vertex data
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        eglSwapBuffers(egl_display, egl_surface);
    }
}