# OpenGL

本文总结OpenGL的基本概念及其在音视频处理中的应用。

## 概述

OpenGL（Open Graphics Library）是一个跨语言、跨平台的图形API，用于渲染2D和3D矢量图形。它广泛应用于游戏开发、CAD、虚拟现实以及音视频处理等领域。

## 状态机

OpenGL采用状态机（State Machine）设计理念，所有的渲染操作都依赖于当前的状态设置。OpenGL的状态通常被称为OpenGL上下文(Context)。我们通常使用如下途径去更改OpenGL状态：设置选项，操作缓冲。最后，我们使用当前OpenGL上下文来渲染。

当使用OpenGL的时候，我们会遇到一些状态设置函数(State-changing Function)，这类函数将会改变上下文。以及状态使用函数(State-using Function)，这类函数会根据当前OpenGL的状态执行一些操作。

## 函数

OpenGL提供了丰富的函数接口，用于创建和管理图形资源、设置渲染状态以及执行渲染操作。

OpenGL函数通常以`gl`开头，后跟功能描述。

## 对象

OpenGL使用对象(Object)来管理和存储图形数据。

可以把对象看做一个C风格的结构体(struct)，对象 (Object) 本质上是： 显存（GPU）中的一块资源数据 + 显卡驱动里的一组状态配置。

```CPP
struct object_name {
    float  option1;
    int    option2;
    char[] name;
};
```

在`C++`代码里，我们通过一个无类型指针（`GLuint`）来引用这个对象：

```CPP
GLuint object_name;
```

标准的OpenGL对象操作流程如下：

1. 生成对象名称（ID）：使用`glGen*`函数生成一个或多个对象名称（ID）。
2. 绑定对象：使用`glBind*`函数将生成的对象名称绑定到当前上下文中，设为**当前目标**。
3. 设置对象参数：使用`gl*Parameter`函数设置对象的各种参数。
4. 使用对象：在渲染过程中使用绑定的对象。
5. 删除对象：使用`glDelete*`函数删除不再需要的对象，释放资源。

常见的GL对象包括：

- 纹理对象（Texture Object）
- 缓冲对象（Buffer Object）
- 着色器对象（Shader Object）
- 帧缓冲对象（Framebuffer Object）
- 渲染缓冲对象（Renderbuffer Object）

### 常见对象

#### 纹理对象（Texture Object）

纹理对象用于存储图像数据，通常用于渲染2D和3D图形。纹理对象的常见操作包括：

- 生成纹理对象：使用`glGenTextures`函数生成一个或多个纹理对象。
- 绑定纹理对象：使用`glBindTexture`函数将生成的纹理对象绑定到当前上下文中，设为**当前目标**。
- 设置纹理参数：使用`glTexParameteri`函数设置纹理的各种参数。
- 设置纹理数据：使用`glTexImage2D`函数设置纹理数据。
- 使用纹理：在渲染过程中使用绑定的纹理。
- 删除纹理对象：使用`glDeleteTextures`函数删除不再需要的纹理对象，释放资源。

#### 缓冲对象（Buffer Object）

缓冲对象用于存储顶点数据、索引数据等，通常用于渲染3D模型。缓冲对象的常见操作包括：

- 生成缓冲对象：使用`glGenBuffers`函数生成一个或多个缓冲对象。
- 绑定缓冲对象：使用`glBindBuffer`函数将生成的缓冲对象绑定到当前上下文中，设为**当前目标**。
- 设置缓冲数据：使用`glBufferData`函数设置缓冲数据。
- 使用缓冲：在渲染过程中使用绑定的缓冲。
- 删除缓冲对象：使用`glDeleteBuffers`函数删除不再需要的缓冲对象，释放资源。

将缓冲对象与不同的缓冲目标（Buffer Target）结合使用，可以实现不同的功能。例如，使用`GL_ARRAY_BUFFER`目标存储顶点数据，使用`GL_ELEMENT_ARRAY_BUFFER`目标存储索引数据。

* VBO（Vertex Buffer Object）：用于存储顶点数据。`glBindBuffer(GL_ARRAY_BUFFER, bufferId);`
* IBO（Index Buffer Object）：用于存储索引数据。`glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferId);`
* PBO（Pixel Buffer Object）：用于存储像素数据。`glBindBuffer(GL_PIXEL_UNPACK_BUFFER, bufferId);`

#### VBO（Vertex Buffer Object）

VBO用于存储顶点数据，通常与VAO结合使用以简化顶点数据的管理和渲染过程。VBO的常见操作包括：

