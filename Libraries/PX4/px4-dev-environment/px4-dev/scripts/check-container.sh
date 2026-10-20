#!/usr/bin/env bash
set -euo pipefail
echo 'User / OS'; id; . /etc/os-release; echo "$PRETTY_NAME"
echo 'GPU'; nvidia-smi -L
echo 'OpenGL'; glxinfo -B
echo 'ARM toolchain'; arm-none-eabi-gcc --version | head -n 1
echo 'ROS 2'; set +u; source /opt/ros/jazzy/setup.bash; set -u; ros2 pkg prefix rclcpp; ros2 pkg prefix ros_gz_bridge
echo 'Gazebo'; gz sim --versions
echo 'CC-Switch CLI'; cc-switch --version
echo 'Proxy'; curl --fail --silent --show-error --output /dev/null \
  --write-out 'HTTP %{http_code}\n' --connect-timeout 10 --max-time 30 https://github.com
echo 'Shared file'; date -Is > "/shared/container-check-${ROS_DOMAIN_ID}.txt"
echo "Source: $PX4_SOURCE_DIR"
echo 'Checks finished. Confirm the OpenGL renderer above is NVIDIA.'
