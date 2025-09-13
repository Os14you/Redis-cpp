#include <server/Redis.hpp>

/* ====== Private methods ====== */

void RedisServer::onRequest(Connection& conn, const std::string& request) {
    Request parsed_request;
    Buffer response;

    if(parseRequest(request, parsed_request) != 0) {
        ResponseBuilder::outErr(response, ERR_PROTOCOL, "Protocol error");
        conn.want_close = true;
    } else {
        executeRequest(parsed_request, response);
    }

    if(!response.empty()) {
        uint32_t total_len = static_cast<uint32_t>(response.size());
        conn.appendOutgoing(reinterpret_cast<const uint8_t*>(&total_len), 4);
        conn.appendOutgoing(response.data(), response.size());
    }
}

bool RedisServer::parseUInt32(const char*& cursor, const char* buffer_end, uint32_t& value) {
    if(cursor + 4 > buffer_end)
        return false; // Not enough data to read a uint32_t

    memcpy(&value, cursor, 4);
    cursor += 4;

    return true; // Successfully read a uint32_t
}

int32_t RedisServer::parseRequest(const std::string& raw_data, Request& parsed_request) {
    const char* cursor = raw_data.data();
    const char* buffer_end = raw_data.data() + raw_data.size();

    uint32_t num_strings = 0;
    if(!parseUInt32(cursor, buffer_end, num_strings))
        return -1; // Failed to parse the number of strings

    const size_t K_MAX_ARGS = 1024;
    if(num_strings > K_MAX_ARGS)
        return -2; // Too many arguments

    while(num_strings--) {
        uint32_t str_len = 0;
        if(!parseUInt32(cursor, buffer_end, str_len))
            return -1; // Failed to parse string length
        
        if(cursor + str_len > buffer_end)
            return -3; // Not enough data for the string
        
        parsed_request.command.emplace_back(cursor, str_len);
        cursor += str_len;
    }

    if(cursor != buffer_end)
        return -4; // Trailing garbage after the command
    
    return 0; // Successfully parsed the request
}

void RedisServer::executeRequest(const Request& request, Buffer& response) {
    if(request.command.empty()) {
        ResponseBuilder::outErr(response, ERR_UNKNOWN_COMMAND, "Empty command");
        return;
    }

    const std::string& cmd = request.lowerCaseCommand();
    auto it = commandTable.find(cmd);

    if(it != commandTable.end()) {
        it->second(request, response);
    } else {
        handleUnknown(request, response);
    }
}

void RedisServer::handleKeys(const Request& request, Buffer& response) {
    (void) request;

    ResponseBuilder::outArr(response, static_cast<uint32_t>(dataStore.size()));

    dataStore.forEach([this, &response](HashTable::Node* node) {
        ResponseBuilder::outStr(response, static_cast<DataEntry*>(node)->key);
    });
}

void RedisServer::handlePing(const Request& request, Buffer& response) {
    (void) request; // Unused parameter
    if (request.command.size() > 2) {
        ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Wrong number of arguments for 'ping'");
        return;
    }

    if (request.command.size() == 1) {
        ResponseBuilder::outStr(response, "PONG");
    } else {
        ResponseBuilder::outStr(response, request.command[1]);
    }
}

void RedisServer::handleUnknown(const Request& request, Buffer& response) {
    ResponseBuilder::outErr(response, ERR_UNKNOWN_COMMAND, "Unknown command '" + request.command[0] + "'");
}

void RedisServer::handleSet(const Request& request, Buffer& response) {
    if(request.command.size() != 3) {
        ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Wrong number of arguments for 'set'");
        return;
    }

    DataEntry key_entry;
    key_entry.key = request.command[1];
    key_entry.hashCode = stringHash(key_entry.key);

    auto equals = [](HashTable::Node* node, HashTable::Node* key) {
        return static_cast<DataEntry*>(node)->key == static_cast<DataEntry*>(key)->key;
    };

    if(HashTable::Node* found_node = dataStore.lookup(&key_entry, equals)) {
        static_cast<DataEntry*>(found_node)->value = request.command[2];
    } else {
        auto new_entry = std::make_unique<DataEntry>();
        new_entry->key = request.command[1];
        new_entry->value = request.command[2];
        new_entry->hashCode = key_entry.hashCode;
        dataStore.insert(std::move(new_entry));
    }

    ResponseBuilder::outNil(response);
}

void RedisServer::handleGet(const Request& request, Buffer& response) {
    if(request.command.size() != 2) {
        ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Wrong number of arguments for 'get'");
        return;
    }

    DataEntry key_entry;
    key_entry.key = request.command[1];
    key_entry.hashCode = stringHash(key_entry.key);

    auto equals = [](HashTable::Node* node, HashTable::Node* key) {
        return static_cast<DataEntry*>(node)->key == static_cast<DataEntry*>(key)->key;
    };

    if(HashTable::Node* found_node = dataStore.lookup(&key_entry, equals)) {
        DataEntry* entry = static_cast<DataEntry*>(found_node);
        if (std::holds_alternative<std::string>(entry->value)) {
            ResponseBuilder::outStr(response, std::get<std::string>(entry->value));
        } else {
            ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Operation against a key holding the wrong kind of value");
        }
    } else {
        ResponseBuilder::outNil(response);
    }
}

