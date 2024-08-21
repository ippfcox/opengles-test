#include "nv24.h"
#include "log.h"

static char vertex_shader_src[] =
    "#version 320 es                          \n"
    "layout (location = 0) in vec3 aPos;      \n"
    "layout (location = 1) in vec2 aTexCoord; \n"
    "out vec2 TexCoord;                       \n"
    "void main()                              \n"
    "{                                        \n"
    "    gl_Position = vec4(aPos, 1.0);       \n"
    "    TexCoord = aTexCoord;                \n"
    "}                                        \n";
static char fragment_shader_src[] =
    "#version 320 es                                                    \n"
    "precision mediump float;                                           \n" // OpenGL ES need explicit precision
    "out vec4 FragColor;                                                \n"
    "in vec2 TexCoord;                                                  \n"
    "uniform sampler2D y_texture;                                       \n"
    "uniform sampler2D uv_texture;                                      \n"
    "void main()                                                        \n"
    "{                                                                  \n"
    "    vec3 yuv;                                                      \n"
    "    yuv.x = texture(y_texture, TexCoord).r;                        \n"
    "    yuv.yz = texture(uv_texture, TexCoord).rg - vec2(0.5, 0.5);    \n"
    "    vec3 rgb = mat3(1,       1,        1,                          \n"
    "                    0,       -0.39465, 2.03211,                    \n"
    "                    1.13983, -0.58060, 0.0     ) * yuv;            \n"
    "    FragColor = vec4(rgb, 1.0);                                    \n"
    "}                                                                  \n";

static int width_;
static int height_;
static GLuint textures_[2];
GLuint program_;

void nv24_init(int width, int height)
{
    width_ = width;
    height_ = height;
}

int nv24_init_shader()
{
    GLuint vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader == 0)
    {
        logerror("load_shader VERTEX failed");
        return -1;
    }

    GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    if (fragment_shader == 0)
    {
        logerror("load_shader FRAGMENT failed");
        return -1;
    }

    program_ = link_program(vertex_shader, fragment_shader);
    if (program_ == 0)
    {
        logerror("link_program failed");
        return -1;
    }

    glUseProgram(program_);

    glUniform1i(glGetUniformLocation(program_, "y_texture"), 0); // 0 for GL_TEXTURE0
    glUniform1i(glGetUniformLocation(program_, "uv_texture"), 1);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    return 0;
}

void nv24_init_texture(void *buffer)
{
    glGenTextures(2, textures_);

    load_texture(GL_TEXTURE0, textures_[0], GL_RED, width_, height_, buffer);
    load_texture(GL_TEXTURE1, textures_[1], GL_RG, width_, height_, buffer + width_ * height_);
}

void nv24_update_texture(void *buffer)
{
    update_texture(GL_TEXTURE0, textures_[0], GL_RED, width_, height_, buffer);
    update_texture(GL_TEXTURE1, textures_[1], GL_RG, width_, height_, buffer + width_ * height_);
    // use shader program
    glUseProgram(program_);
}