#include <iostream>
#include <fstream>
#include <sstream>
#include "shader.hpp"

Shader::Shader(const char *vertex_path, const char *fragment_path)
{
    // 1. 从简路径中获取顶点、片段着色器
    std::string vertex_code, fragment_code;
    std::ifstream vertex_shader_file, fragment_shader_file;
    vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // 打开文件
        vertex_shader_file.open(vertex_path);
        fragment_shader_file.open(fragment_path);
        std::stringstream vertex_shader_stream, fragment_shader_stream;
        // 读取文件
        vertex_shader_stream << vertex_shader_file.rdbuf();
        fragment_shader_stream << fragment_shader_file.rdbuf();
        // 关闭文件处理器
        vertex_shader_file.close();
        fragment_shader_file.close();
        // 转换数据流到string
        vertex_code = vertex_shader_stream.str();
        fragment_code = fragment_shader_stream.str();
    }
    catch (std::ifstream::failure &)
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << std::endl;
    }
    const char *vertex_shader_code = vertex_code.c_str();
    const char *fragment_shader_code = fragment_code.c_str();

    // 2. 编译着色器
    unsigned int vertex, fragment;

    // 顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, CheckType::kVertex);

    // 片段着色器
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, CheckType::kFragment);

    // 着色器程序
    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glLinkProgram(id_);
    CheckCompileErrors(id_, CheckType::kProgram);

    // 删除着色器，已经链接到程序中了
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use()
{
    glUseProgram(id_);
}

void Shader::SetUniform(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<int>(value));
}

void Shader::SetUniform(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetUniform(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::CheckCompileErrors(unsigned int shader, CheckType type)
{
    int success;
    char info_log[1024];
    switch (type)
    {
    case CheckType::kVertex:
    case CheckType::kFragment:
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, info_log);
            std::cout << "GL_COMPILE_STATUS: " << info_log << std::endl;
        }
        break;

    case CheckType::kProgram:
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, info_log);
            std::cout << "GL_LINK_STATUS: " << info_log << std::endl;
        }
        break;
    }
}