# Distributed Framework

This project uses CMake as the build system. Follow the steps below to configure, build, and run the project.

---

## Project Structure

```
Distributed_Framework/
├── build/               # Generated build files
│   ├── client           # Compiled client executable
│   ├── RestAPI          # Compiled REST API executable
│   └── other build files...
├── git-hooks/pre-commit # Clang-format checking
├── blocksDB
├── block_producer
├── DAG-module
├── merkelTree
├── Crow/
├── googletest/
├── protos/
│   ├── protoUtils/
│   ├── block.proto
│   ├── component.proto
│   ├── transaction.proto
│   └── matrix.proto
├── client/              # Contains client.cpp
├── RestAPI/             # Contains RestAPI.cpp
├── auto_deploy.sh       # Auto deployment script     
└── CMakeLists.txt
```

---

## Requirements

- CMake version 3.20 or higher
- GCC 11.4.0 compiler or higher
- Google Protocol Buffers

---

## Creating Multipass VMs

```bash
chmod +x autoDeploy.sh
./autoDeploy.sh
```

---

## Copying Files to the VMs

Execute this from your host machine (outside of VMs):

```bash
multipass transfer -r ./ nodeS:blockchain_singleNode
```
---

## Install Additional Packages (If Not Installed)

Execute this inside each VM:

```bash
multipass shell nodeS
```

Then run:

```bash
sudo apt-get update && \
sudo apt-get install -y cmake unzip python3-pip protobuf-compiler libprotobuf-dev \
  libboost-all-dev libssl-dev libcpprest-dev libboost-system-dev \
  libcurl4-openssl-dev librdkafka-dev libasio-dev libgflags-dev \
  nlohmann-json3-dev libgtest-dev librocksdb-dev \
  libgrpc-dev libgrpc++-dev protobuf-compiler-grpc etcd-client libtbb-dev
```
---
## Build the Project

Run CMake to configure and build the project:

```bash
cd
cd Distributed_Framework
mkdir build
cd build
cmake ..
make
```

---

## Running the Executables

```bash
ctest
```

### Test the RestAPI Module

```bash
./RestAPI   # Run the RestAPI
./client    # Run the client
```


## Deleting Multipass VMs

```bash
multipass stop nodeS
multipass delete nodeS
multipass purge
```
