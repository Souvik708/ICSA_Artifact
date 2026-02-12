#include <curl/curl.h>  // You need to link with libcurl

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../merkleTree/globalState.h"
#include "transaction.pb.h"

using namespace std;

class eCommClient {
 private:
  transaction::Transaction transaction;
  transaction::TransactionHeader transactionHeader;
  struct Node {
    string value;
    string hash;
    vector<string> children;
  };
  // Function to split a string by space and return a vector of strings
  vector<string>* splitString(const string& str, char delimiter = ' ') {
    vector<string> result[2];
    stringstream ss(str);
    string temp;
    while (getline(ss, temp, delimiter)) {
      result[0].push_back(temp);
      getline(ss, temp, delimiter);
      result[1].push_back(temp);
    }
    return result;
  }

 public:
  rocksdb::DB* db;
  rocksdb::Options options;
  rocksdb::Status status =
      rocksdb::DB::OpenForReadOnly(options, "globalState", &db);

  // Function to send the serialized transaction to the REST API using libcurl
  string sendTransactionToRestAPI(const std::string& serializedTransaction) {
    CURL* curl;
    CURLcode res;
    string result;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
      struct curl_slist* headers = nullptr;
      headers =
          curl_slist_append(headers, "Content-Type:application/octet-stream");

      // Set the URL for the REST API endpoint
      curl_easy_setopt(curl, CURLOPT_URL,
                       "http://localhost:18080/api/transaction");

      // Set the HTTP headers
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

      // Set the POST data
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, serializedTransaction.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                       serializedTransaction.size());

      // Perform the request, res will get the return code
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                  << std::endl;
        result = "curl_easy_perform() failed";
      } else {
        result = "Transaction successfully sent to REST API.";
      }
      // Clean up
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    return result;
  }
  string computeHash(const string& input) {
    EVP_MD_CTX* mdctx;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLength = 0;

    if ((mdctx = EVP_MD_CTX_new()) == NULL)
      throw runtime_error("Error in EVP_MD_CTX creation");

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
      throw runtime_error("Error in EVP_MD_CTX digest");

    if (1 != EVP_DigestUpdate(mdctx, input.data(), input.size()))
      throw runtime_error("Error in EVP_MD_CTX digest update");

    if (EVP_DigestFinal_ex(mdctx, hash, &hashLength) != 1) {
      EVP_MD_CTX_free(mdctx);
      throw runtime_error("Error in EVP_sha256 creation");
    }

    EVP_MD_CTX_free(mdctx);
    stringstream ss;
    for (unsigned int i = 0; i < hashLength; i++) {
      ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
  }
  Node deserializeNode(const string& data) {
    Node node;
    size_t pos = 0, nextPos = data.find(',');
    node.hash = data.substr(pos, nextPos - pos);
    pos = nextPos + 1;
    nextPos = data.find(',', pos);
    node.value = data.substr(pos, nextPos - pos);
    pos = nextPos + 1;
    while ((nextPos = data.find(',', pos)) != string::npos) {
      node.children.push_back(data.substr(pos, nextPos - pos));
      pos = nextPos + 1;
    }
    if (pos < data.size()) node.children.push_back(data.substr(pos));
    return node;
  }
  string getValue(const string& key) {
    string keyHash = computeHash(key);
    Node node = getNode(keyHash);
    return node.value;
  }

  Node getNode(const string& key) {
    string data;
    Node node = {"", "", {}};
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &data);
    if (status.ok()) return deserializeNode(data);
    return node;
  }
  // Modify this function to accept the command as a single string
  string processCommand(const std::vector<std::string>& commands) {
    // Validate the number of arguments
    if (commands.size() < 2 || commands.size() > 4) {
      return "Error: Number of arguments wrong";
    }

    // Prepare transaction header
    transactionHeader.set_family_name("eComm");
    transactionHeader.set_client_id(commands[1]);
    transactionHeader.set_client_nonce(to_string(rand()));

    string command = commands[0];

    // Process the command
    if (command == "addToCart") {
      std::string addr1 = "eComm" + commands[1] + "Cart";
      std::string addr2 = "eComm" + commands[2];
      transactionHeader.add_inputs(addr1);
      transactionHeader.add_outputs(addr1);
      transactionHeader.add_inputs(addr2);
      transactionHeader.add_outputs(addr2);

      transaction.set_payload("{\"Verb\": \"addToCart\", \"Name\": \"" + addr1 +
                              "\",\"Item\": \"" + addr2 + "\", \"Value\": \"" +
                              commands[3] + "\"}");
    } else if (command == "removeFromCart") {
      std::string addr1 = "eComm" + commands[1] + "Cart";
      std::string addr2 = "eComm" + commands[2];
      transactionHeader.add_inputs(addr1);
      transactionHeader.add_outputs(addr1);
      transactionHeader.add_inputs(addr2);
      transactionHeader.add_outputs(addr2);

      transaction.set_payload("{\"Verb\": \"removeFromCart\", \"Name\": \"" +
                              addr1 + "\",\"Item\": \"" + addr2 +
                              "\", \"Value\": \"" + commands[3] + "\"}");
    } else if (command == "checkout") {
      std::string addr1 = "eComm" + commands[1] + "Cart";
      std::string cart = getValue(addr1);
      transactionHeader.add_inputs(addr1);
      transactionHeader.add_outputs(addr1);
      if (cart.empty()) {
        cart = "empty cart";
        cout << "Cart is empty" << endl;
        return cart;
      }
      vector<string>* result = splitString(cart);
      for (const string& word : result[0]) {
        transactionHeader.add_inputs(word);
        transactionHeader.add_outputs(word);
      }
      transaction.set_payload("{\"Verb\": \"checkout\", \"Name1\": \"" + addr1 +
                              "\"}");
    } else if (command == "refillItem") {
      std::string addr1 = "eComm" + commands[1];
      transactionHeader.add_inputs(addr1);
      transactionHeader.add_outputs(addr1);

      transaction.set_payload("{\"Verb\": \"refillItem\", \"Name\": \"" +
                              addr1 + "\", \"Value\": \"" + commands[2] +
                              "\"}");
    } else if (command == "viewCart") {
      std::string addr1 = "eComm" + commands[1] + "Cart";
      std::string cart = getValue(addr1);
      if (cart.empty()) {
        cart = "empty cart";
        cout << "Cart is empty" << endl;
        return cart;
      }
      vector<string>* result = splitString(cart);
      cout << "The contents of the cart are " << endl;
      int i = 0;
      for (const string& word : result[0]) {
        cout << word << ":  " << result[1][i] << endl;
        i++;
      }
      return cart;
    } else {
      return "Error: Invalid command.\n Available commands:\n"
             "  addToCart <client_id> <item_id> <quantity>\n"
             "  removeFromCart <client_id> <item_id> <quantity>\n"
             "  ViewCart <client_id> \n"
             "  checkout <client_id> \n"
             "  refillItem <item_id> <quantity>";
    }

    // Serialize the transaction header
    string serializedHeader;
    if (!transactionHeader.SerializeToString(&serializedHeader)) {
      return "Failed to serialize TransactionHeader";
    }

    transaction.set_header(serializedHeader);
    transaction.add_dependencies("");

    // Serialize the transaction
    std::string serializedTransaction;
    if (!transaction.SerializeToString(&serializedTransaction)) {
      return "Failed to serialize Transaction";
    }

    // Send the serialized transaction to the REST API
    return sendTransactionToRestAPI(serializedTransaction);
  }
};
