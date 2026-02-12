#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <memory>

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
  int threadCount;  // threadCount is now unused
  vector<transaction::Transaction> transactions;
  unique_ptr<std::atomic<int>[]> compCount;
  eCommProcessor eCommPro;
  WalletProcessor walletPro;
  VotingProcessor votePro;
  NFTProcessor nftPro;
  atomic<bool> flag = true, completeFlag = false;
  tbb::concurrent_hash_map<std::string, std::string> myMap;
  
  // Constructor
  scheduler(GlobalState& statePtr)
      : state(statePtr), eCommPro(state, myMap), walletPro(state, myMap), nftPro(state,myMap), votePro(state, myMap) {}

  int columnSum(int colIndex) {
    int sum = 0;

    for (const auto& row : dag.adjacencyMatrix) {
      if (colIndex < row.size()) {
        sum += row[colIndex];
      }
    }

    return sum;
  }

  void executeTxns() {
    while (dag.completedTxns < dag.totalTxns && flag == true) {
      int txnId = dag.selectTxn();
      if (txnId != -1) {
        transaction::Transaction txn = transactions[txnId];  // Directly use txnId to get the transaction
        transaction::TransactionHeader header;

        if (!header.ParseFromString(txn.header())) {
          cerr << "Failed to parse transaction header." << endl;
          continue;
        }

        // Process transactions based on the family_name
        if (header.family_name() == "wallet") {
          flag.store(walletPro.ProcessTxn(txn));  // Call wallet processing
        } else if (header.family_name() == "eComm") {
          flag.store(eCommPro.ProcessTxn(txn));  // Call eCommerce processing
        } else if (header.family_name() == "nft") {
          flag.store(nftPro.ProcessTxn(txn));  // Call eCommerce processing
        } else if (header.family_name() == "voting") {
          flag.store(votePro.ProcessTxn(txn));  // Call eCommerce processing
        } else {
          flag.store(false);  // If transaction type doesn't match, set flag to false
        }

        dag.complete(txnId);  // Mark the transaction as complete
        cout.flush();
      }
    }
  }

  void extractBlock(Block block, int /*thCount*/) {
    // The threadCount is now unused, so this will only initialize necessary data
    transactions.assign(block.transactions().begin(), block.transactions().end());

    auto dagS = std::chrono::high_resolution_clock::now();
    dag.create(block, 1);  // We no longer use multiple threads, so pass 1
    auto dagE = std::chrono::high_resolution_clock::now();
    auto dura3 =
        std::chrono::duration_cast<std::chrono::milliseconds>(dagE - dagS)
            .count();

    cout << "total transactions are " << dag.totalTxns << endl;
    std::cout << "Time for dag creation: " << dura3 << " ms" << std::endl;
  }

  void flushMapToState(int /*thCount*/) {
    std::vector<std::pair<std::string, std::string>> entries;

    for (auto it = myMap.begin(); it != myMap.end(); ++it) {
      entries.emplace_back(it->first, it->second);
    }

    // Now, flush entries to state sequentially
    for (const auto& entry : entries) {
      state.insert(entry.first, entry.second);
    }

    // Sequentially update parent hashes after insertions
    for (const auto& [key, value] : entries) {
      state.updateParentHashes(key);
    }
  }

  bool schedulTxns(int /*thCount*/) {
    flag = true;
    completeFlag.store(false);

    // Sequentially execute transactions
    executeTxns();

    // After execution, flush the map to state if flag is true
    if (flag) {
      flushMapToState(1);
    }

    return flag.load();
  }
};
