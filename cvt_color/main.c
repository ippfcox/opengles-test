#include <string.h>
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

int main()
{
    set_log_level(LOG_LEVEL_DEBUG);

    ////////////////////////////////////////////////////////////////////////////
    //                          EGL initialize                                //
    ////////////////////////////////////////////////////////////////////////////

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
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
    EGLConfig egl_config;
    if (!eglChooseConfig(egl_display, attrib_list, &egl_config, 1, &num_configs) || num_configs < 1)
    {
        logerror("eglChooseConfig failed");
        return -1;
    }

    // create a surface
    EGLSurface egl_surface = eglCreatePbufferSurface(egl_display, egl_config, NULL);
    if (egl_surface == EGL_NO_SURFACE)
    {
        logerror("eglCreateWindowSurface failed");
        return -1;
    }

    // create a GL context
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
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
}