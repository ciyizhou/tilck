# version: v5
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

ENV ARCH=aarch64
ENV BOARD=qemu-virt


# Install the required system packages
RUN apt update && apt upgrade -y && \
apt install -y git-core gcc g++ gcc-multilib g++-multilib clang cmake \
python3-minimal python3-serial python3-pip python3-coverage gawk unzip \
parted wget curl qemu-system-aarch64 qemu-system-riscv64 bison flex libssl-dev device-tree-compiler

RUN apt install -y vim htop net-tools curl inetutils-ping file fdisk sudo


ARG USER_ID
ARG GROUP_ID

RUN addgroup --gid $GROUP_ID user
RUN adduser --disabled-password --gecos '' --uid $USER_ID --gid $GROUP_ID user
RUN usermod -aG sudo user
RUN echo 'user ALL=(ALL) NOPASSWD:ALL' | sudo tee /etc/sudoers.d/user-nopasswd
RUN chmod 0440 /etc/sudoers.d/user-nopasswd
USER user
