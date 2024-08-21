#ifndef NV24_H__
#define NV24_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "util.h"

    void nv24_init(int width, int height);
    int nv24_init_shader();
    void nv24_init_texture(void *buffer);
    void nv24_update_texture(void *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NV24_H__