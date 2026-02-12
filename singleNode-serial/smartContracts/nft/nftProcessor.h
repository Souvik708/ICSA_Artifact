#pragma once
#include <tbb/concurrent_hash_map.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#include "../../merkleTree/globalState.h"
#include "sha512.h"
#include "transaction.pb.h"

using json = nlohmann::json;
using namespace std;

class NFTProcessor {
 private:
  GlobalState& state;
  tbb::concurrent_hash_map<std::string, std::string>& nftMap;
  transaction::TransactionHeader transactionHeader;

 public:
  NFTProcessor(GlobalState& stateRef,
               tbb::concurrent_hash_map<std::string, std::string>& mapRef)
      : state(stateRef), nftMap(mapRef) {}

  std::string computeImageHash(const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) {
      std::cerr << "Could not open image: " << imagePath << "\n";
      return "";
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return sha512(oss.str());
  }

  json getOrLoadOwnerNFTList(const std::string& owner,
                              tbb::concurrent_hash_map<std::string, std::string>::accessor& acc) {
    if (nftMap.find(acc, owner)) {
      try {
        return json::parse(acc->second);
      } catch (...) {
        acc->second = json::array().dump();
        return json::array();
      }
    }

    std::string value = state.getValue(owner);
    json nftList = value.empty() ? json::array() : json::parse(value);
    if (nftMap.insert(acc, owner)) {
      acc->second = nftList.dump();
    }
    return nftList;
  }

  bool createNFT(const std::string& imagePath, const std::string& owner) {
    std::string hash = computeImageHash(imagePath);
    if (hash.empty()) return false;

    tbb::concurrent_hash_map<std::string, std::string>::accessor acc;
    json imageList = getOrLoadOwnerNFTList(owner, acc);

    if (std::find(imageList.begin(), imageList.end(), hash) == imageList.end()) {
      imageList.push_back(hash);
      acc->second = imageList.dump();
    } 
    // else {
    //   std::cout << "NFT already exists for this owner.\n";
    // }

    return true;
  }

  bool transferNFT(const std::string& imagePath, const std::string& oldOwner,
                   const std::string& newOwner) {
    std::string hash = computeImageHash(imagePath);
    if (hash.empty()) {
      std::cout << "Invalid image or failed to compute hash.\n";
      return false;
    }

    // Modify old owner list
    {
      tbb::concurrent_hash_map<std::string, std::string>::accessor oldAcc;
      if (!nftMap.find(oldAcc, oldOwner)) {
        std::cout << "Old owner not found.\n";
        return false;
      }

      json oldList = json::parse(oldAcc->second);
      auto it = std::find(oldList.begin(), oldList.end(), hash);
      if (it == oldList.end()) {
        std::cout << "NFT not found under the old owner.\n";
        return false;
      }

      oldList.erase(it);
      oldAcc->second = oldList.dump();
    }

    // Modify new owner list
    {
      tbb::concurrent_hash_map<std::string, std::string>::accessor newAcc;
      json newList = getOrLoadOwnerNFTList(newOwner, newAcc);
      newList.push_back(hash);
      newAcc->second = newList.dump();
    }

    return true;
  }

  bool ProcessTxn(const transaction::Transaction& transaction) {
    if (!transactionHeader.ParseFromString(transaction.header())) {
      cerr << "Failed to extract transaction" << endl;
      return false;
    }

    string payload = transaction.payload();
    try {
      json parsedPayload = json::parse(payload);
      std::string verb = parsedPayload["Verb"];

      if (verb == "nft_create") {
        return createNFT(parsedPayload["ImagePath"], parsedPayload["Owner"]);
      } else if (verb == "nft_transfer") {
        return transferNFT(parsedPayload["ImagePath"],
                           parsedPayload["OldOwner"],
                           parsedPayload["NewOwner"]);
      } else {
        std::cerr << "Unknown NFT verb: " << verb << "\n";
        return false;
      }
    } catch (const std::exception& e) {
      std::cerr << "Failed to process NFT transaction: " << e.what() << "\n";
      return false;
    }
  }
};
