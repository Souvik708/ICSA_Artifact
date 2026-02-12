#include <curl/curl.h>  // You need to link with libcurl

#include <fstream>
#include <iostream>

#include "transaction.pb.h"  // Include the generated Protobuf header

// Function to create a serialized Transaction message based on
// transaction.proto
std::string createSerializedTransaction() {
  transaction::TransactionHeader transactionHeader;
  transactionHeader.set_family_name("intkey");
  transactionHeader.set_client_id("client_12345");
  transactionHeader.set_client_nonce("random_nonce");
  transactionHeader.add_inputs("input1");
  transactionHeader.add_inputs("input2");
  transactionHeader.add_outputs("output1");
  transactionHeader.add_outputs("output2");

  // Serialize TransactionHeader
  std::string serializedHeader;
  if (!transactionHeader.SerializeToString(&serializedHeader)) {
    std::cerr << "Failed to serialize TransactionHeader." << std::endl;
    return "";
  }

  // Create Transaction
  transaction::Transaction transaction;
  transaction.set_header(serializedHeader);
  transaction.add_dependencies("dependency1");
  transaction.add_dependencies("dependency2");
  transaction.set_payload("{'Verb': 'set', 'Name': 'value', 'Value': 123}");

  // Serialize Transaction
  std::string serializedTransaction;
  if (!transaction.SerializeToString(&serializedTransaction)) {
    std::cerr << "Failed to serialize Transaction." << std::endl;
    return "";
  }

  return serializedTransaction;
}

// Function to send the serialized transaction to the REST API using libcurl
void sendTransactionToRestAPI(const std::string& serializedTransaction) {
  CURL* curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (curl) {
    struct curl_slist* headers = nullptr;
    headers =
        curl_slist_append(headers, "Content-Type: application/octet-stream");

    // Set the URL for the REST API endpoint
    curl_easy_setopt(curl, CURLOPT_URL,
                     "http://localhost:18080/api/transaction");

    // Set the HTTP headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set the POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, serializedTransaction.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, serializedTransaction.size());

    // Perform the request, res will get the return code
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                << std::endl;
    } else {
      std::cout << "Transaction successfully sent to REST API." << std::endl;
    }

    // Clean up
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }

  curl_global_cleanup();
}

int main() {
  // Create a serialized Transaction
  std::string serializedTransaction = createSerializedTransaction();
  if (serializedTransaction.empty()) {
    std::cerr << "Failed to create serialized transaction." << std::endl;
    return 1;
  }

  // Send the serialized transaction to the REST API
  sendTransactionToRestAPI(serializedTransaction);

  return 0;
}
