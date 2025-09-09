#include <common/Deserialization.hpp>

void printIndent(const int &lvl) {
    for (int i=0; i<lvl; ++i) {
        std::cout << " ";
    }
}

size_t printResponse(const Buffer &res, size_t offset, int indent) {
    if (offset >= res.size()) {
        std::cerr << "Error: offset out of bound" << std::endl;
        return 0;
    }

    const size_t init_offset = offset;
    uint8_t type = res[offset++];
    size_t cur_pos = offset;

    printIndent(indent);

    try {
        switch (type) {
            case 0: // Nil
                std::cout << "(nil)" << std::endl;
                break;
            case 1: { // Err
                uint32_t code = readAs<uint32_t>(res, cur_pos);
                uint32_t len = readAs<uint32_t>(res, cur_pos);
                if (cur_pos + len > res.size()) return 0;
                std::cout << "(err) code " << code << ": "
                          << std::string(res.begin() + cur_pos, res.begin() + cur_pos + len) << std::endl;
                cur_pos += len;
                break;
            }
            case 2: { // STRING
                uint32_t len = readAs<uint32_t>(res, cur_pos);
                if (cur_pos + len > res.size()) return 0;
                std::cout << "\"" << std::string(res.begin() + cur_pos, res.begin() + cur_pos + len) << "\"" << std::endl;
                cur_pos += len;
                break;
            }
            case 3: { // INT
                int64_t val = readAs<int64_t>(res, cur_pos);
                std::cout << "(integer) " << val << std::endl;
                break;
            }
            case 4: { // ARRAY
                uint32_t count = readAs<uint32_t>(res, cur_pos);
                std::cout << "(arr) " << count << " elements:" << std::endl;

                for (uint32_t i = 0; i < count; ++i) {
                    size_t consumed = printResponse(res, cur_pos, indent + 1);
                    if (consumed == 0) return 0;
                    cur_pos += consumed;
                }
                break;
            }
            default:
                std::cout << "Unkown response type: " << (int)type << std::endl;
                return 0;
        }
    } catch (const std::out_of_range &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;
    }

    return cur_pos - init_offset;
}