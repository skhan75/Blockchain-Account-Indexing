/**
 * @file BlockchainAccountIndexing.cpp
 * @brief Blockchain Account Indexing
 *
 * This program demonstrates the indexing and management of account updates in a blockchain system.
 * It processes JSON files containing account updates, indexes the accounts based on specified criteria,
 * and manages callbacks for the accounts.
 *
 * @author [Sami Ahmad Khan] [sami.ahmadkhan12@gmail.com]
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <random>
#include <chrono>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

struct Account {
    string id;
    string accountType;
    unordered_map<string, int> data;
    int tokens;
    int version;
    int callbackTimeMs;

    Account() : tokens(0), callbackTimeMs(0), version(0) {}
    Account(const string &id, const string &accountType, int tokens, int callbackTimeMs,
            const unordered_map<string, int> &data, int version)
        : id(id), accountType(accountType), tokens(tokens), callbackTimeMs(callbackTimeMs),
          data(data), version(version) {}

    // Comparison operator to compare two Account objects
    bool operator<(const Account &other) const {
        return tokens < other.tokens;
    }
};

struct AccountKey {
    string id;
    int version;

    bool operator==(const AccountKey &other) const {
        return id == other.id && version == other.version;
    }
};

struct AccountKeyHash {
    size_t operator()(const AccountKey &key) const {
        size_t h1 = hash<string>{}(key.id);
        size_t h2 = hash<int>{}(key.version);
        // It combines the two hash values using bitwise operations. It performs an XOR operation between h1 (the hash value for id) and the left shift of h2 (the hash value for version) by 1. This is done to combine the hash values in a way that preserves the uniqueness of the keys.
        return h1 ^ (h2 << 1);
    }
};

// Custom Comparator to heapify based on time
struct AccountTimeComparator {
    bool operator()(const pair<chrono::system_clock::time_point, Account> &a, const pair<chrono::system_clock::time_point, Account> &b) const {
        return a.first > b.first; // Compare based on the callback time
    }
};

// Comparison functor for sorting accounts by tokens in descending order
struct AccountTokenComparator {
    bool operator()(const Account &a1, const Account &a2) const {
        return a1.tokens > a2.tokens;
    }
};

class CallbackManager {
    private:
        // Using a heap to store callbacks provides an efficient way 
        // to manage and retrieve the callbacks based on their scheduled time
        vector<pair<chrono::system_clock::time_point, Account>> callbacks;
        unordered_map<string, int> indexMap;

        void updateIndexMap(const Account &account, int index) {
            indexMap[account.id] = index;
        }

        /**
         * Remove the callback at the given index from the heap.
         * @param index The index of the callback to be removed.
         * Algorithm:
         * 1. Check if the target callback index is not the last element in the vector
         * 2. Then swap last element with target index, so that we can pop it easily from the vector
         * 3. Update the indexMap with the new poition of the swapped callback.
         * 4. Pop the swapped callback from the heap. pop_heap method allows to pop the max element according
         * to heap property
         * 5. Push the swapped callback back into the heap
         */
        void removeAtIndex(int index) {
            if (index != callbacks.size() - 1) {
                std::swap(callbacks[index], callbacks.back());
                updateIndexMap(callbacks[index].second, index);
                std::pop_heap(callbacks.begin(), callbacks.end(), AccountTimeComparator());
                callbacks.pop_back();
                std::push_heap(callbacks.begin(), callbacks.end(), AccountTimeComparator());
            }
            else {
                callbacks.pop_back();
            }
        }

    public:
        /**
         * Schedule a callback for the given account at the specified time.
         * @param account The account associated with the callback.
         * @param callbackTime The time at which the callback should be triggered.
         */
        void scheduleCallback(const Account &account, chrono::system_clock::time_point callbackTime) {
            callbacks.emplace_back(callbackTime, account);
            updateIndexMap(account, callbacks.size() - 1);
            std::push_heap(callbacks.begin(), callbacks.end(), AccountTimeComparator());
        }

        /**
         * Cancel the callback associated with the given account.
         * @param account The account for which the callback should be canceled.
         */
        void cancelCallback(const Account& account) {
            cout<<account.id<<endl;
            auto it = indexMap.find(account.id);
            if (it != indexMap.end()) {
                int index = it->second;
                indexMap.erase(it);
                removeAtIndex(index);
            }
        }

        /**
         * Fire the callbacks that have reached or passed the current time.
         * @param currentTime The current time.
         */
        void fireCallbacks(std::chrono::system_clock::time_point currentTime) {
            while (!callbacks.empty() && callbacks.front().first <= currentTime) {
                const Account &account = callbacks.front().second;
                std::cout << "Callback fired for Account " << account.id << " v" << account.version << std::endl;
                std::pop_heap(callbacks.begin(), callbacks.end());
                callbacks.pop_back();
                indexMap.erase(account.id);
            }
        }
};

