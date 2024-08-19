#ifndef EGL_H__
#define EGL_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

    struct egl_context
    {
        EGLDisplay *display;
        EGLSurface *surface;
        GLuint program;
    };

    int egl_window_create(struct egl_context *ctx, EGLNativeDisplayType egl_native_display, EGLNativeWindowType egl_native_window);

    int egl_load_shader(struct egl_context *ctx, const char *vertex_shader_src, const char *fragment_shader_src);

    void egl_draw(struct egl_context *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // EGL_H__