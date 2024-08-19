#pragma once

#include <iostream>
#include <string>

#include "glad/glad.h"

class Shader
{
public:
    // 读取并构建着色器
    Shader(const char *vertex_path, const char *fragment_path);

    // 使用
    void Use();

    // uniform工具函数
    void SetUniform(const std::string &name, bool value) const;
    void SetUniform(const std::string &name, int value) const;
    void SetUniform(const std::string &name, float value) const;

private:
    // 程序ID
    unsigned int id_;

    enum CheckType
    {
        kVertex,
        kFragment,
        kProgram,
    };

    void CheckCompileErrors(unsigned int shader, CheckType type);
};