// AccountIndexer class is used to manage indexing of account updates
class AccountIndexer {
    private:
        unordered_map<string, priority_queue<
            Account, 
            vector<Account>, 
            AccountTokenComparator>
        > highestTokenAccounts;

    public:
        unordered_map<AccountKey, Account, AccountKeyHash> indexedAccounts;

        /**
         * Index the given account.
         * @param account The account to be indexed.
         */
        void indexAccount(const Account &account) {
            indexedAccounts[{account.id, account.version}] = account;
            cout << "Account " << account.id << " v" << account.version << " has been indexed." << endl;
        }

        /**
         * Remove the given account from the index.
         * @param account The account to be removed.
         */
        void removeAccount(const Account &account) {
            indexedAccounts.erase({account.id, account.version});
        }

        /**
         * Get the map of highest token value accounts for each account type.
         * @return The map of highest token value accounts.
         */
        unordered_map<string, priority_queue<Account, vector<Account>, AccountTokenComparator>> &getHighestTokenAccounts() {
            return highestTokenAccounts;
        }
};

class AccountManager {
    public:
        CallbackManager callbackManager;
        AccountIndexer accountIndexer;

        AccountManager() = default; // Add default constructor
        /**
         * Construct an AccountManager and process the account updates from the given file.
         * @param filename The name of the file containing the account updates.
         */
        AccountManager(const string &filename) {
            processAccountUpdates(filename);
        }

        /**
         * Process the account updates from the given file.
         * @param filename The name of the file containing the account updates.
         */
        void processAccountUpdates(const string &filename) {
            ifstream file(filename);
            if (!file.is_open()) {
                cerr << "Failed to open the file: " << filename << endl;
                return;
            }

            string fileContents((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            json jsonAccounts;
            try {
                jsonAccounts = json::parse(fileContents);
            }
            catch (const json::parse_error &e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                file.close();
                return;
            }

            file.close();

            vector<Account> accountUpdates;
            for (const auto &accountJson : jsonAccounts) {
                Account account = parseAccountUpdate(accountJson);
                accountUpdates.push_back(account);
            }

            for (const auto &account : accountUpdates) {
                ingestAccountUpdate(account);
                callbackManager.fireCallbacks(chrono::system_clock::now());
            }

            printHighestTokenValueAccounts();
        }

        /**
         * Search and filter accounts based on the specified criteria.
         * @param accountType The account type to filter by (optional).
         * @param minTokens The minimum token value to filter by (optional).
         * @param maxTokens The maximum token value to filter by (optional).
         * @return A vector of filtered accounts.
         */
        vector<Account> searchAndFilterAccounts(
            const string &accountType = "", 
            int minTokens = numeric_limits<int>::min(), 
            int maxTokens=numeric_limits<int>::max()
        ) {
            vector<Account> filteredAccounts;
            for(const auto& pair: accountIndexer.indexedAccounts) {
                const Account& account = pair.second;
                
                // Apply filters
                if(!accountType.empty() && account.accountType!=accountType) continue;
                if(account.tokens < minTokens || account.tokens > maxTokens) continue;
 
                // Account meets filter criteria
                filteredAccounts.push_back(account);
            }
            return filteredAccounts;
        }

    private: 
        /**
         * Parse the account update from the given JSON object.
         * @param accountJson The JSON object representing an account update.
         * @return The parsed Account object.
         */
        Account parseAccountUpdate(const json &accountJson) {
            string id = accountJson["id"];
            string accountType = accountJson["accountType"];
            int tokens = accountJson["tokens"];
            int callbackTimeMs = accountJson["callbackTimeMs"];
            unordered_map<string, int> data = accountJson["data"];
            int version = accountJson["version"];
            return Account(id, accountType, tokens, callbackTimeMs, data, version);
        }

        /**
         * Ingest the account update by indexing it, updating the highest token accounts, and scheduling a callback if necessary.
         * @param account The account to be ingested.
         */
        void ingestAccountUpdate(const Account &account) {
            AccountKey key = {account.id, account.version};
            auto &highestTokens = accountIndexer.getHighestTokenAccounts();
            auto it = highestTokens.find(account.accountType);
            if (it != highestTokens.end()) {
                priority_queue<Account, vector<Account>, AccountTokenComparator> &tokenAccounts = it->second;
                bool accountFound = false;
                auto jt = tokenAccounts;
                // p.s. priority_queue doesn't natively support find_if() method, so I have implemented the 
                // iteration to find the relevant account in the queue, remove it and heapify
                while (!jt.empty()) {
                    if (jt.top().id == account.id) {
                        accountFound = true;
                        break;
                    }
                    jt.pop();
                }
                if (accountFound) {
                    if (account.version <= jt.top().version)
                        return;
                    callbackManager.cancelCallback(jt.top());
                    accountIndexer.removeAccount(jt.top());
                    tokenAccounts = removeAccountFromPriorityQueue(tokenAccounts, account);
                }
            }

            accountIndexer.indexAccount(account);

            priority_queue<Account, vector<Account>, AccountTokenComparator> &highestTokensForType = highestTokens[account.accountType];
            highestTokensForType = insertAccountIntoPriorityQueue(highestTokensForType, account);
            if (highestTokensForType.size() > 3) {
                highestTokensForType.pop();
            }

            chrono::milliseconds delay(account.callbackTimeMs + getRandomDelay());
            chrono::system_clock::time_point callbackTime = chrono::system_clock::now() + delay;
            callbackManager.scheduleCallback(account, callbackTime);
        }

        /**
         * Get a random delay in milliseconds.
         * @return The random delay.
         */
        int getRandomDelay() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1000);
            int delay = dis(gen);

            return delay;
        }

