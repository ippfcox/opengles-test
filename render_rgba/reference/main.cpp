#include <iostream>
#include <fstream>
#include <thread>
#include "glad/glad.h"
#include "glfw/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

constexpr char kGlslVersion[] = "#version 130";
constexpr char kVertexFilename[] = "src\\shader\\vertex.glsl";
constexpr char kFragmentFilename[] = "src\\shader\\fragment.glsl";
constexpr char kYuvFilename[] = "D:\\GoldChest\\YUV\\Kimono_1920x1080_30_240_I420.yuv";
const int kYuvWidth = 1920;
const int kYuvHeight = 1080;

const unsigned int kScrWidth = 800;
const unsigned int kScrHeight = 480;

void GlfwErrorCallback(int error, const char *description)
{
    std::cerr << "glfw error " << error << ": " << description << std::endl;
}

// 定义一个回调函数，每次窗口大小被调整时调用
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

unsigned int LoadTexture(const char *filename)
{
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    auto data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cout << "stbi_load failed" << std::endl;
        return -1;
    }

    // 创建一个纹理
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // S轴和T轴的纹理环绕方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // 放大缩小时的纹理过滤选项
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 生成纹理
    int fmt = nrChannels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    // 释放加载的图像
    stbi_image_free(data);

    return texture;
}

void LoadYuv(const char *filename, int index, GLuint *textures)
{
    auto yuv_size = kYuvWidth * kYuvHeight * 3 / 2;
    auto data = new char[yuv_size];

    std::ifstream yuv_file(filename, std::ios::in | std::ios::binary);
    yuv_file.seekg(index * yuv_size);
    yuv_file.read(data, yuv_size);
    yuv_file.close();

    glGenTextures(3, textures);
    struct
    {
        GLuint texture;
        int width;
        int height;
        void *data;
    } texture_attr[3] = {
        {textures[0], kYuvWidth, kYuvHeight, data},
        {textures[1], kYuvWidth / 2, kYuvHeight / 2, data + kYuvWidth * kYuvHeight},
        {textures[2], kYuvWidth / 2, kYuvHeight / 2, data + kYuvWidth * kYuvHeight * 5 / 4},
    };

    for (int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, texture_attr[i].texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texture_attr[i].width, texture_attr[i].height, 0, GL_RED, GL_UNSIGNED_BYTE, texture_attr[i].data);
    }

    delete[] data;
}