- 生成VBO：使用`glGenBuffers`函数生成一个或多个VBO。
- 绑定VBO：使用`glBindBuffer`函数将生成的VBO绑定到当前上下文中，设为**当前目标**。
- 设置VBO数据：使用`glBufferData`函数设置VBO数据。
- 使用VBO：在渲染过程中使用绑定的VBO。
- 删除VBO：使用`glDeleteBuffers`函数删除不再需要的VBO，释放资源。

```CPP
// 准备顶点数据 (全屏矩形)
// 格式: x, y (顶点) + u, v (纹理)
static const GLfloat vertices[] = {
    // 顶点坐标(全屏)   // 纹理坐标(翻转Y轴以适应视频方向)
    -1.0f, -1.0f,      0.0f, 1.0f,  // 左下
    1.0f, -1.0f,      1.0f, 1.0f,  // 右下
    -1.0f,  1.0f,      0.0f, 0.0f,  // 左上
    1.0f,  1.0f,      1.0f, 0.0f   // 右上
};
// 创建 VBO (显存)
glGenBuffers(1, &vboId);
glBindBuffer(GL_ARRAY_BUFFER, vboId);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
// 解绑 VBO
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

#### VAO（Vertex Array Object）

VAO用于存储顶点数据的配置信息，简化顶点数据的管理和渲染过程。如果不使用VAO，每次渲染时都需要重新绑定VBO并设置顶点属性指针，这会导致代码冗长且效率低下。使用VAO后，只需绑定一次VAO即可完成所有顶点数据的配置。

VAO的常见操作包括：

- 生成VAO：使用`glGenVertexArrays`函数生成一个或多个VAO。
- 绑定VAO：使用`glBindVertexArray`函数将生成的VAO绑定到当前上下文中，设为**当前目标**。
- 设置VAO配置：使用`glVertexAttribPointer`函数设置顶点数据的配置信息。
- 使用VAO：在渲染过程中使用绑定的VAO。
- 删除VAO：使用`glDeleteVertexArrays`函数删除不再需要的VAO，释放资源。

```CPP
// 在 initializeGL() 中

// 1. 创建 (Generate)
// 参数1: 你要生成几个 VAO？ (这里是 1 个)
// 参数2:生成的 ID 存到哪里？ (存到 &vaoId 里)
glGenVertexArrays(1, &vaoId);

// 2. 绑定 (Bind)
// 告诉显卡：“接下来的 VBO 绑定和属性指针设置，都记录到 vaoId 这个档案袋里”
glBindVertexArray(vaoId);

// ... 接下来写 VBO 绑定和 setAttributeBuffer ...
// ...

// 3. 解绑 (Release)
// 告诉显卡：“录制结束，合上档案袋”
glBindVertexArray(0);
```

在`paintGL()`中使用VAO：

```CPP
// 1. 绑定 VAO
glBindVertexArray(vaoId);
// 2. 绘制
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
// 3. 解绑 VAO
glBindVertexArray(0);
```

不需要进行重复的`VBO`绑定和属性指针设置，直接绑定`VAO`即可。

### 对象插槽

OpenGL对象通常绑定到特定的插槽（Slot）上，不同类型的对象有不同的绑定点。互相不冲突，通过特定的函数可以使用不同的插槽。

例如，纹理对象通过`glBindTexture`函数绑定到纹理插槽，而缓冲对象通过`glBindBuffer`函数绑定到缓冲插槽。

此外还可以通过`glActiveTexture`函数来切换当前活动的接口槽。

```CPP
// 1. 生成 3 个纹理对象
glGenTextures(3, textureIds);

// 2. 激活 0 号接口
glActiveTexture(GL_TEXTURE0); 
glBindTexture(GL_TEXTURE_2D, textureIds[0]); // 把 Y 插在 0 号口

// 3. 激活 1 号接口
glActiveTexture(GL_TEXTURE1); 
glBindTexture(GL_TEXTURE_2D, textureIds[1]); // 把 U 插在 1 号口
// 此时 Y 依然插在 0 号口上，互不影响！

// 4. 激活 2 号接口
glActiveTexture(GL_TEXTURE2); 
glBindTexture(GL_TEXTURE_2D, textureIds[2]); // 把 V 插在 2 号口
```

### 设置对象参数

#### 纹理参数

纹理对象有很多参数可以设置，常用的参数包括：

- 纹理过滤模式（Texture Filtering Mode）：决定纹理在缩放时如何采样。
- 纹理环绕模式（Texture Wrapping Mode）：决定纹理坐标超出[0,1]范围时的处理方式。
- 纹理格式（Texture Format）：决定纹理数据的存储格式。

glTexParameter函数用于设置纹理参数，例如：

```CPP
// 设置纹理过滤 (线性插值)
// 放大 (Magnification)：窗口比视频大
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 缩小 (Minification)：窗口比视频小
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// 设置纹理环绕模式 (边缘拉伸)
// 3. 横向坐标 (S轴) 超出范围时，锁死在边缘，不要去读对面的像素
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
// 4. 纵向坐标 (T轴) 超出范围时，锁死在边缘，不要去读对面的像素
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