        /**
         * Print the highest token value accounts for each account type.
         */
        void printHighestTokenValueAccounts() {
            for (const auto &pair : accountIndexer.getHighestTokenAccounts()) {
                const string &accountType = pair.first;
                const priority_queue<Account, vector<Account>, AccountTokenComparator> &tokenAccounts = pair.second;

                cout << "Highest token value accounts for account type " << accountType << ":" << endl;

                priority_queue<Account, vector<Account>, AccountTokenComparator> temp = tokenAccounts;
                while (!temp.empty()) {
                    const Account &account = temp.top();
                    cout << "Account " << account.id << " v" << account.version << ": Tokens - " << account.tokens << endl;
                    temp.pop();
                }
                cout << endl;
            }
        }

        /**
         * Insert the account into the priority queue while maintaining the descending order of tokens.
         * @param pq The priority queue.
         * @param account The account to be inserted.
         * @return The updated priority queue.
         */
        priority_queue<Account, vector<Account>, AccountTokenComparator> insertAccountIntoPriorityQueue(priority_queue<Account, vector<Account>, AccountTokenComparator> pq, const Account &account) {
            priority_queue<Account, vector<Account>, AccountTokenComparator> temp;
            while (!pq.empty() && pq.top().tokens > account.tokens) {
                temp.push(pq.top());
                pq.pop();
            }
            temp.push(account);
            while (!pq.empty()) {
                temp.push(pq.top());
                pq.pop();
            }
            return temp;
        }

        /**
         * Remove the account from the priority queue.
         * @param pq The priority queue.
         * @param account The account to be removed.
         * @return The updated priority queue.
         */
        priority_queue<Account, vector<Account>, AccountTokenComparator> removeAccountFromPriorityQueue(priority_queue<Account, vector<Account>, AccountTokenComparator> pq, const Account &account) {
            priority_queue<Account, vector<Account>, AccountTokenComparator> temp;
            while (!pq.empty() && pq.top().id != account.id) {
                temp.push(pq.top());
                pq.pop();
            }
            pq.pop(); // Remove the account
            while (!pq.empty()) {
                temp.push(pq.top());
                pq.pop();
            }
            return temp;
        }
};

int main() {
    // Test Case 1: Single Account Update
    {
        AccountManager accountManager("single_account_update.json");
        assert(accountManager.accountIndexer.indexedAccounts.size() == 1);
        assert(accountManager.accountIndexer.indexedAccounts.count({"GzbXUY1JQwRVUf3j3myg2NbDRwD5i4jD4HJpYhVNfiDm", 123}) == 1);
    }
    // Test Case 2: Multiple Account Updates with Callbacks
    {
        AccountManager accountManager("multi_account_updates_with_callback.json");
        assert(accountManager.accountIndexer.indexedAccounts.size() == 3);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account1", 2}) == 1);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account2", 1}) == 1);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account3", 1}) == 1);
    }
    // Test Case 3: Account Update with Higher Tokens Replacing Existing Account (Callback cancellation)
    {
        AccountManager accountManager("account_replaced_by_higher_token.json");
        assert(accountManager.accountIndexer.indexedAccounts.size() == 1);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account1", 2}) == 1);
    }
    // Test Case 4: Three account updates with different IDs and account types. All three updates have different versions. The assertions check that all three accounts are indexed correctly.
    {
        AccountManager accountManager("multi_account_multi_version_indexing.json");
        assert(accountManager.accountIndexer.indexedAccounts.size() == 2);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account1", 3}) == 1);
        assert(accountManager.accountIndexer.indexedAccounts.count({"account2", 1}) == 1);
    }
    // Test Case 5: Accounts should get filtered based on the criteria
    {
        AccountManager accountManager("multi_accounts_to_be_filtered.json");
        vector<Account> filteredAccounts = accountManager.searchAndFilterAccounts("user", 200, 400);
        // Validate the filtered accounts
        assert(filteredAccounts.size() == 2);
        assert(filteredAccounts[0].id == "id4");
        assert(filteredAccounts[0].accountType == "user");
        assert(filteredAccounts[0].tokens == 400);
        assert(filteredAccounts[1].id == "id3");
        assert(filteredAccounts[1].accountType == "user");
        assert(filteredAccounts[1].tokens == 300);
    }
    return 0;
}