#ifndef RGB24_H__
#define RGB24_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "util.h"

    void rgb24_init(int width, int height);
    int rgb24_init_shader();
    void rgb24_init_texture(void *buffer);
    void rgb24_update_texture(void *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // RGB24_H__