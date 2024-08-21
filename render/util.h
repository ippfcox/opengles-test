#ifndef UTIL_H__
#define UTIL_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

    GLuint load_shader(GLenum type, const char *shader_src);

    GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);

    void load_texture(GLenum texture_id, GLuint texture, GLint format, GLsizei width, GLsizei height, void *buffer);

    void update_texture(GLenum texture_id, GLuint texture, GLint format, GLsizei width, GLsizei height, void *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // UTIL_H__