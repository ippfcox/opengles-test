#include <unistd.h>
#include "log.h"
#include "x11.h"

int x11_window_create(struct x11_context *ctx)
{
    if (!ctx)
    {
        logerror("invalid x11_context");
        return -1;
    }

    // X11 native display initialization
    ctx->display = XOpenDisplay(NULL);
    if (ctx->display == NULL)
    {
        logerror("XOpenDisplay failed");
        return -1;
    }

    Window root = DefaultRootWindow(ctx->display);
    XSetWindowAttributes swa = {.event_mask = ExposureMask | PointerMotionMask | KeyPressMask};
    ctx->win = XCreateWindow(
        ctx->display, root,
        0, 0, 100, 100, 0,
        CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask,
        &swa);
    ctx->s_wm_delete_message = XInternAtom(ctx->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(ctx->display, ctx->win, &ctx->s_wm_delete_message, 1);

    XSetWindowAttributes xattr = {.override_redirect = 0};
    XChangeWindowAttributes(ctx->display, ctx->win, CWOverrideRedirect, &xattr);

    XWMHints hints = {.input = 1, .flags = InputHint};
    XSetWMHints(ctx->display, ctx->win, &hints);

    // show window
    XMapWindow(ctx->display, ctx->win);
    XStoreName(ctx->display, ctx->win, "TITLE");

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(ctx->display, "_NET_WM_STATE", 0);

    XEvent xev = {.type = ClientMessage};
    xev.xclient.window = ctx->win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[0] = 0;
    XSendEvent(ctx->display, DefaultRootWindow(ctx->display), 0, SubstructureNotifyMask, &xev);

    return 0;
}

int x11_window_loop(struct x11_context *ctx, void (*draw_func)(void *data), void *data)
{
    if (!ctx)
    {
        logerror("invalid x11_context");
        return -1;
    }

    XEvent xev;

    int stop = 0;

    while (!stop)
    {
        while (XPending(ctx->display))
        {
            XNextEvent(ctx->display, &xev);
            switch (xev.type)
            {
            case KeyPress:
            {
                KeySym key;
                char key_char;
                if (XLookupString(&xev.xkey, &key_char, 1, &key, 0))
                {
                    loginfo("keypress: %c", key_char);
                }
            }
            break;
            case ClientMessage:
            {
                if (xev.xclient.data.l[0] == ctx->s_wm_delete_message)
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

        if (draw_func)
            draw_func(data);
        else
            usleep(10000);
    }

    return 0;
}