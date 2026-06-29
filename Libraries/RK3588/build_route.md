# RK3588 docker SDK 开发教程

## docker 环境搭建

拉取 Ubuntu 20.04 基础镜像，作为与 SDK 官方要求一致的编译环境。

```shell
docker pull ubuntu:20.04
```

创建并后台启动开发容器，将宿主机工作目录挂载到容器内 `/workspace` 以便持久化 SDK 与编译产物。

```shell
docker run -it -d --name my_rk3588 -p 8080:80 -v /host/data/docker_workspace:/workspace ubuntu:20.04
```

进入已在运行的容器，获得交互式 shell 以执行后续安装与编译命令。

```shell
docker exec -it my_rk3588 /bin/bash
```

安装 SDK 构建所需的依赖包，包括 git、交叉编译辅助工具、Python 及内核编译链相关组件；其中 `bsdmainutils` 提供 `hexdump`（打包固件时识别芯片型号），`vim-common` 提供 `xxd`（部分脚本解析二进制）。

```shell
sudo apt-get update && sudo apt-get install git ssh make gcc libssl-dev \
liblz4-tool expect expect-dev g++ patchelf chrpath gawk texinfo chrpath \
diffstat binfmt-support qemu-user-static live-build bison flex fakeroot \
cmake gcc-multilib g++-multilib unzip device-tree-compiler ncurses-dev \
bzip2 expat gpgv2 cpp-aarch64-linux-gnu libgmp-dev \
libmpc-dev bc python-is-python3 python2 bsdmainutils vim-common
```

配置 Git 全局用户名与邮箱，供 `repo sync` 及后续提交记录使用。

```shell
git config --global user.name "your name"
git config --global user.email "your email"
```

将 SDK 压缩包解压到 `/workspace/rk3588_linux_sdk`，并用 `repo sync` 拉取 manifest 所管理的全部子仓库源码。

```shell
mkdir /workspace/rk3588_linux_sdk
tar xvf atk-rk3588_linux_release_v1.0_20240601.tgz -C /workspace/rk3588_linux_sdk
cd /workspace/rk3588_linux_sdk/
.repo/repo/repo sync -l -j10
```

安装 `sudo` 并配置免密规则，使 SDK 构建脚本中的 `sudo` 调用在容器内可直接执行。

```shell
apt-get update && apt-get install -y sudo \
  && echo 'ALL ALL=(ALL) NOPASSWD: ALL' > /etc/sudoers.d/nopasswd \
  && chmod 440 /etc/sudoers.d/nopasswd
```

创建 UID/GID 均为 1000 的 `builder` 用户，并将 SDK 目录所有权移交该用户，满足脚本以非 root 编译的要求。

```shell
# 创建 uid/gid 均为 1000 的用户（名字可自定）
groupadd -g 1000 builder 2>/dev/null || true
useradd -m -u 1000 -g 1000 -s /bin/bash builder 2>/dev/null || true

# 把 SDK 目录所有权交给 builder（路径按你的实际改）
chown -R builder:builder /workspace/rk3588_linux_sdk
```

先加载正点原子 RK3588 板级 defconfig 选定目标产品配置，再执行全量编译生成内核、rootfs 及烧录所需固件。

```shell
su - builder
cd /workspace/rk3588_linux_sdk
./build.sh alientek_rk3588_defconfig
./build.sh all
```

## 预置 Buildroot 下载缓存（推荐）

SDK 全量编译中最耗时的环节是**根文件系统（rootfs）**的构建：Buildroot 会联网下载大量第三方开源包，既显著拉长编译时间，也可能因网络不稳或上游地址变更导致拉包失败，进而使整个 rootfs 编译中断。将厂商提供的依赖包压缩包解压到 SDK 的 `buildroot/` 目录，可跳过重复下载，加快构建并降低失败概率。

```shell
tar -xzf dl.tgz -C /workspace/rk3588_linux_sdk/buildroot/
```

解压后源码包会进入 `buildroot/dl/`，Buildroot 编译时会优先使用本地缓存，而不再从网络拉取。
