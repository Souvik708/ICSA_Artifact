#include <pthread.h>

#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "../dagModule/DAGmodule.h"
#include "../smartContracts/eCommerce/eCommProcessor.h"
#include "../smartContracts/wallet/walletProcessor.h"
#include "../smartContracts/nft/nftProcessor.h"
#include "../smartContracts/voting/votingProcessor.h"
#include "block.pb.h"
#include "components.pb.h"
#include "matrix.pb.h"
#include "transaction.pb.h"

using namespace std;

class scheduler {
 public:
  GlobalState& state;
  DAGmodule dag;
  unordered_set<int> processed_components;
  vector<transaction::Transaction> transactions;
  eCommProcessor eCommPro;
  WalletProcessor walletPro;
  VotingProcessor votePro;
  NFTProcessor nftPro;
  atomic<bool> flag = true;
  atomic<bool> completeFlag = false;
  tbb::concurrent_hash_map<std::string, std::string> myMap;

  // Constructor
  scheduler(GlobalState& statePtr)
      : state(statePtr), eCommPro(state, myMap), walletPro(state, myMap),nftPro(state, myMap) ,votePro(state, myMap) {}

  int columnSum(int colIndex) {
    int sum = 0;
    for (const auto& row : dag.adjacencyMatrix) {
      if (colIndex < row.size()) {
        sum += row[colIndex];
      }
    }
    return sum;
  }

  void executeTxns(int PID) {
    while (dag.completedTxns < dag.totalTxns && flag.load()) {
      int txnId = dag.selectTxn();
      if (txnId != -1) {
        transaction::Transaction txn = transactions[txnId];
        transaction::TransactionHeader header;

        if (!header.ParseFromString(txn.header())) {
          cerr << "Failed to parse transaction header." << endl;
          continue;
        }

        bool localSuccess = false;
        if (header.family_name() == "wallet") {
          localSuccess = walletPro.ProcessTxn(txn);
        } else if (header.family_name() == "eComm") {
          localSuccess = eCommPro.ProcessTxn(txn);
        }  else if (header.family_name() == "voting") {
          localSuccess =votePro.ProcessTxn(txn);  // Call eCommerce processing
        } else if (header.family_name() == "nft") {
          localSuccess = nftPro.ProcessTxn(txn);  // Call eCommerce processing
        }else {
          cerr << "Unknown transaction family: " << header.family_name() << endl;
        }

        if (!localSuccess) {
          flag.store(false);  // One thread can stop all on failure
        }

        dag.complete(txnId);
        cout.flush();
      }
    }
  }

void clear() {
  dag.dagClean();  // Clear DAG state
  // Reset control flags
  flag.store(true);
  completeFlag.store(false);

  // Clear processed components
  processed_components.clear();

  // Clear local write-set map
  myMap.clear();

  // Reset processors if needed (optional: if they hold internal states)
  // Not resetting eCommPro, walletPro, etc., since they use references to state & myMap
}


  void extractBlock(Block block, int thCount) {
    transactions.assign(block.transactions().begin(), block.transactions().end());

    auto dagS = std::chrono::high_resolution_clock::now();
    dag.create(block, thCount);
    auto dagE = std::chrono::high_resolution_clock::now();
    auto dura3 = std::chrono::duration_cast<std::chrono::milliseconds>(dagE - dagS).count();

    cout << "total transactions are " << dag.totalTxns << endl;
    std::cout << "Time for dag creation: " << dura3 << " ms" << std::endl;
  }

  void flushMapToState(int thCount) {
    std::vector<std::pair<std::string, std::string>> entries;
    for (auto it = myMap.begin(); it != myMap.end(); ++it) {
      entries.emplace_back(it->first, it->second);
    }

    std::vector<std::thread> threads;
    int chunkSize = (entries.size() + thCount - 1) / thCount;

    for (int t = 0; t < thCount; ++t) {
      threads.emplace_back([&, t]() {
        int start = t * chunkSize;
        int end = std::min(start + chunkSize, (int)entries.size());

        for (int i = start; i < end; ++i) {
          state.insert(entries[i].first, entries[i].second);
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    // Sequential parent hash updates
    for (const auto& [key, value] : entries) {
      state.updateParentHashes(key);
    }
  }

  bool schedulTxns(int thCount) {
    vector<thread> threads(thCount);
    flag.store(true);
    completeFlag.store(false);

    for (int i = 0; i < thCount; ++i) {
      threads[i] = thread(&scheduler::executeTxns, this, i);
    }

    for (int i = 0; i < thCount; ++i) {
      threads[i].join();
    }

    
    if (flag.load()) {
      // cout<<"until here its fine"<<flag.load()<<endl;  
      flushMapToState(thCount);
    }
    // cout<<"until here its fine"<<flag.load()<<endl;
    return flag.load();
  }
};