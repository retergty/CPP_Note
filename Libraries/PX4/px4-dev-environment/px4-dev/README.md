# PX4 Docker 开发操作指南

更新：2026-10-19。一份公共 Dockerfile 和 compose.yaml，通过 env/*.env 选择环境，分别构建镜像、启动容器。

## 1. 环境与文件

| 项目 | NEW | BACKUP |
|---|---|---|
| 环境变量文件 | env/new.env | env/backup.env |
| 镜像 | px4-new:ubuntu24-jazzy | px4-backup:ubuntu24-jazzy |
| 宿主机挂载目录 | ~/PX4/PX4_NEW | ~/PX4/PX4_BACKUP |
| 容器源码目录 | /workspaces/PX4-Autopilot | /workspaces/PX4_BACKUP |
| ROS_DOMAIN_ID | 10 | 20 |

镜像包含 Ubuntu 24.04、PX4 工具链、Gazebo Harmonic、ROS 2 Jazzy、ros_gz、DDS Agent 2.4.3 和 cc-switch CLI 5.11.2。两套镜像分别读取各自源码的安装脚本。

要求：Ubuntu 24.04 amd64 宿主机、可用的 Docker Engine、Compose v2.24+、NVIDIA 驱动与 Container Toolkit（已注册 nvidia runtime）、X11/XWayland 桌面。默认代理为宿主机 127.0.0.1:7897，Docker daemon 拉镜像代理需单独配置。

已在一套 NEW 环境验证 GPU/OpenGL、工具链、ROS 包、Gazebo 版本、cc-switch、代理和共享目录；最终公共配置未完成整套重建与 SITL/ROS 联调。

| 产出 | 用途 |
|---|---|
| Dockerfile | 公共镜像构建文件 |
| compose.yaml | 公共构建、挂载、代理、GPU、X11 配置 |
| env/new.env、env/backup.env | 环境名称、挂载目录、源码子目录、ROS 域 |
| scripts/prepare.sh | 生成 .env、.dockerignore、X11 授权和持久化目录 |
| scripts/check-container.sh | 容器基础检查 |
| scripts/codex-provider.example.json | 不含真实凭据的 API 导入模板 |

## 2. 部署与初始化（宿主机）

先将压缩包保存到 ~/Downloads，并将现有两套完整源码放到下列目录；解压会覆盖同名配置，已有修改先保存。包不包含源码、镜像或 API 凭据。

~~~bash
mkdir -p ~/PX4
tar -xzf ~/Downloads/px4-dev-environment.tar.gz \
  -C ~/PX4 --strip-components=1
cd ~/PX4
sudo apt-get install -y xauth git python3
bash scripts/prepare.sh
~~~

确认 env/*.env 中的源码目录已存在后运行 prepare；不使用的环境文件可移出 env/。prepare 在桌面普通终端执行，不加 sudo；它匹配 UID/GID、准备 X11，并只在 Codex 配置缺失时创建空文件。

~~~text
~/PX4/
├── Dockerfile、compose.yaml、scripts/
├── env/new.env、env/backup.env
├── .env、state/Xauthority
├── shared/                         → 容器 /shared
├── PX4_NEW/                        → NEW 容器 /workspaces
│   ├── PX4-Autopilot/
│   ├── ros2_ws/
│   └── container-state/
│       ├── codex/                  → /home/dev/.codex
│       ├── cc-switch/              → /home/dev/.cc-switch
│       ├── cache/、gz/、vscode-server/
└── PX4_BACKUP/                     → BACKUP 容器 /workspaces
    ├── PX4_BACKUP/
    ├── ros2_ws/
    └── container-state/            # 与 NEW 分开保存
~~~

旧 compose.new.yaml、compose.backup.yaml、.devcontainer 配置和 .secrets/*.env 不再使用；解压不会删除旧文件，VS Code 按下文附加容器。项目名称与镜像名保持 px4-new / px4-backup，可继续管理原有容器。

### 修改目录

环境文件只保存差异参数，不保存 API 密钥；WORKSPACE_DIR 相对 ~/PX4，SOURCE_FOLDER 相对该工作区，修改后重新运行 prepare。

~~~dotenv
# env/new.env
ENV_NAME=new
WORKSPACE_DIR=./PX4_NEW
SOURCE_FOLDER=PX4-Autopilot
ROS_DOMAIN_ID=10
~~~

~~~dotenv
# env/backup.env
ENV_NAME=backup
WORKSPACE_DIR=./PX4_BACKUP
SOURCE_FOLDER=PX4_BACKUP
ROS_DOMAIN_ID=20
~~~

prepare 自动根据 env/*.env 生成 .dockerignore，请勿手工维护该文件；路径使用不含空格的相对路径，源码需位于 ~/PX4 内且安装脚本支持 Ubuntu 24.04。

需要改变代理时在 .env 加入 PROXY_URL=http://宿主机地址:端口；本方案使用 host 网络，面向本机 Linux Docker Engine。

## 3. 构建、启动、检查（宿主机）

首次顺序构建，下载和编译可能需要几十分钟；日常启动不会重新安装依赖。

~~~bash
cd ~/PX4
docker compose --env-file .env --env-file env/new.env --progress plain build
docker compose --env-file .env --env-file env/backup.env --progress plain build
~~~

分别启动并检查，确认 OpenGL renderer 显示 NVIDIA；报告文件写入 ~/PX4/shared。

~~~bash
docker compose --env-file .env --env-file env/new.env up -d
docker compose --env-file .env --env-file env/new.env exec px4 bash /opt/px4-dev-scripts/check-container.sh

docker compose --env-file .env --env-file env/backup.env up -d
docker compose --env-file .env --env-file env/backup.env exec px4 bash /opt/px4-dev-scripts/check-container.sh
~~~

## 4. VS Code

先启动目标环境，宿主机安装 Dev Containers 扩展，执行 Attach to Running Container，选择 px4-new-px4-1 或 px4-backup-px4-1。

进入容器后打开 /workspaces，确认终端用户为 dev，并将 C/C++、CMake Tools、Python、Codex、clangd（LLVM）扩展安装到该容器。两套环境可分别打开窗口，宿主机 VS Code 保留其代理设置。

### clangd 扩展

在容器 VS Code 安装 LLVM 发布的 clangd 扩展。找不到语言服务器时，按扩展提示在容器端自动下载 clangd；PX4 源码根目录需要先生成编译数据库。

~~~bash
cd "$PX4_SOURCE_DIR"
make matek_h743_default
ls build/matek_h743_default/compile_commands.json
~~~

在 VS Code 工作区设置中合并以下配置；切换板型或 SITL 时修改编译目录，并重启 clangd。

~~~json
{
  "clangd.arguments": [
    "--background-index",
    "--compile-commands-dir=build/matek_h743_default",
    "--query-driver=/usr/bin/arm-none-eabi-gcc,/usr/bin/arm-none-eabi-g++"
  ],
  "C_Cpp.intelliSenseEngine": "disabled"
}
~~~

## 5. cc-switch 与 API 导入

### 进入目标容器（宿主机）

按需选择其中一条，后续 API 操作均在该容器内执行。

~~~bash
# NEW
docker compose --env-file .env --env-file env/new.env exec px4 bash
# BACKUP
docker compose --env-file .env --env-file env/backup.env exec px4 bash
~~~

### 方式一：终端交互配置（容器内）

以 dev 用户运行，选择 Codex，添加并启用提供商；API 地址、密钥和模型由你填写。

~~~bash
cc-switch --app codex
~~~

### 方式二：JSON 文件导入（容器内）

复制模板并编辑，填写自己的密钥、模型和 base_url；接口格式沿用已验证配置，Website URL 是可选的官网地址。

~~~bash
mkdir -p ~/.cc-switch/imports
cp -n /opt/px4-dev-scripts/codex-provider.example.json ~/.cc-switch/imports/my-api.json
chmod 600 ~/.cc-switch/imports/my-api.json
nano ~/.cc-switch/imports/my-api.json
~~~

config 字段是 TOML 字符串，反斜杠加 n 表示换行；model_provider 的值必须对应表名，例如 my_api 对应 [model_providers.my_api]。nano 按 Ctrl+O、回车保存，Ctrl+X 退出。

导入后再切换启用；已有 my-api 时在 TUI 编辑，新增另一配置使用不同 ID。

~~~bash
cc-switch --app codex provider add \
  --name "MyGPT" \
  --id my-api \
  --config-file ~/.cc-switch/imports/my-api.json

cc-switch --app codex provider switch my-api
cc-switch --app codex provider list
~~~

单个提供商使用 provider add --config-file；config import 用于整体配置。文件和命令输出可能含密钥，不要分享或提交 Git。

### 使用与保存

切换后重载对应 VS Code 容器窗口或重启 Codex 会话，再发送请求验证；无需让 cc-switch 常驻。终端需要 codex 命令时才另装 Codex CLI。

提供商文件在宿主机的 PX4_NEW/container-state/cc-switch/imports/ 或 PX4_BACKUP/container-state/cc-switch/imports/。两套 cc-switch 和 Codex 数据分别保存，重建容器后仍保留。

## 6. PX4 编译与 Gazebo（容器内）

以 Matek H743 固件为例，其他飞控替换目标名；PX4_SOURCE_DIR 已由环境文件指向源码。

~~~bash
cd "$PX4_SOURCE_DIR"
make matek_h743_default
~~~

启动 Gazebo SITL，宿主机可运行 QGroundControl 连接；初期每次只运行一套仿真。

~~~bash
cd "$PX4_SOURCE_DIR"
make px4_sitl gz_x500
~~~

## 7. ROS 2（容器内）

以下依赖源码提供 Tools/copy_to_ros_ws.sh；只在本套空 ROS 工作区首次执行，已有工作区先审查脚本。

~~~bash
source /opt/ros/jazzy/setup.bash
bash "$PX4_SOURCE_DIR/Tools/copy_to_ros_ws.sh" /workspaces/ros2_ws
cd /workspaces/ros2_ws
colcon build --symlink-install --base-paths src
source install/setup.bash
~~~

另开同一容器终端启动 Agent，再按上一节启动 SITL。

~~~bash
MicroXRCEAgent udp4 -p 8888
~~~

在 PX4 的 pxh> 控制台确认 DDS 域；随本方案检查的两套 SITL 启动脚本会读取 ROS_DOMAIN_ID，NEW 应为 10，BACKUP 应为 20。

~~~text
param show UXRCE_DDS_DOM_ID
~~~

另开同一容器终端查看消息。

~~~bash
source /opt/ros/jazzy/setup.bash
source /workspaces/ros2_ws/install/setup.bash
ros2 topic list
ros2 topic echo /fmu/out/vehicle_status --once --qos-reliability best_effort
~~~

两套容器共用 host 网络，ROS 域不同仍可能冲突于 Agent 8888、PX4 UDP、Gazebo transport。并行仿真需另外规划端口和分区，外部 ROS 通信需调整默认 LOCALHOST 发现范围。

## 8. 日常管理（宿主机）

以下以 NEW 为例；BACKUP 将 env/new.env 换成 env/backup.env。exec 打开终端后用 exit 返回宿主机，容器继续运行。

~~~bash
cd ~/PX4
# 启动
docker compose --env-file .env --env-file env/new.env up -d
# 进入终端
docker compose --env-file .env --env-file env/new.env exec px4 bash
~~~

在另一个宿主机终端停止容器，或删除容器；挂载的源码和状态目录保留。

~~~bash
# 停止
docker compose --env-file .env --env-file env/new.env stop
# 删除容器（按需执行）
docker compose --env-file .env --env-file env/new.env down
~~~

修改 Dockerfile 或工具链依赖后重建对应环境；只改 PX4 业务源码时直接在容器内编译。

~~~bash
docker compose --env-file .env --env-file env/new.env --progress plain build
docker compose --env-file .env --env-file env/new.env up -d
~~~

查看所有容器、Docker 空间和挂载目录空间。

~~~bash
docker ps -a --size
docker system df
du -sh ~/PX4/PX4_NEW ~/PX4/PX4_BACKUP ~/PX4/shared
~~~

## 9. 常见问题

### 桌面重新登录后无法显示窗口（宿主机）

刷新 X11 授权和 DISPLAY，然后重建运行容器。

~~~bash
cd ~/PX4
bash scripts/prepare.sh
docker compose --env-file .env --env-file env/new.env up -d --force-recreate
docker compose --env-file .env --env-file env/backup.env up -d --force-recreate
~~~

### 编译引用旧电脑的绝对路径（容器内）

源码迁移后可清理构建残留再编译；make clean 也会清理子模块中被 Git 忽略的文件，有自存文件时先保存。

~~~bash
cd "$PX4_SOURCE_DIR"
make clean
make matek_h743_default
~~~

### API 报缺少 Base URL

检查 config 中的 model_provider 与 [model_providers.对应名称] 是否一致；--name 显示名和 --id 不要求与其相同。

### API 提示 Live sync skipped（容器内）

只创建缺失的配置文件，再切换；已有文件不会覆盖。

~~~bash
mkdir -p ~/.codex
if [ ! -e ~/.codex/config.toml ]; then
  touch ~/.codex/config.toml
fi
cc-switch --app codex provider switch my-api
~~~

USB 烧录与 SWD/JTAG 设备映射尚未配置；GDB 若遇到 ptrace 权限问题，再按实际调试方式配置。

## 参考

- [PX4 Ubuntu](https://docs.px4.io/main/en/dev_setup/dev_env_linux_ubuntu)
- [PX4 ROS 2](https://docs.px4.io/main/en/ros2/user_guide)
- [VS Code Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers)
- [CC-Switch CLI 5.11.2](https://github.com/SaladDay/cc-switch-cli/releases/tag/v5.11.2)