### 设置对象数据

#### 纹理数据

使用`glTexImage2D`函数可以设置纹理数据，例如：

```CPP
// 设置纹理数据
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
```

* 参数1：目标纹理类型（Target），例如`GL_TEXTURE_2D`表示二维纹理。
* 参数2：纹理的细节级别（Level），通常设置为0，
* 参数3：纹理的内部格式（Internal Format），例如`GL_RGBA`表示每个像素包含红、绿、蓝、透明四个分量。
* 参数4：纹理的宽度（Width），以像素为单位。
* 参数5：纹理的高度（Height），以像素为单位。
* 参数6：边框（Border），通常设置为0。
* 参数7：纹理数据的格式（Format），例如`GL_RGBA`表示数据包含红、绿、蓝、透明四个分量。
* 参数8：纹理数据的数据类型（Type），例如`GL_UNSIGNED_BYTE`表示每个分量使用无符号字节存储。
* 参数9：指向纹理数据的指针（Data），如果为`NULL`，则只分配内存但不初始化数据。

### 使用对象

#### 纹理使用

使用`glDrawArrays`或`glDrawElements`函数可以使用纹理数据进行渲染，例如：

```CPP
// 使用纹理数据进行渲染
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
```

### 绑定着色器变量

使用`glUniform1`函数或`glUniform2f`函数等可以设置着色器变量的值，例如：

```CPP
// 激活 Shader (必须先激活才能修改 Uniform！)
glUseProgram(programId);

// 设置着色器变量的值
glUniform1i(glGetUniformLocation(programId, "textureY"), 0);
glUniform1i(glGetUniformLocation(programId, "textureU"), 1);
glUniform1i(glGetUniformLocation(programId, "textureV"), 2);
```

* `glGetUniformLocation`
    * 显卡编译完 Shader 后，会给每个 uniform 变量分配一个整数编号（Location）。这一步就是通过名字（字符串）去查这个编号。
* `glUniform1i`
    * 把整数 0 传给 uniform 变量 textureY，表示 Y 纹理插在 0 号接口上。

注意，使用着色器变量之前必须先激活着色器程序（`glUseProgram`）。

`glGetUniformLocation 涉及字符串匹配，速度很慢`，因此建议在初始化时保存这些编号，避免每次渲染时都去查。

在`initializeGL`时查好所有变量的 Location，存到成员变量里（比如 m_loc_tex_y）。

在`paintGL`时直接用这些 Locatio

### 顶点数据

顶点数据用于描述3D模型的顶点信息，通常存储在缓冲对象（Buffer Object）中。顶点数据可以存储在VBO（Vertex Buffer Object）中。

顶点就是数据集合。

* 位置（Position）：描述顶点在3D空间中的位置。在GLSL中通常用`vec3`或`vec4`表示。
* 颜色（Color）：描述顶点的颜色信息。在GLSL中通常用`vec3`或`vec4`表示。
* 纹理坐标（Texture Coordinate）：描述顶点在纹理上的位置。在GLSL中通常用`vec2`或`vec3`表示。
* 法线（Normal）：描述顶点的法线信息，用于光照计算。在GLSL中通常用`vec3`或`vec4`表示。

### 顶点属性

顶点属性用于描述顶点数据的格式和布局，常用的顶点属性包括位置、颜色、纹理坐标等。

属性不属于某个对象，而是属于当前OpenGL上下文。

`glVertexAttribPointer`函数用于设置顶点属性，例如：

```CPP
// 设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offset);
```

* 参数1：属性index（Location），对应着色器中的`layout(location = 0) in vec3 position;`

    ```OpenGL Shading Language
    // layout(location = 0) 就是指 index = 0
    layout(location = 0) in vec3 aPos; 

    // layout(location = 1) 就是指 index = 1
    layout(location = 1) in vec2 aTexCoord;
    ```

* 参数2：每个顶点属性的组件数量（1~4），例如位置有3个分量（x,y,z），纹理坐标有2个分量（u,v）。在GLSL中float, vec2, vec3, vec4分别对应1, 2, 3, 4个分量。

* 参数3：数据类型，例如`GL_FLOAT`表示浮点数。VBO里存的格式。

