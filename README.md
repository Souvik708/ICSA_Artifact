BlockRaFT is a crash-tolerant and scalable distributed framework designed to improve the availability and performance of blockchain nodes, particularly in permissioned settings. Instead of replicating full nodes per organization, BlockRaFT distributes the internal workload of a single blockchain node across a cluster using a RAFT-based leader–follower architecture. This design preserves fairness in consensus while enabling scalability and resilience to node crashes. Additionally, the framework introduces a concurrent Merkle tree optimization that decouples smart contract execution from state updates, significantly reducing execution overhead. Experimental results demonstrate substantial performance improvements over traditional single-core and multi-core baselines while maintaining strong fault tolerance.

To evaluate BlockRaFT, we compare our distributed framework against two single-node baselines: a serial scheduler and a parallel multi-core scheduler. The goal is to isolate the benefits of distributed workload partitioning and crash tolerance beyond local parallelism.

# BlockRaFT Distributed Framework

A C++-based distributed framework built with **CMake**, supporting REST APIs, clients, DAG-based execution, protobuf-based communication, and coordination via **etcd** and **Redpanda**.

---
**The repository is organized as follows:**
```
ICSA_ARTIFACT/
├── BlockRaFT-distributed_node --> BlockRaFT
├── singleNode-parallel        --> Parallel execution mode
├── singleNode-serial          --> Serial execution mode
└── Experiment_files           --> All necessary exp related files
```

---
# 1. Setup
The initial setup requires more than 30 minutes, primarily due to the following steps: 
1. Provisioning and configuring multiple virtual machines (VMs) to emulate a distributed cluster environment.
2. Installation and configuration of etcd for distributed coordination and RAFT-based leader election.
3. Installation of required dependencies including: Google Protocol Buffers (protobuf) for serialization.
4. Google Test (gtest) for unit testing.

The setup time may vary depending on system configuration, network bandwidth, and VM provisioning speed.
The framework supports two deployment configurations:

1. **Single Node Framework**
2. **BlockRaFT distributed Node Framework**

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
The VM is created using Multipass, and all dependencies are installed inside the VM. After building via CMake, correctness can be verified using ctest. 
aq

Install snap in case not availbale
```bash
sudo apt update
sudo apt install snapd
```
```bash
sudo snap install multipass
```

Verify installation:

```bash
sudo multipass version
```

#### Create Single Node VM

```bash
sudo multipass launch -n nodeS --cpus 8 --mem 10G --disk 20G 24.04
```

Enter the VM:

```bash
sudo multipass shell nodeS
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
cd singleNode-serial
sudo multipass transfer -r ./ nodeS:singleNode-serial
```
OR,
```bash
cd singleNode-parallel
sudo multipass transfer -r ./ nodeS:singleNode-parallel
```

---

### 1.1.4 Building the Project

Inside the VM:

For **Serial** execution mode :

```bash
cd singleNode-serial
mkdir -p build
cd build
cmake ..
make
```

For **Parallel** execution mode :

```bash
cd singleNode-parallel
mkdir -p build
cd build
cmake ..
make
```

For verify the proper setup :
```bash
cd build 
ctest
```

---

## 1.2 BlockRaFT Distributed Node Framework

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
sudo multipass version
```

---

### 1.2.4 Creating Multipass VMs

```bash
cd BlockRaFT-distributed_node
chmod +x autoDeploy.sh
sudo ./autoDeploy.sh
```

---

### 1.2.5 Copying Project Files to VMs

```bash
sudo multipass transfer -r ./ node0:Distributed_Framework
sudo multipass transfer -r ./ node1:Distributed_Framework
sudo multipass transfer -r ./ node2:Distributed_Framework
```

---

### 1.2.6 Installing System Dependencies

Enter each VM in three different terminals:

```bash
sudo multipass shell node0
sudo multipass shell node1
sudo multipass shell node2
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

For verify the proper setup :
```bash
cd build 
ctest
```

---

---

# 2. Experiment Configuration & Running

This section explains how to configure and reproduce experiments.

---

## 2.1 Configuration File

Each execution folder contains:

```
config.json
```

Example:

```json
{
  "threadCount": 8,
  "txnCount": 10000,
  "blocks": 2,
  "scheduler": "parallel",
  "mode": "production"
}
```

### Parameters

- `threadCount` → Worker threads (parallel mode)
- `txnCount` → Number of transactions
- `blocks` → Blocks generated
- `scheduler` → `"serial"` or `"parallel"`
- `mode` → Execution mode

---

## 2.2 Experiment Workloads

Located in:

```bash
ICSA_ARTIFACT
└──Experiment_files/
   ├── Wallet/
   └── Voting/
```

### File Naming Format

```
<TxnCount>-<ConflictPercentage>.txt
```

Examples:

- `1000-0.txt`
- `5000-50.txt`

---

## 2.3 Preparing an Experiment

### Step 1: Select Workload

Choose from:

```bash
Experiment_files/Wallet/
```
or
```bash
Experiment_files/Voting/
```

---
### A. Single Node Experiments

### Step 2: Copy Transactions

Copy into:

```bash
singleNode-serial/testFile/testFile.txt
```
or
```bash
singleNode-parallel/testFile/testFile.txt
```

inside selected execution folder.

### B. BlockRaFT - Distributed Node Experiments

### Step 2: Copy Transactions

Copy into:

```bash
BlockRAFT-distributed_node/leader/testFile.txt
```

inside selected execution folder.

---

### Step 3: Copy Setup File

Copy:

```bash
Experiment_files/Wallet/setup.txt
```
or
```bash
Experiment_files/Voting/setup.txt
```

**Copy into**:

```bash
singleNode-serial/testFile/setup.txt
```
or
```bash
singleNode-parallel/testFile/setup.txt
```

inside selected execution folder.

### B. BlockRaFT - Distributed Node Experiments

### Step 3: Copy Setup File

Copy:

```bash
Experiment_files/Wallet/setup.txt
```

**Copy into**:

```bash
BlockRAFT-distributed_node/leader/setupFile.txt
```

inside selected execution folder.

---

## 2.4 Running Experiments

---

### Single Node — Serial

```bash
cd singleNode-serial/build
./node
```

---

### Single Node — Parallel

```bash
cd singleNode-parallel/build
./node
```

### BlockRaFT-distributed node

Run the below mentioned command in each VMs for example for 3 Node setup i.e (`node0`, `node1`, `node2`).

```bash
cd BlockRAFT-distributed_node/build
./node
```

---
### 2.2.4 Cleaning Up

```bash
sudo multipass stop node0 node1 node2
sudo multipass delete node0 node1 node2
sudo multipass purge
```

---

# 3. Reproducibility Guidelines

To reproduce results reported in the paper:

- Use identical `config.json` parameters
- Use the same workload file
- Use identical VM specs
- Avoid background workloads
- Run multiple iterations and report averages

---

# Notes

- All distributed nodes must use identical builds.
- Ensure etcd and Redpanda ports are open.
- Maintain consistent startup order.
- Recommended VM: 8 CPUs, 10GB RAM.

