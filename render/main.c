#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "log.h"
#include "nv24.h"
#include "rgb24.h"

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540
#define WINDOW_TITLE "Render"
#define NV24_FILENAME "../assets/Kimono_1920x1080_30_1_NV24.yuv"
#define NV24_WIDTH 1920
#define NV24_HEIGHT 1080
#define RGB24_FILENAME "../assets/Kimono_1920x1080_30_1_RGB24.yuv"
#define RGB24_WIDTH 1920
#define RGB24_HEIGHT 1080

enum pixel_format
{
    NV24,
    RGB24,
};

static enum pixel_format fmt = RGB24;

static EGLint get_context_render_type(EGLDisplay egl_display)
{
    const char *extensions = eglQueryString(egl_display, EGL_EXTENSIONS);
    if (extensions != NULL && strstr(extensions, "EGL_KHR_create_context"))
        return EGL_OPENGL_ES3_BIT_KHR;
    return EGL_OPENGL_ES2_BIT;
}

int main()
{
    set_log_level(LOG_LEVEL_DEBUG);

    void (*init_func)(int width, int heith);
    int (*init_shader_func)();
    void (*init_texture_func)(void *buffer);
    void (*update_texture_func)(void *buffer);
    char *yuv_filename;
    int yuv_width;
    int yuv_height;
    size_t yuv_size;

    switch (fmt)
    {
    case NV24:
        init_func = nv24_init;
        init_shader_func = nv24_init_shader;
        init_texture_func = nv24_init_texture;
        update_texture_func = nv24_update_texture;
        yuv_filename = NV24_FILENAME;
        yuv_width = NV24_WIDTH;
        yuv_height = NV24_HEIGHT;
        yuv_size = NV24_WIDTH * NV24_HEIGHT * 3;
        break;
    case RGB24:
        init_func = rgb24_init;
        init_shader_func = rgb24_init_shader;
        init_texture_func = rgb24_init_texture;
        update_texture_func = rgb24_update_texture;
        yuv_filename = RGB24_FILENAME;
        yuv_width = RGB24_WIDTH;
        yuv_height = RGB24_HEIGHT;
        yuv_size = RGB24_WIDTH * RGB24_HEIGHT * 3;
        break;
    }

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
        0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
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
    XStoreName(x11_display, x11_window, WINDOW_TITLE);

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
    //                       VBO/VAO/EBO                                      //
    ////////////////////////////////////////////////////////////////////////////

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO); // create VAO(vertex Array object)
    glGenBuffers(1, &VBO);      // create VBO(vertex buffer object)
    glGenBuffers(1, &EBO);      // create EBO(element buffer object)

    glBindVertexArray(VAO); // bind VAO, for buffer config & vertex config

    // vertex:                                   texture:
    // +------------------------------+
    // |                              |
    // |      [2]------------[1]      |         [2]------------[1]
    // |       |    (0, 0)    |       |          |              |
    // |      [3]------------[0]      |         [3]------------[0]
    // |                              |
    // +------------------------------+
    // (x, y, z, s, t), (x, y, z) for vertex location, (s, t) for texture
    float vertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f};
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind VBO, pass vertex data (`vertices`) to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // two triagnle -> rectangle
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // configure EBO, pass index data (`indices`) to GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // config vertex attribute 0 pointer, location
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)0);
    glEnableVertexAttribArray(0);
    // config index attribute 1 pointer, texture location
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // clear binding for VAO
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    glBindVertexArray(GL_NONE);

    ////////////////////////////////////////////////////////////////////////////
    //                              shader                                    //
    ////////////////////////////////////////////////////////////////////////////

    init_func(yuv_width, yuv_height);
    if (init_shader_func() != 0)
    {
        logerror("nv24_init_shader failed");
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////
    //                             buffer                                     //
    ////////////////////////////////////////////////////////////////////////////

    // read NV24
    void *buffer = calloc(1, yuv_size);
    FILE *fp = fopen(yuv_filename, "rb");
    fread(buffer, 1, yuv_size, fp);
    fclose(fp);

    ////////////////////////////////////////////////////////////////////////////
    //                            texture                                     //
    ////////////////////////////////////////////////////////////////////////////

    init_texture_func(buffer);

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

        update_texture_func(buffer);

        // clear window
        glClear(GL_COLOR_BUFFER_BIT);
        // bind VAO
        glBindVertexArray(VAO);
        // draw rectangle
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // unbind
        glBindVertexArray(0);
        glUseProgram(0);

        eglSwapBuffers(egl_display, egl_surface);
    }
}