void RedisServer::handleDel(const Request& request, Buffer& response) {
    if(request.command.size() != 2) {
        ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Wrong number of arguments for 'del'");
        return;
    }

    DataEntry key_entry;
    key_entry.key = request.command[1];
    key_entry.hashCode = stringHash(key_entry.key);

    auto equals = [](HashTable::Node* node, HashTable::Node* key) {
        return static_cast<DataEntry*>(node)->key == static_cast<DataEntry*>(key)->key;
    };

    if(dataStore.remove(&key_entry, equals)) {
        ResponseBuilder::outInt(response, 1);
    } else {
        ResponseBuilder::outInt(response, 0);
    }
}

void RedisServer::handleZAdd(const Request& request, Buffer& response) {
    if (request.command.size() < 4 || request.command.size() % 2 != 0) {
        ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Wrong number of arguments for 'zadd'");
        return;
    }

    const std::string& key = request.command[1];
    DataEntry key_entry;
    key_entry.key = key;
    key_entry.hashCode = stringHash(key);

    auto equals = [](HashTable::Node* node, HashTable::Node* key) {
        return static_cast<DataEntry*>(node)->key == static_cast<DataEntry*>(key)->key;
    };

    DataEntry* entry = nullptr;
    if (HashTable::Node* found_node = dataStore.lookup(&key_entry, equals)) {
        entry = static_cast<DataEntry*>(found_node);
        if (!std::holds_alternative<SortedSet>(entry->value)) {
            ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "Operation against a key holding the wrong kind of value");
            return;
        }
    } else {
        auto new_entry = std::make_unique<DataEntry>();
        new_entry->key = key;
        new_entry->value = SortedSet{};
        new_entry->hashCode = key_entry.hashCode;
        entry = new_entry.get();
        dataStore.insert(std::move(new_entry));
    }

    SortedSet &zset = std::get<SortedSet>(entry->value);
    int elements = 0;

    for (size_t i=2; i<request.command.size(); i+=2) {
        const std::string &score_str = request.command[i];
        const std::string &member = request.command[i+1];

        double score;
        try {
            score = std::stod(score_str);
        } catch (const std::invalid_argument &e) {
            ResponseBuilder::outErr(response, ERR_WRONG_ARGS, "value \'" + request.command[i] + "\' is not a valid float");
            return;
        }

        ZSetMemberNode member_key;
        member_key.member = member;
        member_key.hashCode = stringHash(member);
        auto member_equals = [](HashTable::Node* node, HashTable::Node* key) {
            return static_cast<ZSetMemberNode*>(node)->member == static_cast<ZSetMemberNode*>(key)->member;
        };

        if (auto* found_member_node = zset.member_to_score_map.lookup(&member_key, member_equals)) {
            static_cast<ZSetMemberNode*>(found_member_node)->score = score;
        } else {
            auto new_member_node = std::make_unique<ZSetMemberNode>();
            new_member_node->member = member;
            new_member_node->score = score;
            new_member_node->hashCode = member_key.hashCode;
            zset.member_to_score_map.insert(std::move(new_member_node));
        }

        auto new_zset_node = std::make_unique<ZSetNode>();
        new_zset_node->member = member;
        new_zset_node->score = score;

        auto compare_nodes = [](AVLTree::Node* a, AVLTree::Node* b) {
            double score_a = static_cast<ZSetNode*>(a)->score;
            double score_b = static_cast<ZSetNode*>(b)->score;

            if (score_a < score_b) return -1;
            if (score_a > score_b) return  1;

            return static_cast<ZSetNode*>(a)->member.compare(static_cast<ZSetNode*>(b)->member);
        };

        zset.score_sorted_tree.insert(std::move(new_zset_node), compare_nodes);
        ++elements;
    }

    ResponseBuilder::outInt(response, elements);
}

// FNV-1a hash function for strings
uint64_t RedisServer::stringHash(const std::string& str) {
    uint64_t hash = 0xcdf29ce484222325;
    for(char c: str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3;
    }
    return hash;
}

/* ====== Public methods ====== */

RedisServer::RedisServer(uint16_t port) : Server(port) {
    commandTable = {
        {"get",  [this](const Request& req, Buffer& res) { handleGet(req, res);  }},
        {"set",  [this](const Request& req, Buffer& res) { handleSet(req, res);  }},
        {"del",  [this](const Request& req, Buffer& res) { handleDel(req, res);  }},
        {"zadd", [this](const Request& req, Buffer& res) { handleZAdd(req, res); }}, 
        {"keys", [this](const Request& req, Buffer& res) { handleKeys(req, res); }},
        {"ping", [this](const Request& req, Buffer& res) { handlePing(req, res); }},
        {"zrange", [this](const Request& req, Buffer& res) { handleZRange(req, res); }},
    };
}