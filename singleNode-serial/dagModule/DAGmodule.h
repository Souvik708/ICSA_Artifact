#pragma once
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "block.pb.h"
#include "components.pb.h"
#include "matrix.pb.h"
#include "transaction.pb.h"

using namespace std;

struct TransactionStruct {
  int txn_no;
  int inputscount;
  vector<string> inputs;
  int outputcount;
  vector<string> outputs;
  int compID = -1;
};

class DAGmodule {
 public:
  vector<TransactionStruct> CurrentTransactions;
  vector<vector<int>> adjacencyMatrix;
  unique_ptr<std::atomic<int>[]> inDegree;
  atomic<int> completedTxns{0}, lastTxn{0};
  int totalTxns, threadCount = 2;  // threadCount is now unused
  components::componentsTable cTable;

  DAGmodule() {}

  TransactionStruct extractTransaction(const transaction::Transaction& tx, int position) {
    TransactionStruct txn;
    txn.txn_no = position;

    transaction::TransactionHeader txHeader;
    if (!txHeader.ParseFromString(tx.header())) {
      cerr << "Failed to parse TransactionHeader." << endl;
      return txn;
    }

    txn.inputscount = txHeader.inputs_size();
    for (const auto& input : txHeader.inputs()) {
      txn.inputs.push_back(input);
    }

    txn.outputcount = txHeader.outputs_size();
    for (const auto& output : txHeader.outputs()) {
      txn.outputs.push_back(output);
    }

    return txn;
  }

  void dependencyMatrix() {
    bool flag = false;

    for (int txnAssigned = 0; txnAssigned < totalTxns; txnAssigned++) {
      for (int i = txnAssigned + 1; i < totalTxns; i++) {
        flag = false;

        // Input-Output
        for (int j = 0; j < CurrentTransactions[txnAssigned].inputscount; j++) {
          for (int k = 0; k < CurrentTransactions[i].outputcount; k++) {
            if (CurrentTransactions[txnAssigned].inputs[j] ==
                CurrentTransactions[i].outputs[k]) {
              flag = true;
            }
          }
        }

        // Output-Input
        if (!flag) {
          for (int j = 0; j < CurrentTransactions[txnAssigned].outputcount; j++) {
            for (int k = 0; k < CurrentTransactions[i].inputscount; k++) {
              if (CurrentTransactions[txnAssigned].outputs[j] ==
                  CurrentTransactions[i].inputs[k]) {
                flag = true;
              }
            }
          }
        }

        // Output-Output
        if (!flag) {
          for (int j = 0; j < CurrentTransactions[txnAssigned].outputcount; j++) {
            for (int k = 0; k < CurrentTransactions[i].outputcount; k++) {
              if (CurrentTransactions[txnAssigned].outputs[j] ==
                  CurrentTransactions[i].outputs[k]) {
                flag = true;
              }
            }
          }
        }

        if (flag) {
          adjacencyMatrix[txnAssigned][i] = 1;
          inDegree[i].fetch_add(1, std::memory_order_relaxed);
        }
      }
    }
  }

  void DFSUtil(int v, vector<bool>& visited,
               components::componentsTable::component* component) {
    auto* txn = component->add_transactionlist();
    txn->set_id(v);
    visited[v] = true;
    int n = adjacencyMatrix.size();
    for (int i = 0; i < n; i++) {
      if ((adjacencyMatrix[v][i] || adjacencyMatrix[i][v]) && !visited[i]) {
        DFSUtil(i, visited, component);
      }
    }
  }

  components::componentsTable connectedComponents() {
    int n = adjacencyMatrix.size();
    int totalComponents = 0;
    vector<bool> visited(n, false);

    for (int i = 0; i < n; i++) {
      if (!visited[i]) {
        auto* component = cTable.add_componentslist();
        DFSUtil(i, visited, component);
        totalComponents++;
      }
    }
    cTable.set_totalcomponents(totalComponents);
    return cTable;
  }

  void printDAGState() const {
    cout << "Adjacency Matrix:\n";
    for (size_t i = 0; i < adjacencyMatrix.size(); ++i) {
      cout << "Txn " << i << ": ";
      for (int val : adjacencyMatrix[i]) {
        cout << val << " ";
      }
      cout << "\n";
    }

    cout << "\nIn-Degree Array:\n";
    for (int i = 0; i < totalTxns; ++i) {
      cout << "Txn " << i << ": " << inDegree[i].load() << "\n";
    }
  }

  bool createfollower(Block block) {
    int position = 0;
    for (const auto& transaction : block.transactions()) {
      CurrentTransactions.push_back(extractTransaction(transaction, position));
      position++;
    }

    totalTxns = position;
    completedTxns = position;
    adjacencyMatrix.resize(totalTxns, vector<int>(totalTxns, 0));
    inDegree = unique_ptr<atomic<int>[]>(new atomic<int>[totalTxns]);
    for (int i = 0; i < totalTxns; ++i) {
      inDegree[i].store(0);
    }

    dependencyMatrix();  // Sequential

    return true;
  }

  std::string serializeDAG() {
    matrix::DirectedGraph graphProto;
    graphProto.set_num_nodes(adjacencyMatrix.size());

    for (const auto& row : adjacencyMatrix) {
      matrix::DirectedGraph::MatrixRow* matrixRow =
          graphProto.add_adjacencymatrix();
      for (int edge : row) {
        matrixRow->add_edges(edge);
      }
    }

    std::string serializedData;
    if (!graphProto.SerializeToString(&serializedData)) {
      cerr << "Failed to serialize the adjacency matrix." << endl;
      return "";
    }
    return serializedData;
  }

  bool create(Block block, int /*thCount*/) {
    int position = 0;
    for (const auto& transaction : block.transactions()) {
      CurrentTransactions.push_back(extractTransaction(transaction, position));
      position++;
    }

    totalTxns = position;
    adjacencyMatrix.resize(totalTxns, vector<int>(totalTxns, 0));
    inDegree = unique_ptr<atomic<int>[]>(new atomic<int>[totalTxns]);
    for (int i = 0; i < totalTxns; ++i) {
      inDegree[i].store(0);
    }

    dependencyMatrix();  // Sequential

    return true;
  }

  int selectTxn() {
    int pos = 0, var_zero = 0;

    for (int i = pos; i < totalTxns; i++) {
      if (inDegree[i].load() == 0) {
        var_zero = 0;
        if (inDegree[i].compare_exchange_strong(var_zero, -1)) {
          lastTxn.store(i);
          return i;
        }
      }
    }

    for (int i = 0; i < totalTxns; i++) {
      if (inDegree[i].load() == 0) {
        var_zero = 0;
        if (inDegree[i].compare_exchange_strong(var_zero, -1)) {
          lastTxn.store(i);
          return i;
        }
      }
    }

    return -1;
  }

  void complete(int txnID) {
    if (txnID < 0 || txnID >= totalTxns) {
      cerr << "Invalid txnID: " << txnID << endl;
      return;
    }

    inDegree[txnID].fetch_sub(1);
    completedTxns++;

    for (int i = txnID + 1; i < totalTxns; i++) {
      if (adjacencyMatrix[txnID][i] == 1) {
        inDegree[i].fetch_sub(1);
      }
    }
  }

  bool smartValidator(const std::string& blockData) {
    Block block;
    return true;
    // Placeholder for real validation logic
  }

  void dagClean() {
    CurrentTransactions.clear();
    adjacencyMatrix.clear();
    inDegree.reset();
    completedTxns.store(0);
    lastTxn.store(0);
    totalTxns = 0;
    cTable.Clear();
  }
};
