#include <gtest/gtest.h>

#include "eCommProcessor.h"
tbb::concurrent_hash_map<std::string, std::string> myMap;
// Helper function to create transactions
transaction::Transaction createTransaction(const std::string& payload) {
  transaction::Transaction txn;
  transaction::TransactionHeader header;
  header.set_family_name("eComm");
  header.set_client_id("testClient");
  header.set_client_nonce("12345");

  std::string serializedHeader;
  header.SerializeToString(&serializedHeader);
  txn.set_header(serializedHeader);
  txn.set_payload(payload);

  return txn;
}

// Test fixture for eCommProcessor tests
class eCommProcessorTest : public ::testing::Test {
 protected:
  GlobalState state;
  eCommProcessor processor;

  eCommProcessorTest() : processor(state, myMap) {}

  void SetUp() override {}
};

// Test: Add an item to the cart
TEST_F(eCommProcessorTest, AddToCart) {
  std::string payload =
      R"({"Verb": "addToCart", "Name": "eCommTestCart", "Item": "item123", "Value": "2"})";
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_TRUE(processor.ProcessTxn(txn));
  EXPECT_EQ(state.getValue("eCommTestCart"), "item123 2 ");
}

// Test: Add an existing item to the cart (update quantity)
TEST_F(eCommProcessorTest, AddExistingItem) {
  state.insert("eCommTestCart", "item569 5 ");
  std::string payload =
      R"({"Verb": "addToCart", "Name": "eCommTestCart", "Item": "item569", "Value": "2"})";
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_TRUE(processor.ProcessTxn(txn));
  EXPECT_EQ(state.getValue("eCommTestCart"), "item569 7 ");
}

// Test: Remove an item from the cart
TEST_F(eCommProcessorTest, RemoveFromCart) {
  state.insert("eCommTestCart", "item123 5 ");
  std::string payload =
      R"({"Verb": "removeFromCart", "Name": "eCommTestCart", "Item": "item123", "Value": "2"})";
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_TRUE(processor.ProcessTxn(txn));
  EXPECT_EQ(state.getValue("eCommTestCart"), "item123 3 ");
}

// Test: Try to remove an item that doesn't exist in the cart
TEST_F(eCommProcessorTest, RemoveMissingItem) {
  state.insert("eCommTestCart", "item123 5 ");
  std::string payload =
      R"({"Verb": "removeFromCart", "Name": "eCommTestCart", "Item": "item567", "Value": "2"})";
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_FALSE(processor.ProcessTxn(txn));
}

// Test: Checkout should clear the cart
TEST_F(eCommProcessorTest, Checkout) {
  state.insert("eCommTestCart", "item123 3 item456 2 ");
  std::string payload =
      R"({"Verb": "checkout", "Name": "eCommTestCart"})";  // Fixed "Name1"
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_TRUE(processor.ProcessTxn(txn));
  EXPECT_EQ(state.getValue("eCommTestCart"), "");
}

// Test: Refill an item in inventory
TEST_F(eCommProcessorTest, RefillItem) {
  state.insert("eCommitem123", "5");
  std::string payload =
      R"({"Verb": "refillItem", "Name": "eCommitem123", "Value": "10"})";
  transaction::Transaction txn = createTransaction(payload);

  EXPECT_TRUE(processor.ProcessTxn(txn));
  EXPECT_EQ(state.getValue("eCommitem123"), "15");
}

// Main function to run all tests
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}