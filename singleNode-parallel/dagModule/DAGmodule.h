#pragma once
#include <pthread.h>

#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

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
  unique_ptr<atomic<int>[]> inDegree;
  atomic<int> completedTxns{0}, lastTxn{0};
  int totalTxns, threadCount = 2;
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
    for (const auto& input : txHeader.inputs()) txn.inputs.push_back(input);

    txn.outputcount = txHeader.outputs_size();
    for (const auto& output : txHeader.outputs()) txn.outputs.push_back(output);

    return txn;
  }

  void dependencyMatrix(int PID) {
    int chunk = totalTxns / threadCount;
    int rem = totalTxns % threadCount;
    int start = PID * chunk + min(PID, rem);
    int end = start + chunk + (PID < rem ? 1 : 0);

    for (int tx = start; tx < end; tx++) {
      for (int j = tx + 1; j < totalTxns; j++) {
        bool flag = false;

        for (const auto& in : CurrentTransactions[tx].inputs) {
          for (const auto& out : CurrentTransactions[j].outputs) {
            if (in == out) {
              flag = true;
              break;
            }
          }
          if (flag) break;
        }

        if (!flag) {
          for (const auto& out : CurrentTransactions[tx].outputs) {
            for (const auto& in : CurrentTransactions[j].inputs) {
              if (out == in) {
                flag = true;
                break;
              }
            }
            if (flag) break;
          }
        }

        if (!flag) {
          for (const auto& out1 : CurrentTransactions[tx].outputs) {
            for (const auto& out2 : CurrentTransactions[j].outputs) {
              if (out1 == out2) {
                flag = true;
                break;
              }
            }
            if (flag) break;
          }
        }

        if (flag) {
          adjacencyMatrix[tx][j] = 1;
          inDegree[j].fetch_add(1);
        }
      }
    }
  }

  void DFSUtil(int v, vector<bool>& visited, components::componentsTable::component* component) {
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
    vector<bool> visited(n, false);
    int totalComponents = 0;

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

  bool create(Block block, int thCount) {
    threadCount = thCount;
    vector<thread> threads;
    CurrentTransactions.clear();

    int position = 0;
    for (const auto& transaction : block.transactions()) {
      CurrentTransactions.push_back(extractTransaction(transaction, position));
      position++;
    }
    totalTxns = position;

    adjacencyMatrix.assign(totalTxns, vector<int>(totalTxns, 0));
    inDegree = make_unique<atomic<int>[]>(totalTxns);
    for (int i = 0; i < totalTxns; ++i) {
      inDegree[i].store(0);
    }

    for (int i = 0; i < threadCount; ++i) {
      threads.emplace_back(&DAGmodule::dependencyMatrix, this, i);
    }

    for (auto& th : threads) th.join();

    return true;
  }

  int selectTxn() {
    for (int i = 0; i < totalTxns; ++i) {
      int expected = 0;
      if (inDegree[i].compare_exchange_strong(expected, -1)) {
        lastTxn.store(i);
        return i;
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

    for (int i = txnID + 1; i < totalTxns; ++i) {
      if (adjacencyMatrix[txnID][i] == 1) {
        inDegree[i].fetch_sub(1);
      }
    }
  }

  string serializeDAG() {
    matrix::DirectedGraph graphProto;
    graphProto.set_num_nodes(adjacencyMatrix.size());

    for (const auto& row : adjacencyMatrix) {
      auto* matrixRow = graphProto.add_adjacencymatrix();
      for (int edge : row) matrixRow->add_edges(edge);
    }

    string serializedData;
    if (!graphProto.SerializeToString(&serializedData)) {
      cerr << "Failed to serialize DAG." << endl;
      return "";
    }

    return serializedData;
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

  bool smartValidator(const std::string& blockData) {
    Block block;
    return true;
  }
};
