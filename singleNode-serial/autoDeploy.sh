#!/usr/bin/env bash

set -e  # Exit immediately if any command fails

###############################################################################
# 1) Create one or more Multipass instances
###############################################################################
NODES=("nodeS")  # Define an array of node names

echo -e "\033[31m=== Creating multipass instances (${NODES[*]}) ===\033[0m"

for NODE in "${NODES[@]}"; do
  if multipass list | awk '{print $1}' | grep -qx "$NODE"; then
    echo "Instance '$NODE' already exists. Skipping creation..."
  else
    multipass launch -n "$NODE" --cpus 8 --mem 10G --disk 20G 24.04
    echo "Instance '$NODE' created successfully."
  fi
done

# Give the VMs some time to boot up
sleep 5

###############################################################################
# 2) Retrieve IP addresses for the newly launched instances
###############################################################################
echo -e "\033[31m=== Retrieving IP addresses ===\033[0m"
IP_NODE=$(multipass info nodeS | grep IPv4 | awk '{print $2}')

echo "nodeS IP: $IP_NODE"

###############################################################################
# 3) Install Docker inside each instance
###############################################################################
echo -e "\033[31m=== Installing Docker on each node ===\033[0m"

multipass exec nodeS -- bash -c "sudo rm -f /etc/apt/sources.list.d/docker.list || true"
multipass exec nodeS -- sudo apt update -y
multipass exec nodeS -- sudo apt install -y docker.io
multipass exec nodeS -- sudo systemctl start docker
multipass exec nodeS -- sudo systemctl enable docker
multipass exec nodeS -- docker --version
multipass exec nodeS -- sudo usermod -aG docker ubuntu
multipass exec nodeS -- sudo apt install -y docker-compose


########################
# Redpanda on nodeS
########################
multipass exec nodeS -- bash -c "\
cat <<EOF > redpanda-nodes.yaml
version: '3.8'
services:
  redpanda:
    image: docker.redpanda.com/redpandadata/redpanda:v24.2.10
    container_name: redpanda-node
    command:
      - redpanda
      - start
      - --smp 1
      - --reserve-memory 0M
      - --overprovisioned
      - --node-id 0
      - --kafka-addr internal://0.0.0.0:9092,external://0.0.0.0:19092
      - --advertise-kafka-addr internal://$IP_NODE:9092,external://$IP_NODE:19092
      - --rpc-addr 0.0.0.0:33145
      - --advertise-rpc-addr $IP_NODE:33145
    ports:
      - '19092:19092'
      - '9092:9092'
      - '33145:33145'
      - '9644:9644'
EOF

docker-compose -p redpanda-nodes -f redpanda-nodes.yaml up -d
"
echo -e "\033[31m=== All done! ===\033[0m"