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

### docker网桥

`docker`默认使用网桥网络模式（bridge），为每个容器分配一个虚拟网卡，并通过NAT技术实现容器与宿主机及外部网络的通信。

在宿主机上输入

```bash
$ ip addr show docker0
3: docker0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 ...
    inet 172.17.0.1/16 brd 172.17.255.255 ...
```

可以看到`docker0`网桥的IP地址是`172.17.0.1`，是所有容器的网关，容器可以通过这个地址访问宿主机的网络资源。

## 常见命令

### 镜像操作

#### 拉取镜像

```shell
docker pull <镜像名>:<标签>
# 例子
docker pull ubuntu:20.04
```

标签（tag）用于指定镜像的版本，如果不指定，默认拉取`latest`标签的镜像。

#### 查看本地镜像

```shell
docker images
```

#### 删除镜像

```shell
docker rmi <镜像ID或名称>
# 例子
docker rmi ubuntu:20.04
```

#### 构建镜像

```shell
docker build -t <镜像名>:<标签> <Dockerfile所在目录>
# 例子
docker build -t my-ubuntu:1.0 .
```

按照`docker build`命令所在目录下的`Dockerfile`文件构建镜像。

#### 创建自己的镜像

```shell
docker commit <容器ID或名称> <新镜像名>:<标签>
# 例子
docker commit my_container my_custom_image:1.0
```

将一个正在运行的容器保存为一个新的镜像，可以用来备份或分享当前容器的状态。

不会保存`-v`挂载的卷数据，因为卷是独立于容器文件系统的。

### 容器操作

#### 运行容器

```shell
docker run -it --name <容器名> <镜像名>:<标签>
# 例子
docker run -it -d --name my_bot -p 8080:80 -v /host/data:/container/data ubuntu:20.04
```

* `-it`：交互式终端
* `-d`：后台运行
* `--name`：指定容器名称
* `-p`：端口映射，格式为`宿主机端口:容器端口`
* `-v`：数据卷挂载，格式为`宿主机路径:容器路径`

```shell
docker run [command] [args]
```

运行一个容器并执行指定的命令和参数。

注意，这个命令会在容器的`PID 1`进程运行，当`PID 1`进程退出时，容器也会停止运行。

#### 日常控制

* `docker ps`：查看正在运行的容器
* `docker ps -a`：查看所有容器（包括未运行的）
* `docker stop <容器ID或名称>`：停止容器
* `docker start <容器ID或名称>`：启动容器
* `docker restart <容器ID或名称>`：重启容器
* `docker rm <容器ID或名称>`：删除容器

#### 调试与交互

* `docker exec -it <容器ID或名称> /bin/bash`：进入正在运行的容器的交互式终端
* `docker logs <容器ID或名称>`：查看容器日志
* `docker cp [主机文件] [容器]:[路径]`：从容器复制文件到宿主机
* `docker cp [容器]:[路径] [主机文件]`：从宿主机复制文件到容器
* `docker inspect <容器ID或名称>`：查看容器的详细信息（如网络配置、挂载点等）

#### 监控与清理

* `docker stats`：实时查看容器的资源使用情况（CPU、内存、网络等）
* `docker top <容器ID或名称>`：查看容器内运行的进程
* `docker system df`：查看Docker使用的磁盘空间
* `docker system prune`：清理未使用的容器、网络、镜像等，释放磁盘空间

## docker-compose.yaml

`docker-compose`是一个用于定义和管理多容器Docker应用的工具。通过一个名为`docker-compose.yaml`的配置文件，可以轻松地配置应用的服务、网络和卷等。

### 示例