* 参数4：是否归一化（Normalize），对于整数数据类型，设置为`GL_TRUE`表示将数据归一化到[0,1]或[-1,1]范围内，`GL_FALSE`表示不进行归一化。对于浮点数类型，该参数通常设置为`GL_FALSE`。

* 参数5：步长（Stride），表示连续顶点属性之间的字节偏移量。如果顶点属性是紧密排列的，可以设置为0，OpenGL会自动计算步长。`VBO`是`[XYZ UV, XYZ UV, ...]`，那么步长就是`sizeof(XYZ) + sizeof(UV)`。

* 参数6：指针（Pointer），表示顶点属性在VBO中的偏移量。

例子：

```CPP
float vertices[] = {
    // 顶点 1
    // 位置 (3个)    // 纹理 (2个)
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   
    // 顶点 2
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   
    // ...
};
```

一行的总长度 = 5 个 float

配置代码，管道0：负责位置XYZ

‵‵‵CPP
glVertexAttribPointer(
    0,                  // 对应 Shader 的 location = 0
    3,                  // 每次读 3 个数 (x,y,z)
    GL_FLOAT,           // 是 float
    GL_FALSE,           // 不归一化
    5 * sizeof(float),  // 步长：读完这3个，要跨过 5个float 的距离才能到下一行
    (void*)0            // 偏移：从数组最开头开始读
);
glEnableVertexAttribArray(0); // 打开阀门
```

管道1：负责纹理UV

```CPP
glVertexAttribPointer(
    1,                  // 对应 Shader 的 location = 1
    2,                  // 每次读 2 个数 (u,v)
    GL_FLOAT,           // 是 float
    GL_FALSE,           // 不归一化
    5 * sizeof(float),  // 步长：和上面一样，也是每行 5个float
    (void*)(3 * sizeof(float)) // 偏移：跳过前面的 3个XYZ，从第4个数开始读
);
glEnableVertexAttribArray(1); // 打开阀门
```

### 绘制

使用`glDrawArrays`或`glDrawElements`函数可以执行绘制操作，例如：

```CPP
// 使用顶点数据进行绘制
glDrawArrays(GL_TRIANGLES, 0, vertexCount);
```

* 参数1：绘制模式（Mode），例如`GL_TRIANGLES`表示绘制三角形，`GL_TRIANGLE_STRIP`表示绘制三角形条带等。
* 参数2：起始索引（First），表示从哪个顶点开始绘制
* 参数3：顶点数量（Count），表示要绘制的顶点数量

如果是`GL_TRIANGLE_STRIP`模式，顶点顺序如下：

```text
顶点0
顶点1 顶点2
顶点3 顶点4
```

也就是说每次增加一个顶点，就会形成一个新的三角形。

## 着色器

着色器（Shader）是运行在GPU上的小程序，用于处理图形渲染的各个阶段。常见的着色器类型包括顶点着色器（Vertex Shader）、片段着色器（Fragment Shader）等。

### 顶点着色器

顶点着色器用于处理顶点数据，负责将顶点从模型空间转换到裁剪空间。顶点着色器的输入通常包括顶点位置、颜色、纹理坐标等，输出通常包括裁剪空间位置和传递给片段着色器的数据。

```GLSL
attribute vec4 vertexIn;    // 顶点坐标
attribute vec2 textureIn;   // 纹理坐标
varying vec2 textureOut;    // 传给片段着色器的坐标
void main(void) {
    gl_Position = vertexIn;
    textureOut = textureIn;
}
```

### 片段着色器

片段着色器用于处理片段数据，负责计算每个像素的最终颜色。片段着色器的输入通常包括从顶点着色器传递过来的数据，输出通常是像素的颜色值。

```GLSL
varying vec2 textureOut;
uniform sampler2D tex_y; // Y 纹理
uniform sampler2D tex_u; // U 纹理
uniform sampler2D tex_v; // V 纹理
void main(void) {
    vec3 yuv;
    vec3 rgb;
    // 读取纹理数据
    yuv.x = texture2D(tex_y, textureOut).r;
    yuv.y = texture2D(tex_u, textureOut).r - 0.5;
    yuv.z = texture2D(tex_v, textureOut).r - 0.5;

    // BT.601 转换公式 (标准视频)
    rgb = mat3( 1,       1,         1,
                0,       -0.39465,  2.03211,
                1.13983, -0.58060,  0) * yuv;

    gl_FragColor = vec4(rgb, 1);
}
```

`texture2D`函数用于从纹理中采样颜色值使用插值算法

`sampler2D`类型用于表示2D纹理采样器.（x-y平面）
