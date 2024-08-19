#include "x11.h"
#include "egl.h"
#include "log.h"

int main()
{
    set_log_level(LOG_LEVEL_DEBUG);

    struct x11_context x11_ctx = {0};
    x11_window_create(&x11_ctx);

    struct egl_context egl_ctx = {0};
    egl_window_create(&egl_ctx, x11_ctx.display, x11_ctx.win);

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
    egl_load_shader(&egl_ctx, vertex_shader_src, fragment_shader_src);

    x11_window_loop(&x11_ctx, (void (*)(void *))egl_draw, &egl_ctx);
}