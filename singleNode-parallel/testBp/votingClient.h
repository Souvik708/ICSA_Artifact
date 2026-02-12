
#include <curl/curl.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../merkleTree/globalState.h"
#include "transaction.pb.h"

using namespace std;

class votingClient {
 private:
  transaction::Transaction transaction;


  struct Node {
    string value;
    string hash;
    vector<string> children;
  };

 public:
 
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

      curl_easy_setopt(curl, CURLOPT_URL,
                       "http://localhost:18080/api/transaction");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, serializedTransaction.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                       serializedTransaction.size());

      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
             << endl;
        result = "curl failed";
      } else {
        result = "Transaction successfully sent.";
      }

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
      throw runtime_error("EVP_MD_CTX creation failed");

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) ||
        1 != EVP_DigestUpdate(mdctx, input.data(), input.size()) ||
        EVP_DigestFinal_ex(mdctx, hash, &hashLength) != 1) {
      EVP_MD_CTX_free(mdctx);
      throw runtime_error("SHA256 digest failed");
    }

    EVP_MD_CTX_free(mdctx);
    stringstream ss;
    for (unsigned int i = 0; i < hashLength; i++)
      ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
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


  transaction::Transaction processCommand(const vector<string>& commands) {
    transaction::TransactionHeader transactionHeader;
    string verb = commands[0];
    transactionHeader.set_family_name("voting");
    transactionHeader.set_client_id(commands.size() > 1 ? commands[1]
                                                        : "unknown");
     transactionHeader.set_client_nonce(to_string(rand()));

    if (verb == "registerVoter") {
      string voterKey = "voter" + commands[1];
      transactionHeader.add_inputs(voterKey);
      transactionHeader.add_outputs(voterKey);
      transaction.set_payload("{\"Verb\":\"registerVoter\",\"Name\":\"" +
                              voterKey + "\"}");

    } else if (verb == "registerCandidate") {
      string candidateKey = "candidate" + commands[1];
      transactionHeader.add_inputs(candidateKey);
      transactionHeader.add_outputs(candidateKey);
      transaction.set_payload("{\"Verb\":\"registerCandidate\",\"Name\":\"" +
                              candidateKey + "\"}");

    } else if (verb == "transferVote") {
      string fromKey = "voter" + commands[1];
      string toKey = "voter" + commands[2];
      transactionHeader.add_inputs(fromKey);
      transactionHeader.add_inputs(toKey);
      transactionHeader.add_outputs(fromKey);
      transactionHeader.add_outputs(toKey);
      transaction.set_payload("{\"Verb\":\"transferVote\",\"From\":\"" +
                              fromKey + "\",\"To\":\"" + toKey +
                              "\",\"Count\":\"" + commands[3] + "\"}");
                              

    } else if (verb == "castVote") {
      string voterKey = "voter" + commands[1];
      string candidateKey = "candidate" + commands[2];
      transactionHeader.add_inputs(voterKey);
      transactionHeader.add_inputs(candidateKey);
      transactionHeader.add_outputs(voterKey);
      transactionHeader.add_outputs(candidateKey);
      transaction.set_payload("{\"Verb\":\"castVote\",\"Voter\":\"" + voterKey +
                              "\",\"Candidate\":\"" + candidateKey + "\"}");

    } else {transaction.set_payload("{\"Verb\": \"empty\"}"); }

    // Serialize and send transaction
    string serializedHeader;
    transactionHeader.SerializeToString(&serializedHeader);
    transaction.set_header(serializedHeader);
    transaction.add_dependencies("");

    return transaction;
  }
};