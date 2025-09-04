#pragma once

#include <vector>
#include <cstdint>
#include <string>

typedef std::vector<uint8_t> Buffer;

enum ResponseType {
    RES_NIL = 0,
    RES_ERR = 1,
    RES_STR = 2,
    RES_INT = 3,
    RES_ARR = 4,
};

enum ErrorType {
    ERR_UNKNOWN_COMMAND = 0,
    ERR_WRONG_ARGS = 1,
    ERR_PROTOCOL = 2,
};

struct ResponseBuilder {
    static void outNil(Buffer &out);
    static void outErr(Buffer &out, ErrorType type, const std::string &msg);
    static void outStr(Buffer &out, const std::string &val);
    static void outInt(Buffer &out, const int64_t &val);
    static void outArr(Buffer &out, const uint32_t &n);
};