```yaml
# 1. 版本号 (必须是字符串)
version: '3.8'

# 2. 服务定义 (核心)
services:
  
  # --- 服务 A: 基于已有镜像 ---
  my-web-app:               # 服务名 (在网络中作为域名使用)
    image: nginx:alpine     # 使用的镜像
    container_name: web01   # 容器名 (可选，不写系统会自动生成)
    restart: always         # 重启策略: 挂了自动重启
    ports:                  # 端口映射 (-p)
      - "8080:80"           # 宿主机端口:容器端口
    volumes:                # 挂载 (-v)
      - ./html:/usr/share/nginx/html  # 宿主机路径:容器路径
    networks:               # 加入哪个网络
      - my-net
    environment:            # 环境变量 (-e)
      - APP_ENV=production

  # --- 服务 B: 基于 Dockerfile 构建 ---
  my-backend:
    build: .                # 在当前目录寻找 Dockerfile 构建镜像
    # 或者详细写法:
    # build:
    #   context: ./api
    #   dockerfile: Dockerfile.dev
    command: python app.py  # 覆盖镜像默认的启动命令
    depends_on:             # 启动顺序
      - my-web-app          # 等 web 启动了再启动我
    networks:
      - my-net

# 3. 网络定义 (可选，不写默认会有 default 网络)
networks:
  my-net:
    driver: bridge          # 模式: bridge, host, macvlan

# 4. 数据卷定义 (可选，用于具名卷)
volumes:
  db-data:                  # 定义一个叫 db-data 的卷，由 Docker 管理
```

## dockerfile

`Dockerfile`是一个文本文件，包含了一系列指令，用于定义如何构建一个Docker镜像。通过编写`Dockerfile`，可以自动化地创建自定义的镜像，确保环境的一致性和可重复性。

### 常见指令

* `FROM <基础镜像>`：指定构建镜像所基于的基础镜像。
* `WORKDIR <目录>`：设置工作目录，后续的命令都将在该目录下执行。
* `COPY <源路径> <目标路径>`：将文件或目录从主机复制到镜像中的指定路径。
* `RUN <命令>`：在镜像中执行命令，通常用于安装软件包或配置环境。
* `CMD ["可执行文件", "参数1", "参数2"]`：指定容器启动时(PID 1)执行的命令和参数。每个镜像只能有一个`CMD`指令。
* `ENTRYPOINT ["可执行文件", "参数1", "参数2"]`：配置容器启动时执行的命令，通常与`CMD`结合使用，`ENTRYPOINT`指定主命令，`CMD`提供默认参数。
* `ENV <变量名> <值>`：设置环境变量。
* `EXPOSE <端口号>`：声明容器监听的端口，但不会自动映射到宿主机端口。
* `VOLUME ["/数据卷路径"]`：声明数据卷，用于持久化数据或共享数据。
* `LABEL <键>=<值>`：为镜像添加元数据标签。

### 示例

```Dockerfile
# 使用 ROS 官方镜像
FROM ros:noetic-robot

# 设置 Shell，否则 source 命令无效 (ROS 特有坑)
SHELL ["/bin/bash", "-c"]

# 安装额外的 Linux 工具和 ROS 包
RUN apt-get update && apt-get install -y \
    vim \
    git \
    ros-noetic-navigation \
    && rm -rf /var/lib/apt/lists/* # 清理缓存减小体积

# 创建工作空间
WORKDIR /root/catkin_ws

# 拷贝你的代码
COPY ./src ./src

# 编译代码 (注意要 source 环境)
RUN source /opt/ros/noetic/setup.bash && \
    catkin_make

# 设置入口点 (自动 source 环境脚本)
COPY ./ros_entrypoint.sh /
ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]
```

## 常见问题

### 配置docker守护进程全局代理

#### 对于版本号低于23.03的docker

让`docker pull`等命令，所有的容器都默认走代理

1. 修改配置目录

    ```shell
    sudo mkdir -p /etc/systemd/system/docker.service.d
    ```

2. 创建代理文件

    ```shell
    sudo nano /etc/systemd/system/docker.service.d/http-proxy.conf
    ```

3. 填入内容

    ```ini
    [Service]
    Environment="HTTP_PROXY=http://127.0.0.1:7890"
    Environment="HTTPS_PROXY=http://127.0.0.1:7890"
    #这行很重要，设置不走代理的地址，防止连接本地仓库出问题
    Environment="NO_PROXY=localhost,127.0.0.1,::1,.local"
    ```

4. 重新加载配置并重启docker

    ```shell
    sudo systemctl daemon-reload
    sudo systemctl restart docker
    ```

#### 对于版本号高于23.03的docker

1. 修改配置目录

    ```shell
    sudo mkdir -p /etc/docker
    ```

2. 创建代理文件

    ```shell
    sudo vim /etc/docker/daemon.json
    ```

3. 填入内容

    ```json
    {
      "proxies": {
        "http-proxy": "http://127.0.0.1:7890",
        "https-proxy": "http://127.0.0.1:7890",
        "no-proxy": "localhost,127.0.0.1,::1,.local"
      }
    }
    ```

