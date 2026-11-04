FROM ubuntu:26.04
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y \
    build-essential \
    clang-format \
    clang-tidy \
    git \
    gnu-efi \
    mtools \
    parted \
    qemu-system-x86 \
    vim
