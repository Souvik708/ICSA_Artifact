FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Dependecies
RUN apt update && apt install -y \
    cmake \
    gcc \
    g++ \
    unzip \
    python3-pip \
    protobuf-compiler \
    libprotobuf-dev \
    libboost-all-dev \
    libssl-dev \
    libcpprest-dev \
    libboost-system-dev \
    libcurl4-openssl-dev \
    librdkafka-dev \
    libasio-dev \
    libgflags-dev \
    nlohmann-json3-dev \
    libgtest-dev \
    librocksdb-dev \
    libgrpc-dev \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    curl \ 
    git && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /Distributed-Framework

# Copying Files 
COPY . .

RUN git submodule update --init --recursive

# Install etcd-cpp-apiv3
RUN if [ ! -d "etcd-cpp-apiv3" ]; then \
    git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git; \
    fi

RUN sed -i '1i #include <cstdint>' etcd-cpp-apiv3/etcd/v3/Member.hpp && \
    sed -i '1i #include <cstdint>' etcd-cpp-apiv3/src/v3/Member.cpp && \
    cd etcd-cpp-apiv3 && mkdir build && cd build && \
    cmake .. && make -j$(nproc) && make install

# Install Redpanda CLI (rpk)
RUN if [ ! -f "rpk-linux-amd64.zip" ]; then \
    curl -LO https://github.com/redpanda-data/redpanda/releases/latest/download/rpk-linux-amd64.zip && \
    mkdir -p /usr/local/bin && \
    unzip rpk-linux-amd64.zip -d /usr/local/bin/; \
    fi

RUN chmod +x /usr/local/bin/rpk && \
    rm rpk-linux-amd64.zip

# Build the project
RUN rm -rf build && mkdir -p /Distributed-Framework/build && cd /Distributed-Framework/build && \
    cmake .. && \
    make -j$(nproc)





