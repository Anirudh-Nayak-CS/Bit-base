#pragma once

#include "../../schema/schema.h"
#include <cstring>
#include <vector>
#include <stdexcept>


inline std::vector<char> serializeRow(const Row& row) {
    std::vector<char> buf;

    // id first
    buf.insert(buf.end(),
               reinterpret_cast<const char*>(&row.id),
               reinterpret_cast<const char*>(&row.id) + sizeof(uint32_t));

    for (const Value& v : row.fields) {
        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;

            if constexpr (std::is_same_v<T, int32_t>) {
                uint8_t tag = static_cast<uint8_t>(DataType::INT32);
                buf.push_back(static_cast<char>(tag));
                buf.insert(buf.end(),
                           reinterpret_cast<const char*>(&val),
                           reinterpret_cast<const char*>(&val) + sizeof(int32_t));

            } else if constexpr (std::is_same_v<T, double>) {
                uint8_t tag = static_cast<uint8_t>(DataType::DOUBLE);
                buf.push_back(static_cast<char>(tag));
                buf.insert(buf.end(),
                           reinterpret_cast<const char*>(&val),
                           reinterpret_cast<const char*>(&val) + sizeof(double));

            } else if constexpr (std::is_same_v<T, bool>) {
                uint8_t tag = static_cast<uint8_t>(DataType::BOOL);
                buf.push_back(static_cast<char>(tag));
                uint8_t b = val ? 1 : 0;
                buf.push_back(static_cast<char>(b));

            } else if constexpr (std::is_same_v<T, std::string>) {
                uint8_t tag = static_cast<uint8_t>(DataType::TEXT);
                buf.push_back(static_cast<char>(tag));
                uint32_t len = static_cast<uint32_t>(val.size());
                buf.insert(buf.end(),
                           reinterpret_cast<const char*>(&len),
                           reinterpret_cast<const char*>(&len) + sizeof(uint32_t));
                buf.insert(buf.end(), val.begin(), val.end());
            }
        }, v);
    }

    return buf;
}

inline Row deserializeRow(const void* src, const Schema& schema) {
    const char* ptr = reinterpret_cast<const char*>(src);
    Row row;

    // read id
    std::memcpy(&row.id, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // read one field per schema column
    for (size_t i = 0; i < schema.columns.size(); ++i) {
        uint8_t tag = static_cast<uint8_t>(*ptr);
        ptr++;

        switch (static_cast<DataType>(tag)) {
            case DataType::INT32: {
                int32_t v;
                std::memcpy(&v, ptr, sizeof(v));
                ptr += sizeof(v);
                row.fields.push_back(v);
                break;
            }
            case DataType::DOUBLE: {
                double v;
                std::memcpy(&v, ptr, sizeof(v));
                ptr += sizeof(v);
                row.fields.push_back(v);
                break;
            }
            case DataType::BOOL: {
                bool v = (*ptr != 0);
                ptr++;
                row.fields.push_back(v);
                break;
            }
            case DataType::TEXT: {
                uint32_t len;
                std::memcpy(&len, ptr, sizeof(len));
                ptr += sizeof(len);
                std::string s(ptr, len);
                ptr += len;
                row.fields.push_back(s);
                break;
            }
            default:
                throw std::runtime_error("deserializeRow: unknown DataType tag");
        }
    }

    return row;
}