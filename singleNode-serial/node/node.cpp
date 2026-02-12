#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "../Crow/include/crow.h"
#include "../blockProducer/blockProducer.h"
#include "../blocksDB/blocksDB.h"
#include "../merkleTree/globalState.h"
#include "../node/transactionQueue.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/serialScheduler.h"
#include "transaction.pb.h"
#include "../testBp/testingBlockProducer.h"

using namespace std;

LockFreeQueue& queue = LockFreeQueue::getInstance();
blocksDB db;

// Custom log handler to suppress Crow logs
class NullLogger : public crow::ILogHandler {
 public:
  void log(std::string /*message*/, crow::LogLevel /*level*/) {
    // Do nothing (disable logging)
  }
};

string consumerFunction() {
  std::string message;
  if (queue.pop(message)) {
    // std::cout << "Consumed: " << message << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(50));
    return message;
  }
  return "";
}

int main() {
  std::ifstream configFile("../config.json");
  if (!configFile.is_open()) {
    std::cerr << "Failed to open config.json" << std::endl;
    return 1;
  }

  nlohmann::json configJson;
  configFile >> configJson;

  int threadCount = configJson["threadCount"];
  int txnCount = configJson["txnCount"];
  int blocksCount = configJson["blocks"];
  std::string schedulerMode = configJson["scheduler"];
  state.duplicateState("globalState_tmp");
  GlobalState tmp("globalState_tmp");
  scheduler parallelScheduler(tmp);

  // std::this_thread::sleep_for(std::chrono::seconds(200));  // let API
  // initialize

  for (int count = 0; count < blocksCount; ++count) {
    Block newBlock;
    BlockHeader header;
    TestBlockProducer text ;
    auto start = std::chrono::high_resolution_clock::now();
    if(count%2 == 0){
   
      cout << "Setup File Running : " << endl ;
      newBlock = text.produce(db,txnCount,"../testFile/setup.txt",tmp) ;
           }
           else{
            cout << "Test File Running : " << endl ;
            newBlock = text.produce(db,txnCount,"../testFile/testFile.txt",tmp) ;
  
           } 
    string serializedBlock;
    if (!newBlock.SerializeToString(&serializedBlock)) {
      cerr << "Failed to serialize the block." << endl;
    }
    string headerString = newBlock.header();
    if (!header.ParseFromString(headerString)) {
      cerr << "Failed to read graph from file." << endl;
      return false;
    }
    cout << "block being generated" << endl;
    auto exeS = std::chrono::high_resolution_clock::now();

    if (parallelScheduler.dag.create(newBlock, threadCount)) {
      parallelScheduler.extractBlock(newBlock, threadCount);
      if (!parallelScheduler.schedulTxns(threadCount)) {
        std::cerr << "Parallel execution failed.\n";
        break;
      }

      parallelScheduler.dag.dagClean();
    } else {
      std::cerr << "Failed to create DAG for block.\n";
    }
    db.storeBlock("B" + to_string(header.block_num()), serializedBlock);
    auto exeE = std::chrono::high_resolution_clock::now();
    auto dura3 =
        std::chrono::duration_cast<std::chrono::milliseconds>(exeS - start)
            .count();
    std::cout << "Time for block creation: " << dura3 << " ms" << std::endl;
    auto dura4 =
        std::chrono::duration_cast<std::chrono::milliseconds>(exeE - exeS)
            .count();
    std::cout << "Time for total execution: " << dura4 << " ms" << std::endl;
    // std::cout << "Execution of block " << newBlockNum << " done\n";
  }
  db.destroyDB();
  // restapiThread.join();
  return 0;
}