int main()
{
    glfwSetErrorCallback(GlfwErrorCallback);
    // glfw初始化并配置版本信息
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac OS X

    // 创建一个800x600且名称为LearnOpenGL的窗口
    GLFWwindow *window = glfwCreateWindow(kScrWidth, kScrHeight, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Fialed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // 将窗口的上下文设置为当前线程的主上下文
    glfwMakeContextCurrent(window);
    // 启用垂直同步
    glfwSwapInterval(1); // Enable vsync
    // 注册窗口大小调整回调
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewports / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(kGlslVersion);

    // Load Fonts
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("", 16.0f);
    // ImFont *font = io.Fonts->AddFontFromFileTTF("", 16.0f);
    // IM_ASSERT(font != NULL);

    ImGuiWindowFlags window_flags = 0;
    // window_flags |= ImGuiWindowFlags_MenuBar;      // 启用菜单栏
    window_flags |= ImGuiWindowFlags_NoCollapse;   // 不允许折叠
    window_flags |= ImGuiWindowFlags_NoResize;     // 不允许缩放
    window_flags |= ImGuiWindowFlags_NoTitleBar;   // 禁用标题栏
    window_flags |= ImGuiWindowFlags_NoBackground; // 禁用背景

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 3个点的坐标
    float vertices[] = {
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f};

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3};

    // 生成一个VBO对象，把创建的缓冲绑定到GL_ARRAY_BUFFER目标上，我们使用的任何在GL_ARRAY_BUFFER上的任何缓冲调用都会用来配置VBO
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); // 获取buffer
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定buffer
    // GL_STATIC_DRAW 数据不会或几乎不会被（我们）改变
    // GL_DYNAMIC_DRAW 数据会被（我们）改变很多
    // GL_STREAM_DRAW 数据每次绘制都会改变
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 复制顶点的数据到缓冲内存中

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 描述顶点数据属性
    // - 第一个0对应顶点着色器的location = 0
    // - 第二个3对应vec3的3，也就是一个数据的维度
    // - 第三个GL_FLOAT指定数据类型
    // - 第四个是归一化（0~1），但我们用的-1~1所以是GL_FLASE
    // - 连续的顶点属性之间的间隔，用于表示数据在buffer中的偏移
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)0);
    glEnableVertexAttribArray(0); // 启用顶点属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindVertexArray(VAO);
    // 线框模式，填充模式使用GL_FILL
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GLuint textures[3];

    Shader our_shader(kVertexFilename, kFragmentFilename);
    our_shader.Use();
    our_shader.SetUniform("texture_y", 0);
    our_shader.SetUniform("texture_u", 1);
    our_shader.SetUniform("texture_v", 2);

    int yuv_index = 0;

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 输入
        ProcessInput(window);

        // 渲染
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // （状态设置函数）设置清空屏幕使用的颜色
        glClear(GL_COLOR_BUFFER_BIT);         // 清空屏幕

        if (yuv_index > 100)
            yuv_index = 0;

        LoadYuv(kYuvFilename, yuv_index++, textures);
        if (textures[0] < 0 || textures[1] < 0 || textures[2] < 0)
        {
            std::cout << "LoadYuv failed" << std::endl;
            return -1;
        }

        // Start the Dear ImGui frame
        // ImGui_ImplOpenGL3_NewFrame();
        // ImGui_ImplGlfw_NewFrame();
        // ImGui::NewFrame();

        // 绑定纹理单元
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textures[2]);

        // 激活程序对象
        // our_shader.use();

        // 更新uniform颜色
        // float greenValue = (sin(static_cast<float>(glfwGetTime())) / 2.0f) + 0.5f;
        // int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor"); // 查找uniform变量ourColor的位置，不需要使用过着色器程序
        // glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);            // 更新uniform值必须首先使用过着色器程序

        // glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 3);

        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGuiWindowFlags window_flags = 0;
        // window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        // window_flags |= ImGuiWindowFlags_NoBackground;

        // 加这段是为了让ini
        // We specify a default position/size in case there's no data in the .ini file.
        // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
        // const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
        // ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
        // ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

        // if (!ImGui::Begin("Hello World", NULL, window_flags))
        // {
        // }

        // if (ImGui::BeginMenuBar())
        // {
        //     if (ImGui::BeginMenu("File"))
        //     {
        //         ImGui::MenuItem("?", NULL, false, false);
        //         ImGui::Separator();
        //         if (ImGui::MenuItem("Quit", "Alt+F4"))
        //         {
        //             glfwSetWindowShouldClose(window, true);
        //         }
        //         ImGui::EndMenu();
        //     }

        //     if (ImGui::BeginMenu("Help"))
        //     {
        //         if (ImGui::MenuItem("About"))
        //         {
        //             ImGui::ShowAboutWindow();
        //         }
        //         ImGui::EndMenu();
        //     }
        //     ImGui::EndMenuBar();
        // }

        // ImGui::Text("Text");
        // ImGui::Text("%u, %dx%d", ctx.texture_id, ctx.image_width, ctx.image_height);

        // glTexSubImage2D(GL_TEXTURE_2D, 0, 50, 50, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE, ctx.square_data);

        // ImGui::Image((void *)(intptr_t)ctx.texture_id, ImVec2((float)ctx.image_width, (float)ctx.image_height));
        // ImGui::Image((void *)(intptr_t)textures[0], ImVec2(200.0f, 200.0f));

        // ImGui::End();

        // // Rendering
        // ImGui::Render();
        // int display_w, display_h;
        // glfwGetFramebufferSize(window, &display_w, &display_h);
        // glViewport(0, 0, display_w, display_h);
        // // glClearColor()
        // glClear(GL_COLOR_BUFFER_BIT);
        // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        // For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        // {
        //     GLFWwindow *backup_current_context = glfwGetCurrentContext();
        //     ImGui::UpdatePlatformWindows();
        //     ImGui::RenderPlatformWindowsDefault();
        //     glfwMakeContextCurrent(backup_current_context);
        // }

        // 交换颜色缓冲，双缓冲，这个在玩easyx的时候就有体会了
        glfwSwapBuffers(window);
        // 检查是否有键盘输入、鼠标移动事件、更新窗口状态，调用相应回调
        glfwPollEvents();

        // std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    glfwTerminate();

    return 0;
}