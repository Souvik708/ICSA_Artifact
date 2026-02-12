#pragma once

#include <curl/curl.h>  // You need to link with libcurl
#include <tbb/concurrent_hash_map.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>
using json = nlohmann::json;

#include "../../merkleTree/globalState.h"
#include "transaction.pb.h"

using namespace std;

class eCommProcessor {
 private:
  transaction::TransactionHeader transactionHeader;
  tbb::concurrent_hash_map<std::string, std::string>& myMap;

 public:
  GlobalState& state;

  eCommProcessor(GlobalState& statePtr,
                 tbb::concurrent_hash_map<std::string, std::string>& mapRef)
      : state(statePtr), myMap(mapRef) {}

  std::string getOrLoadValue(const std::string& name) {
    tbb::concurrent_hash_map<std::string, std::string>::const_accessor acc;
    if (myMap.find(acc, name)) {
      return acc->second;
    }

    std::string fromState = state.getValue(name);
    tbb::concurrent_hash_map<std::string, std::string>::accessor insertAcc;
    myMap.insert(insertAcc, name);
    insertAcc->second = fromState;
    return fromState;
  }

  bool checkout(const json& parsedPayload) {
    string cartAddress = parsedPayload["Name1"];
    string cartContents = getOrLoadValue(cartAddress);

    if (cartContents.empty()) {
      return true;
    }

    tbb::concurrent_hash_map<std::string, std::string>::accessor acc;
    myMap.insert(acc, cartAddress);
    acc->second = "";  // Clear the cart
    return true;
  }

  bool addToCart(const json& parsedPayload) {
    string cartAddress = parsedPayload["Name"];
    string item = parsedPayload["Item"];
    string quantity = parsedPayload["Value"];

    string cartContents = getOrLoadValue(cartAddress);
    istringstream iss(cartContents);
    string token, updatedCart;
    bool found = false;

    while (iss >> token) {
      string itemName = token;
      string itemQuantity;
      if (iss >> itemQuantity) {
        if (itemName == item) {
          found = true;
          int newQuantity = stoi(itemQuantity) + stoi(quantity);
          updatedCart += itemName + " " + to_string(newQuantity) + " ";
        } else {
          updatedCart += itemName + " " + itemQuantity + " ";
        }
      }
    }

    if (!found) {
      updatedCart += item + " " + quantity + " ";
    }

    tbb::concurrent_hash_map<std::string, std::string>::accessor acc;
    myMap.insert(acc, cartAddress);
    acc->second = updatedCart;

    return true;
  }

  bool removeFromCart(const json& parsedPayload) {
    string cartAddress = parsedPayload["Name"];
    string item = parsedPayload["Item"];
    string quantity = parsedPayload["Value"];

    string cartContents = getOrLoadValue(cartAddress);
    istringstream iss(cartContents);
    string token, updatedCart;
    bool found = false;

    while (iss >> token) {
      string itemName = token;
      string itemQuantity;
      if (iss >> itemQuantity) {
        if (itemName == item) {
          found = true;
          int newQuantity = stoi(itemQuantity) - stoi(quantity);
          if (newQuantity > 0) {
            updatedCart += itemName + " " + to_string(newQuantity) + " ";
          }
        } else {
          updatedCart += itemName + " " + itemQuantity + " ";
        }
      }
    }

    if (!found) {
      return false;
    }

    tbb::concurrent_hash_map<std::string, std::string>::accessor acc;
    myMap.insert(acc, cartAddress);
    acc->second = updatedCart;

    return true;
  }

  bool refillItem(const json& parsedPayload) {
    string itemAddress = parsedPayload["Name"];
    string quantity = parsedPayload["Value"];

    string existingQuantity = getOrLoadValue(itemAddress);
    int newQuantity = stoi(existingQuantity.empty() ? "0" : existingQuantity) +
                      stoi(quantity);

    tbb::concurrent_hash_map<std::string, std::string>::accessor acc;
    myMap.insert(acc, itemAddress);
    acc->second = to_string(newQuantity);

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

      if (verb == "checkout") {
        return checkout(parsedPayload);
      } else if (verb == "addToCart") {
        return addToCart(parsedPayload);
      } else if (verb == "removeFromCart") {
        return removeFromCart(parsedPayload);
      } else if (verb == "refillItem") {
        return refillItem(parsedPayload);
      } else {
        cerr << "Unknown transaction verb" << endl;
        return false;
      }
    } catch (const json::parse_error& e) {
      cerr << "Failed to parse payload as JSON: " << e.what() << endl;
      return false;
    } catch (const std::exception& e) {
      cerr << "Unexpected error: " << e.what() << endl;
      return false;
    }

    return true;
  }
};