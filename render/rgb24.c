#include "rgb24.h"
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
    "#version 320 es                          \n"
    "precision mediump float;                 \n" // OpenGL ES need explicit precision
    "out vec4 FragColor;                      \n"
    "in vec2 TexCoord;                        \n"
    "uniform sampler2D rgb;                   \n"
    "void main()                              \n"
    "{                                        \n"
    "    FragColor = texture(rgb, TexCoord);  \n"
    "}                                        \n";

static int width_;
static int height_;
static GLuint textures_[1];
GLuint program_;

void rgb24_init(int width, int height)
{
    width_ = width;
    height_ = height;
}

int rgb24_init_shader()
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

    glUniform1f(glGetUniformLocation(program_, "rgb"), 0);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    return 0;
}

void rgb24_init_texture(void *buffer)
{
    glGenTextures(1, textures_);

    load_texture(GL_TEXTURE0, textures_[0], GL_RGB, width_, height_, buffer);
}

void rgb24_update_texture(void *buffer)
{
    update_texture(GL_TEXTURE0, textures_[0], GL_RGB, width_, height_, buffer);
    // use shader program
    glUseProgram(program_);
}