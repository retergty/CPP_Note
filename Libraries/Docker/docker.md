# docker

`docker`是一个开源的容器化平台，允许开发者打包应用及其依赖项到一个轻量级、可移植的容器中，从而实现跨环境的一致运行。类似于虚拟机，但更高效，Docker容器共享主机操作系统的内核，启动速度快，占用资源少。

## 概念

### 镜像Image

* **定义**：镜像是一个只读的模板，包含运行某个应用所需的所有文件、环境(Python/GCC),系统库(glibc, OpenCV)和配置文件等。可以将镜像看作是容器的蓝图。镜像可以从Docker Hub等公共仓库下载，也可以自定义构建。
* **特性**：它是静态的，存储在硬盘上.


### 容器Container

* **定义**：容器是镜像的一个运行实例，是一个独立的、隔离的环境。每个容器都有自己的文件系统、网络接口和进程空间。容器可以启动、停止、删除，且不会影响其他容器或主机系统。
* **特性**：它是动态的，运行在内存中.容器之间相互隔离，但可以通过网络进行通信。

### 仓库Repository

* **定义**：仓库是存储和分发Docker镜像的地方。可以是公共的（如Docker Hub）或私有的。开发者可以将自己构建的镜像推送到仓库，供他人下载使用。可以直接`pull`镜像到本地使用。

### .dockerignore文件

* **定义**：`.dockerignore`文件用于指定在构建Docker镜像时应忽略的文件和目录。类似于`.gitignore`文件，可以防止不必要的文件被包含在镜像中，从而减小镜像体积并提高构建效率。
* **用法**：在项目根目录下创建一个名为`.dockerignore`的文件，列出要忽略的文件和目录，每行一个。

## 常见问题

### 传递主机代理

```shell
# 把HTTP_PROXY环境变量传给 Docker
docker build \
    --build-arg http_proxy=http://127.0.0.1:7890 \
    --build-arg https_proxy=http://127.0.0.1:7890 \
    -t my-robot .
```

### DockerFile中使用国内源

```Dockerfile
# 使用阿里云的Ubuntu源
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirrors.aliyun.com/ubuntu/|g' /etc/apt/sources.list && \
    apt-get update
```