4. 重新加载配置并重启docker

    ```shell
    sudo systemctl daemon-reload
    sudo systemctl restart docker
    ```

### 将当前用户加入docker

```shell
sudo usermod -aG docker $USER
# 重新登录生效
```

### 配置所有容器内默认代理

```shell
mkdir -p ~/.docker
vim ~/.docker/config.json
```

填入

```json
{
 "proxies": {
   "default": {
     "httpProxy": "http://172.17.0.1:7890",
     "httpsProxy": "http://172.17.0.1:7890",
     "noProxy": "localhost,127.0.0.1,::1,.local,192.168.0.0/16,10.0.0.0/8"
   }
 }
}
```

其中`127.17.0.1`是宿主机在docker网桥中的地址，可以通过`ip addr show docker0`查看。

### 传递主机代理

```shell
# 把HTTP_PROXY环境变量传给 Docker
docker build \
    --build-arg http_proxy=http://127.0.0.1:7890 \
    --build-arg https_proxy=http://127.0.0.1:7890 \
    -t my-robot .
```

### 运行apt时无法找到包

```shell
apt-get update
```

首先需要运行`apt-get update`更新包列表，否则会提示找不到包。

### DockerFile中使用国内源

```Dockerfile
# 换 Ubuntu 系统源 (阿里源)
sed -i 's@http://.*archive.ubuntu.com@http://mirrors.aliyun.com@g' /etc/apt/sources.list && \
sed -i 's@http://.*security.ubuntu.com@http://mirrors.aliyun.com@g' /etc/apt/sources.list
apt-get update
```

注意，不会影响宿主机，任何操作均是隔离的。

### 更改默认的docker存储位置

默认情况下，Docker将镜像、容器和数据存储在`/var/lib/docker`目录下。如果需要更改这个位置，可以按照以下步骤操作：

1. 停止Docker服务：

    ```shell
    sudo systemctl stop docker
    ```

2. 创建新的存储目录：

    ```shell
    sudo mkdir -p /home/docker-data
    ```

3. 修改Docker配置文件：

    ```shell
    sudo vim /etc/docker/daemon.json
    ```

4. 在配置文件中添加以下内容：

    ```json
    {
      "data-root": "/home/docker-data"
    }
    ```

5. 将现有数据迁移到新目录：

    ```shell
    sudo rsync -aP /var/lib/docker/ /home/docker-data/
    ```

6. 重新加载Docker配置并启动服务：

    ```shell
    sudo systemctl daemon-reload
    sudo systemctl start docker
    ```

### 连接图形界面

使用docker连接图形界面，比如ROS2,Gazebo，同时调用GPU进行渲染，可以通过以下方式：

```shell
# 以X11为例，运行容器时添加以下参数
docker run -it \
    -e DISPLAY=$DISPLAY \  # 传递显示环境变量
    -v /tmp/.X11-unix:/tmp/.X11-unix \  # 挂载X11套接字
    --name my_ros_sim \
    --gpus all \  # 允许访问所有GPU
    --net=host \ # 使用主机网络模式，简化网络配置
    -e QT_X11_NO_MITSHM=1 \ # 解决Qt应用在Docker中的显示问题
    -e NVIDIA_DRIVER_CAPABILITIES=all \ # 允许访问所有GPU功能
    -e NVIDIA_VISIBLE_DEVICES=all \ # 允许访问所有GPU设备
    my_ros_image:latest
```

此外，还需要在宿主机上安装NVIDIA Container Toolkit，并确保正确配置了GPU驱动和Docker的GPU支持。

配置软件源仓库

```shell
# 1. 导入 GPG 密钥
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg

# 2. 添加软件源
curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
  sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
  sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
```

安装工具包

```shell
sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
```

配置 Docker 运行时

```shell
# 自动修改 /etc/docker/daemon.json 增加 NVIDIA 配置
sudo nvidia-ctk runtime configure --runtime=docker

# 重启 Docker 使配置生效
sudo systemctl restart docker
```

验证

```shell
sudo docker run --rm --gpus all nvidia/cuda:12.0.1-base-ubuntu22.04 nvidia-smi
```

如果输出显示了 NVIDIA GPU 的信息，说明配置成功，可以在 Docker 容器中使用 GPU 进行计算和渲染了。
