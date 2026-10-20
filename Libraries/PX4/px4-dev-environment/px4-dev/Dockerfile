FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
ARG DEV_UID=1000
ARG DEV_GID=1000
# Each Compose configuration selects its own local installer inputs.
ARG PX4_SOURCE_REL
ARG DDS_AGENT_REF=73622810d984349b80bbac0ef55fc0b694d62222
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Docker's predefined HTTP(S)_PROXY build args are supplied by Compose.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl git gnupg sudo locales software-properties-common \
    python3-venv mesa-utils xauth x11-apps iproute2 procps less nano \
    bash-completion openssh-client libglvnd0 libgl1 libegl1 libxkbcommon-x11-0 \
    libxcb-cursor0 libxcb-xinerama0 && \
    locale-gen en_US.UTF-8 && add-apt-repository -y universe
ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

# Install the terminal-only CLI fork; provider credentials are entered after startup.
ARG CC_SWITCH_CLI_VERSION=5.11.2
ARG CC_SWITCH_CLI_SHA256=bb0255e37894c47f0bea721605c2f02ac7a69d699011bb1636db751e175d2baa
RUN test "$(dpkg --print-architecture)" = amd64 && \
    curl -fL --retry 3 --connect-timeout 20 --max-time 600 \
      "https://github.com/SaladDay/cc-switch-cli/releases/download/v${CC_SWITCH_CLI_VERSION}/cc-switch-cli-linux-x64-musl.tar.gz" \
      -o /tmp/cc-switch-cli.tar.gz && \
    printf '%s  /tmp/cc-switch-cli.tar.gz\n' "${CC_SWITCH_CLI_SHA256}" | sha256sum -c - && \
    mkdir /tmp/cc-switch-cli && \
    tar -xzf /tmp/cc-switch-cli.tar.gz -C /tmp/cc-switch-cli cc-switch && \
    install -m 0755 /tmp/cc-switch-cli/cc-switch /usr/local/bin/cc-switch && \
    cc-switch --version | tee /opt/cc-switch-cli-version.txt && \
    grep -Fx "cc-switch ${CC_SWITCH_CLI_VERSION}" /opt/cc-switch-cli-version.txt && \
    rm -rf /tmp/cc-switch-cli /tmp/cc-switch-cli.tar.gz

RUN if id ubuntu &>/dev/null; then userdel -r ubuntu; fi && \
    if ! getent group "${DEV_GID}" >/dev/null; then groupadd -g "${DEV_GID}" dev; fi && \
    useradd -m -s /bin/bash -u "${DEV_UID}" -g "${DEV_GID}" dev && \
    printf 'Defaults env_keep += "HTTP_PROXY HTTPS_PROXY NO_PROXY http_proxy https_proxy no_proxy"\n' \
      > /etc/sudoers.d/proxy-env && chmod 0440 /etc/sudoers.d/proxy-env

# Only installer inputs enter the image; use existing local source and edits.
COPY ${PX4_SOURCE_REL}/Tools/setup/ubuntu.sh /opt/px4-setup/ubuntu.sh
COPY ${PX4_SOURCE_REL}/Tools/setup/requirements.txt /opt/px4-setup/requirements.txt
RUN sha256sum /opt/px4-setup/ubuntu.sh /opt/px4-setup/requirements.txt > /opt/px4-toolchain-inputs.sha256 && \
    RUNS_IN_DOCKER=true USER=dev bash /opt/px4-setup/ubuntu.sh && \
    rm -rf /opt/px4-setup

# ROS apt metadata/packages are authenticated with the ROS signing key.
RUN curl -fsSL --retry 3 https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
      -o /usr/share/keyrings/ros-archive-keyring.gpg && \
    echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu noble main' \
      > /etc/apt/sources.list.d/ros2.list && \
    apt-get update && apt-get install -y --no-install-recommends \
      ros-jazzy-desktop ros-jazzy-ros-gz ros-dev-tools && \
    rm -rf /var/lib/apt/lists/*

RUN git init /opt/dds-agent-src && \
    git -C /opt/dds-agent-src remote add origin https://github.com/eProsima/Micro-XRCE-DDS-Agent.git && \
    git -C /opt/dds-agent-src fetch --depth=1 origin "${DDS_AGENT_REF}" && \
    git -C /opt/dds-agent-src checkout --detach FETCH_HEAD && \
    cmake -S /opt/dds-agent-src -B /opt/dds-agent-build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /opt/dds-agent-build --parallel 4 && \
    cmake --install /opt/dds-agent-build && ldconfig && \
    rm -rf /opt/dds-agent-src /opt/dds-agent-build

RUN printf '\nsource /opt/ros/jazzy/setup.bash\nexport PATH="$PATH:/opt/xtensa-esp-elf/bin"\nif [ -f /workspaces/ros2_ws/install/setup.bash ]; then source /workspaces/ros2_ws/install/setup.bash; fi\n' \
      >> /home/dev/.bashrc
USER dev
WORKDIR /workspaces
CMD ["sleep", "infinity"]
