#include <curl/curl.h>
#include <openssl/sha.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "transaction.pb.h"

#define FAMILY_NAME "simplewallet"

std::string sha512(const std::string &data) {
  unsigned char hash[SHA512_DIGEST_LENGTH];
  SHA512((unsigned char *)data.c_str(), data.size(), hash);
  std::stringstream ss;
  for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}

class SimpleWalletClient {
 public:
  SimpleWalletClient(const std::string &baseUrl) : _baseUrl(baseUrl) {
    _clientID = sha512(FAMILY_NAME).substr(0, 8);  // Create a simple client ID
    _address =
        sha512(FAMILY_NAME).substr(0, 6) + sha512(_clientID).substr(0, 64);
  }

  void deposit(int value) { _wrap_and_send("deposit", std::to_string(value)); }

  void withdraw(int value) {
    _wrap_and_send("withdraw", std::to_string(value));
  }

  void transfer(int value, const std::string &clientToKey) {
    _wrap_and_send("transfer", std::to_string(value), clientToKey);
  }

  void balance() {
    CURL *curl = curl_easy_init();
    if (curl) {
      std::string url = _baseUrl + "/balance";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      std::string response_string;
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "Error fetching balance: " << curl_easy_strerror(res)
                  << std::endl;
      } else {
        std::cout << "Balance: " << response_string << std::endl;
      }
      curl_easy_cleanup(curl);
    }
  }

 private:
  std::string _baseUrl;
  std::string _clientID;
  std::string _address;

  static size_t write_callback(void *contents, size_t size, size_t nmemb,
                               void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  void _wrap_and_send(const std::string &action, const std::string &value,
                      const std::string &clientToKey = "") {
    // Generate the payload
    std::string payload = action + "," + value;
    if (!clientToKey.empty()) {
      payload += "," + clientToKey;
    }

    // Create a TransactionHeader
    transaction::TransactionHeader txn_header;
    txn_header.set_family_name(FAMILY_NAME);
    txn_header.add_inputs(_address);
    txn_header.set_client_id(_clientID);
    txn_header.set_client_nonce(random_hex());
    txn_header.add_outputs(_address);

    // Serialize the TransactionHeader
    std::string header_string;
    txn_header.SerializeToString(&header_string);

    // Create a Transaction
    transaction::Transaction txn;
    txn.set_header(header_string);
    txn.set_payload(payload);

    // Create a TransactionList
    transaction::TransactionList txn_list;
    txn_list.add_transactions()->CopyFrom(txn);

    // Serialize the TransactionList
    std::string txn_list_string;
    txn_list.SerializeToString(&txn_list_string);

    // Send the serialized transaction to the REST API
    CURL *curl = curl_easy_init();
    if (curl) {
      std::string url = _baseUrl + "/api/transaction";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, txn_list_string.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, txn_list_string.size());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      std::string response_string;
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "Error sending transaction: " << curl_easy_strerror(res)
                  << std::endl;
      } else {
        std::cout << "Transaction response: " << response_string << std::endl;
      }
      curl_easy_cleanup(curl);
    }
  }

  std::string random_hex() {
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
      ss << std::hex << ((rand() % 256) & 0xFF);
    }
    return ss.str();
  }
};

int main() {
  SimpleWalletClient client("http://localhost:18080");

  int choice;
  while (true) {
    std::cout << "Choose an action:\n1. Deposit\n2. Withdraw\n3. Check "
                 "Balance\n4. Exit\n";
    std::cin >> choice;

    if (choice == 1) {
      int value;
      std::cout << "Enter amount to deposit: ";
      std::cin >> value;
      client.deposit(value);
    } else if (choice == 2) {
      int value;
      std::cout << "Enter amount to withdraw: ";
      std::cin >> value;
      client.withdraw(value);
    } else if (choice == 3) {
      client.balance();
    } else if (choice == 4) {
      break;
    } else {
      std::cout << "Invalid choice. Please try again.\n";
    }
  }

  return 0;
}
