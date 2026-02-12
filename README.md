# Distributed Framework

A C++-based distributed framework built with **CMake**, supporting REST APIs, clients, DAG-based execution, protobuf-based communication, and coordination via **etcd** and **Redpanda**.

---

## Index

- [1. Setup](#1-setup)
  - [1.1 Single Node Framework](#11-single-node-framework)
    - [1.1.1 Requirements](#111-requirements)
    - [1.1.2 Install & VM Setup](#112-install--vm-setup)
    - [1.1.3 Execution Modes](#113-execution-modes)
  - [1.2 Distributed Node Framework](#12-distributed-node-framework)
    - [1.2.1 Project Structure](#121-project-structure)
    - [1.2.2 Requirements](#122-requirements-1)
    - [1.2.3 Installing Multipass (Ubuntu)](#123-installing-multipass-ubuntu)
    - [1.2.4 Creating Multipass VMs](#124-creating-multipass-vms)
    - [1.2.5 Copying Project Files to VMs](#125-copying-project-files-to-vms)
    - [1.2.6 Installing System Dependencies](#126-installing-system-dependencies)
    - [1.2.7 Installing rpk (Redpanda CLI)](#127-installing-rpk-redpanda-cli)
    - [1.2.8 Installing etcd-cpp-apiv3](#128-installing-etcd-cpp-apiv3)
    - [1.2.9 Building the Project](#129-building-the-project)
- [2. Experiment Running](#2-experiment-running)
  - [2.1 Single Node Framework](#21-single-node-framework)
    - [2.1.1 Serial Execution](#211-serial-execution)
    - [2.1.2 Parallel Execution](#212-parallel-execution)
  - [2.2 Distributed Node Framework](#22-distributed-node-framework)
    - [2.2.1 Running Tests](#221-running-tests)
    - [2.2.2 Running Core Components](#222-running-core-components)
    - [2.2.3 Monitoring Transactions](#223-monitoring-transactions)
    - [2.2.4 Cleaning Up](#224-cleaning-up)

---

# 1. Setup

The framework supports two deployment configurations:

1. **Single Node Framework**
2. **Distributed Node Framework**

---

## 1.1 Single Node Framework

This configuration runs the framework on a single machine using one VM (`nodeS`).

---

### 1.1.1 Requirements

Ensure the following are installed:

- CMake >= 3.20
- GCC >= 11.4.0
- Google Protocol Buffers
- Boost
- gRPC
- RocksDB
- etcd client libraries (if required)
- Multipass

---

### 1.1.2 Install & VM Setup

#### Install Multipass (Ubuntu)

```bash
sudo snap install multipass
```

Verify installation:

```bash
multipass version
```

#### Create Single Node VM

```bash
multipass launch -n nodeS --cpus 8 --mem 10G --disk 20G 24.04
```

Enter the VM:

```bash
multipass shell nodeS
```

#### Install Dependencies Inside VM

```bash
sudo apt-get update && \
sudo apt-get install -y cmake unzip python3-pip protobuf-compiler libprotobuf-dev \
  libboost-all-dev libssl-dev libcpprest-dev libboost-system-dev \
  libcurl4-openssl-dev librdkafka-dev libasio-dev libgflags-dev \
  nlohmann-json3-dev libgtest-dev librocksdb-dev \
  libgrpc-dev libgrpc++-dev protobuf-compiler-grpc etcd-client libtbb-dev
```

---

### 1.1.3 Execution Modes

After completing the above steps, your Single Node environment is ready.

Copy the required module folder depending on the execution mode:

- **Serial Execution** → transfer to `nodeS:singleNode-serial`
- **Parallel Execution** → transfer to `nodeS:singleNode-parallel`

Example:

```bash
multipass transfer -r ./ nodeS:singleNode-serial
```

---

## 1.2 Distributed Node Framework

This configuration runs the framework across multiple VMs (`node0`, `node1`, `node2`) using **etcd** and **Redpanda**.

---

### 1.2.1 Project Structure

```
Distributed_Framework/
├── build/
├── blocksDB
├── block_producer
├── DAG-module
├── merkelTree
├── Crow/
├── googletest/
├── protos/
├── client/
├── RestAPI/
├── auto_deploy.sh
└── CMakeLists.txt
```

---

### 1.2.2 Requirements

- CMake >= 3.20
- GCC >= 11.4.0
- Google Protocol Buffers
- Multipass

---

### 1.2.3 Installing Multipass (Ubuntu)

```bash
sudo snap install multipass
```

Verify installation:

```bash
multipass version
```

---

### 1.2.4 Creating Multipass VMs

```bash
chmod +x autoDeploy.sh
./autoDeploy.sh
```

---

### 1.2.5 Copying Project Files to VMs

```bash
multipass transfer -r ./ node0:Distributed_Framework
multipass transfer -r ./ node1:Distributed_Framework
multipass transfer -r ./ node2:Distributed_Framework
```

---

### 1.2.6 Installing System Dependencies

Enter each VM:

```bash
multipass shell node0
multipass shell node1
multipass shell node2
```

Install dependencies inside each VM:

```bash
sudo apt-get update && \
sudo apt-get install -y cmake unzip python3-pip protobuf-compiler libprotobuf-dev \
  libboost-all-dev libssl-dev libcpprest-dev libboost-system-dev \
  libcurl4-openssl-dev librdkafka-dev libasio-dev libgflags-dev \
  nlohmann-json3-dev libgtest-dev librocksdb-dev \
  libgrpc-dev libgrpc++-dev protobuf-compiler-grpc etcd-client libtbb-dev
```

---

### 1.2.7 Installing rpk (Redpanda CLI)

```bash
curl -LO https://github.com/redpanda-data/redpanda/releases/latest/download/rpk-linux-amd64.zip && \
mkdir -p ~/.local/bin && \
export PATH="$HOME/.local/bin:$PATH" && \
unzip rpk-linux-amd64.zip -d ~/.local/bin/
```

---

### 1.2.8 Installing etcd-cpp-apiv3

```bash
git clone --recurse-submodules https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git
cd etcd-cpp-apiv3
mkdir build && cd build
cmake ..
sudo make -j4
sudo make install
```

---

### 1.2.9 Building the Project

Inside each VM:

```bash
cd Distributed_Framework
mkdir -p build
cd build
cmake ..
make
```

---

# 2. Experiment Running

---

## 2.1 Single Node Framework

### 2.1.1 Serial Execution

```bash
./client
```

Used for correctness validation and baseline comparison.

---

### 2.1.2 Parallel Execution

```bash
./client
```

Used for evaluating multi-core performance.

---

## 2.2 Distributed Node Framework

### 2.2.1 Running Tests

```bash
ctest
```

---

### 2.2.2 Running Core Components

```bash
./RestAPI
./client
```

---

### 2.2.3 Monitoring Transactions

```bash
rpk topic consume transaction_pool --brokers localhost:19092
rpk cluster info -X brokers=localhost:19092
```

---

### 2.2.4 Cleaning Up

```bash
multipass stop node0 node1 node2
multipass delete node0 node1 node2
multipass purge
```

---

## Notes

- All distributed nodes must use identical builds.
- Ensure required ports for etcd and Redpanda are open.
- Follow consistent startup order for reproducible results.

