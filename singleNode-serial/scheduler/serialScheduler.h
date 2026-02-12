// #include <iostream>
// #include <string>
// #include <vector>

// #include "../smartContracts/eCommerce/eCommProcessor.h"
// #include "../smartContracts/wallet/walletProcessor.h"
// #include "block.pb.h"
// #include "transaction.pb.h"

// using namespace std;

// class SerialScheduler {
//  public:
//   GlobalState& state;
//   eCommProcessor eCommPro;
//   WalletProcessor walletPro;
//   vector<transaction::Transaction> transactions;

//   // Constructor
//   SerialScheduler(GlobalState& statePtr)
//       : state(statePtr), eCommPro(state), walletPro(state) {}

//   // Load transactions from block
//   void extractBlock(const Block& block) {
//     transactions.assign(block.transactions().begin(),
//                         block.transactions().end());
//   }

//   // Run transactions in serial
//   bool scheduleTxns() {
//     for (const auto& txn : transactions) {
//       transaction::TransactionHeader header;
//       if (!header.ParseFromString(txn.header())) {
//         cerr << "Failed to parse transaction header." << endl;
//         return false;
//       }

//       bool result = false;

//       if (header.family_name() == "wallet") {
//         result = walletPro.ProcessTxn(txn);
//       } else if (header.family_name() == "eComm") {
//         result = eCommPro.ProcessTxn(txn);
//       } else {
//         cerr << "Unknown transaction family: " << header.family_name() <<
//         endl; return false;
//       }

//       if (!result) {
//         cerr << "Transaction failed. Halting execution." << endl;
//         return false;
//       }

//       // cout << "Transaction executed successfully." << endl;
//     }

//     return true;  // All transactions processed successfully
//   }
// };