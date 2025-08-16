FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install basic tools
RUN apt-get update && apt-get install -y \
    software-properties-common \
    curl \
    wget \
    lsb-release \
    ca-certificates \
    gnupg2 \
    python3-pip

# Install GCC 13 and G++ 13
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Install development tools and dependencies
RUN apt-get install -y \
    git \
    libtool \
    pkg-config \
    gperf \
    libcap-dev \
    ninja-build \
    cmake \
    python3-setuptools \
    build-essential \
    unzip

# Install Boost 1.81 from source
RUN mkdir -p /tmp/boost && \
    cd /tmp/boost && \
    curl -L -o boost_1_81_0.tar.gz https://archives.boost.io/release/1.81.0/source/boost_1_81_0.tar.gz && \
    tar xfz boost_1_81_0.tar.gz && \
    cd boost_1_81_0 && \
    ./bootstrap.sh && \
    ./b2 install && \
    cd / && rm -rf /tmp/boost

# Upgrade pip and install Python packages
RUN pip3 install --upgrade pip
RUN pip3 install meson jinja2 jsonschema mako inflection PyYAML

# Install systemd build dependencies
RUN apt-get update && apt-get install -y \
    libmount-dev \
    libbpf-dev \
    libblkid-dev
# Install systemd headers (v253+) from source
RUN mkdir -p /tmp/systemd && \
    cd /tmp/systemd && \
    git clone --depth 1 --branch v253 https://github.com/systemd/systemd.git && \
    cd systemd && \
    meson setup build && \
    ninja -C build && \
    ninja -C build install && \
    ldconfig && \
    cd / && rm -rf /tmp/systemd

# Clone, build, and install sdbusplus, then clean up
RUN git clone https://github.com/openbmc/sdbusplus.git /opt/sdbusplus && \
    cd /opt/sdbusplus && \
    meson setup build && \
    ninja -C build && \
    ninja -C build install && \
    cd / && rm -rf /opt/sdbusplus

# Default to bash
CMD ["/bin/bash"]