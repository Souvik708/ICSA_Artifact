#include <gtest/gtest.h>
#ifdef UNIT_TEST
#define TESTS_DISABLED
#endif
#include "scheduler.h"
GlobalState state;
transaction::Transaction CreateWalletTransaction(string inputs, string value) {
  transaction::Transaction transaction;
  transaction::TransactionHeader header;
  header.set_family_name("wallet");

  transaction.set_payload("{\"Verb\": \"deposit\", \"Name\": \"" + inputs +
                          "\", \"Value\": \"" + value + "\"}");
  std::string serializedHeader;
  header.SerializeToString(&serializedHeader);
  transaction.set_header(serializedHeader);

  return transaction;
}

transaction::Transaction CreateECommTransaction(string inputs, string value) {
  transaction::Transaction transaction;
  transaction::TransactionHeader header;
  header.set_family_name("eComm");
  transaction.set_payload("{\"Verb\": \"refillItem\", \"Name\": \"" + inputs +
                          "\", \"Value\": \"" + value + "\"}");
  std::string serializedHeader;
  header.SerializeToString(&serializedHeader);
  transaction.set_header(serializedHeader);
  return transaction;
}
#ifndef TESTS_DISABLED
TEST(schedulTxnsTest, walletECommTxns) {
  scheduler sched(state);
  sched.dag.adjacencyMatrix = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  sched.dag.totalTxns = 3;
  sched.dag.completedTxns = 0;
  sched.dag.inDegree = unique_ptr<atomic<int>[]>(new atomic<int>[3]);

  for (size_t i = 0; i < 3; ++i) {
    sched.dag.inDegree[i].store(0, std::memory_order_relaxed);
  }
  BlockHeader block_header;

  // Setting values for block_header fields
  block_header.set_block_num(1);  // Example block number
  block_header.set_previous_block_id(
      "previous_block_id_string");        // Example previous block ID
  block_header.add_transaction_ids("0");  // Example transaction ID 1
  block_header.add_transaction_ids("1");  // Example transaction ID 2
  block_header.add_transaction_ids("2");  // Example transaction ID 3
  block_header.set_state_root_hash(
      "final_state_root_hash");            // Example state root hash
  block_header.set_timestamp(1676543200);  // Example timestamp

  // Step 2: Serialize BlockHeader (this will be a binary representation)
  string serialized_header;
  block_header.SerializeToString(
      &serialized_header);  // Serialize to string (binary format)

  // Step 3: Create Transactions using create_txn()

  vector<transaction::Transaction> transactions;

  // Create transactions using the create_txn function
  transactions.push_back(CreateWalletTransaction("client1", "100"));
  transactions.push_back(CreateECommTransaction("client2", "100"));
  transactions.push_back(CreateWalletTransaction("client3", "100"));

  Block block;
  block.set_header(serialized_header);  // Set the serialized header
  for (const auto& tx : transactions) {
    *block.add_transactions() = tx;
  }
  string serialized_block;
  block.SerializeToString(&serialized_block);
  sched.extractBlock(block, 2);

  bool flag = sched.schedulTxns(2);
  ASSERT_TRUE(flag);
}

TEST(schedulTxnsTest, walletTxns) {
  scheduler sched(state);
  sched.dag.adjacencyMatrix = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  sched.dag.totalTxns = 3;
  sched.dag.completedTxns = 0;
  sched.dag.inDegree = unique_ptr<atomic<int>[]>(new atomic<int>[3]);
  for (size_t i = 0; i < 3; ++i) {
    sched.dag.inDegree[i].store(0, std::memory_order_relaxed);
  }
  BlockHeader block_header;

  // Setting values for block_header fields
  block_header.set_block_num(1);  // Example block number
  block_header.set_previous_block_id(
      "previous_block_id_string");        // Example previous block ID
  block_header.add_transaction_ids("0");  // Example transaction ID 1
  block_header.add_transaction_ids("1");  // Example transaction ID 2
  block_header.add_transaction_ids("2");  // Example transaction ID 3
  block_header.set_state_root_hash(
      "final_state_root_hash");            // Example state root hash
  block_header.set_timestamp(1676543200);  // Example timestamp

  // Step 2: Serialize BlockHeader (this will be a binary representation)
  string serialized_header;
  block_header.SerializeToString(
      &serialized_header);  // Serialize to string (binary format)

  // Step 3: Create Transactions using create_txn()

  vector<transaction::Transaction> transactions;

  // Create transactions using the create_txn function
  transactions.push_back(CreateWalletTransaction("client1", "100"));
  transactions.push_back(CreateWalletTransaction("client2", "100"));
  transactions.push_back(CreateWalletTransaction("client3", "100"));

  Block block;
  block.set_header(serialized_header);  // Set the serialized header
  for (const auto& tx : transactions) {
    *block.add_transactions() = tx;
  }
  string serialized_block;
  block.SerializeToString(&serialized_block);
  sched.extractBlock(block, 2);

  bool flag = sched.schedulTxns(2);
  ASSERT_TRUE(flag);
}

TEST(schedulTxnsTest, eCommTxns) {
  scheduler sched(state);
  sched.dag.adjacencyMatrix = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  sched.dag.totalTxns = 3;
  sched.dag.completedTxns = 0;
  sched.dag.inDegree = unique_ptr<atomic<int>[]>(new atomic<int>[3]);
  for (size_t i = 0; i < 3; ++i) {
    sched.dag.inDegree[i].store(0, std::memory_order_relaxed);
  }
  BlockHeader block_header;

  // Setting values for block_header fields
  block_header.set_block_num(1);  // Example block number
  block_header.set_previous_block_id(
      "previous_block_id_string");        // Example previous block ID
  block_header.add_transaction_ids("0");  // Example transaction ID 1
  block_header.add_transaction_ids("1");  // Example transaction ID 2
  block_header.add_transaction_ids("2");  // Example transaction ID 3
  block_header.set_state_root_hash(
      "final_state_root_hash");            // Example state root hash
  block_header.set_timestamp(1676543200);  // Example timestamp

  // Step 2: Serialize BlockHeader (this will be a binary representation)
  string serialized_header;
  block_header.SerializeToString(
      &serialized_header);  // Serialize to string (binary format)

  // Step 3: Create Transactions using create_txn()

  vector<transaction::Transaction> transactions;

  // Create transactions using the create_txn function
  transactions.push_back(CreateECommTransaction("client1", "100"));
  transactions.push_back(CreateECommTransaction("client2", "100"));
  transactions.push_back(CreateECommTransaction("client3", "100"));

  Block block;
  block.set_header(serialized_header);  // Set the serialized header
  for (const auto& tx : transactions) {
    *block.add_transactions() = tx;
  }
  string serialized_block;
  block.SerializeToString(&serialized_block);
  sched.extractBlock(block, 2);
  bool flag = sched.schedulTxns(2);
  ASSERT_TRUE(flag);
}
#endif