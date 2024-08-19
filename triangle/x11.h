#ifndef X11_H__
#define X11_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <X11/Xlib.h>
#include <X11/Xutil.h>

    struct x11_context
    {
        Display *display;
        Atom s_wm_delete_message;
        Window win;
    };

    int x11_window_create(struct x11_context *ctx);

    int x11_window_loop(struct x11_context *ctx, void (*draw_func)(void *data), void *data);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // X11_H__