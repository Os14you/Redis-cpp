#include <Serialization.hpp>

static void appendBuffer(Buffer &out, const void* data, size_t len) {
    const auto* ptr = static_cast<const uint8_t*>(data);
    out.insert(out.end(), ptr, ptr + len);
}

void ResponseBuilder::outNil(Buffer &out) {
    out.push_back(RES_NIL);
}

void ResponseBuilder::outErr(Buffer &out, ErrorType type, const std::string &msg) {
    out.push_back(RES_ERR);

    uint32_t t = static_cast<uint32_t>(type);
    appendBuffer(out, &t, 4);

    uint32_t len = static_cast<uint32_t>(msg.length());
    appendBuffer(out, &len, 4);

    appendBuffer(out, msg.data(), msg.length());
}

void ResponseBuilder::outStr(Buffer &out, const std::string &val) {
    out.push_back(RES_STR);

    uint32_t len = static_cast<uint32_t>(val.length());
    appendBuffer(out, &len, 4);

    appendBuffer(out, val.data(), val.length());
}

void ResponseBuilder::outInt(Buffer &out, const int64_t &val) {
    out.push_back(RES_INT);
    appendBuffer(out, &val, 8);
}

void ResponseBuilder::outArr(Buffer &out, const uint32_t &n) {
    out.push_back(RES_ARR);
    appendBuffer(out, &n, 4);
}