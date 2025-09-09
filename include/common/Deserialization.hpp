#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <algorithm>

typedef std::vector<uint8_t> Buffer;

size_t printResponse(const Buffer &res, size_t offset, int indent = 0);

void printIndent(const int &lvl);

template <typename T>
T readAs(const Buffer &buffer, size_t& offset) {
    if(offset + sizeof(T) > buffer.size()) {
        throw std::out_of_range("Attempting to read post the end of the buffer.");
    }

    T value;
    std::copy (
        buffer.data() + offset,
        buffer.data() + offset + sizeof(T),
        reinterpret_cast<uint8_t*>(&value)
    );

    offset += sizeof(T);
    return value;
}