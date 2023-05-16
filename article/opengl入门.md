## 前言
opengl作为C++一块为数不多的阵地，是一个想要涉足可视化开发的C++工程师绕不开的一块内容
但是网上的教程和书籍大多过于老旧，这里特别感谢cherno的教程。

## OpenGL是什么？
opengl不是一个框架，也不是一种代码实现，它更像是一种标准。（类似于C++STL）
也就是说，opengl除了规定接口的输入输出，基本功能之外，没有强制实现标准
所以，我们可以在不同的厂商和作者那里看见不同版本的opengl代码。
而且，他也不是开源的，只有少数作者愿意开源实现。

## 除了opengl我们还有哪些选择？
在win下，我们还有dx11,有dx3D
在mac下，我们还有Metal
在linux下，则是……算了，linux还是用跨平台的库吧，例如vulkan


## 使用opengl的环境准备
我们这里使用宇宙IDE-v- vs(2019)以及两个opengl的相关库glew 

## opengl中的基础概念
1.缓冲区是什么？
缓冲区其实就是一段连续的内存地址，它代表的含义渲染的信息。

2.着色器是什么？
着色器是一段跑在GPU上的程序，他主要处理的事儿就是将输入进行某些数学变换后，输出给GPU。具体怎么写后面有实例。

顶点着色器：
顾名思义，就是一个着色器代表一个或者一类顶点。

片段着色器：


3.索引缓冲区

定义：缓冲区代表一段内存，且这意味着它实际上在显卡显存（Video RAM）上。
意义：定义一些数据来表示三角形，把它放入显卡的 VRAM 中，告诉显卡如何读取和解释这些数据


## 使用opengl画出第一个三角形
在这里，我们会用传统的opengl与现代opengl分别实现画一个三角形的功能
所以，我们还是先介绍一下什么是传统的opengl，什么又是现代的opengl.
传统与现代与否，在计算机的发展上，往往区分了功能强大与否。
传统的api往往只用作简单的功能实现，有一种浓浓的时代工业风，而现代的玩意儿大多都是花里胡哨的。
从学习和使用上说，现代opengl对着色器的处理要更加精细。

话不多说，直接开搞。

```
首先是传统的opengl：

```
#include <glew.h>
#include <glfw3.h>
#include <iostream>
using namespace std;

int main()
{
    /* Initialize the library */
    if(!glfwInit())
        return -1;

    /* Create a Windowed mode and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while(!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

+       glBegin(GL_TRIANGLES);
+       glVertex2f(-0.5f, -0.5f);
+       glVertex2f( 0.0f,  0.5f);
+       glVertex2f( 0.5f, -0.5f);
+       glEnd();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
```
```

但是传统的opengl能干的事情毕竟有限，所以后续还是会使用现代opengl
```
1.首先，定义一个缓冲区,缓冲区代表一段内存，且这意味着它实际上在显卡显存（Video RAM）上。
思路为定义一些数据来表示三角形，把它放入显卡的 VRAM 中，告诉显卡如何读取和解释这些数据，然后还需要发出 DrawCall 绘制指令。
```
unsigned int buffer;
glGenBuffers(1, &buffer);
```



下面是懒人版代码：
```
#include "Application.h"
#include<iostream>

#include  <glew.h>
#include <glfw3.h>
static unsigned int CompileShader( unsigned int itype,const std::string& source)
{
    unsigned int id = glCreateShader(itype);
    const char* src = source.c_str();
    glShaderSource(id,1,&src,nullptr);
    glCompileShader(id);

    //error handle
    int result;
    glGetShaderiv(id,GL_COMPILE_STATUS,&result);
    if (result==GL_FALSE)
    {
        int length;
        glGetShaderiv(id,GL_INFO_LOG_LENGTH,&length);
        //char* message = (char*)alloca(length*sizeof(char));
        char message[1024];
        glGetShaderInfoLog(id,1024,NULL, message);
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

static unsigned int createShader(const std::string& VertexShader, const std::string& FragShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, VertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, FragShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);


    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;


    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    GLenum res = glewInit();
    if (res == GLEW_OK) {
        std::cout << "ok" << std::endl;
        std::cout << glGetString(GL_VERSION) << std::endl;
    }

    float positions[6] = {
        -0.5f, -0.5f,
        0.5f, 0.5f,
        0.5f,-0.5f
    };
    
    unsigned int buffer;
    glGenBuffers(1,&buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float),positions,GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(float)*2,0);

    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::string vertexShader =

        "#version 330 core\n"
        "layout(location =0)in vec4 position;\n"
        "void main()\n"
        "{\n"
        "gl_Position=position;\n"
        "}\n";

    std::string fragShader =
        "#version 330 core\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   color=vec4(1.0f,0.0f,0.0f,1.0f);\n"
        "}\n";


    unsigned int shader = createShader(vertexShader, fragShader);

    glUseProgram(shader);


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES,0,3);
        //即时模式绘制
        /*glBegin(GL_TRIANGLES);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f(0.5f, 0.5f);
        glVertex2f(0.5f,-0.5f);
        glEnd();*/

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
```

## 使用索引缓冲区的三角形

## 简单的错误处理
opengl的错误通常会得到一个黑框，我们需要一种方式去发现错误的地方
2种方式
1.gl get error
```
static void GlClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

static void GlCheckError()
{
    while (GLenum error= glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ")" << std::endl;
    }
}

        GlClearError();
        glDrawElements(GL_TRIANGLES,6,GL_INT,nullptr);//必须使用UNSIGNED
        GlCheckError();
```

升级版1.0
```
#define ASSERT(x) if (!(x)) __debugbreak();
#define GlCall(x) GlClearError();\
    x;\
    ASSERT(GlCheckError());

static void GlClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

static bool GlCheckError()
{
    while (GLenum error= glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ")" << std::endl;
        return false;
    }
    return true;
}    

 GlCall( glDrawElements(GL_TRIANGLES,6,GL_INT,nullptr));//必须使用UNSIGNED
```

升级版2.0
```
#define ASSERT(x) if (!(x)) __debugbreak();
#define GlCall(x) GlClearError();\
    x;\
    ASSERT(GlCheckError(#x,__FILE__,__LINE__));

static void GlClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

static bool GlCheckError(const char * function,const char *file,int line)
{
    while (GLenum error= glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ")" << function <<
            " "<< file <<" "<< line << std::endl;
        return false;
    }
    return true;
}

  GlCall( glDrawElements(GL_TRIANGLES,6,GL_INT,nullptr));//必须使用UNSIGNED
```
2.gl debug message callback(4.3及以上版本才可以拥有)


## uniform

uniform与属性的异同
uniform是一次性的，改一次，就渲染一次，或者说实在画的时候才进行修改
