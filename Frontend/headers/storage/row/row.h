#pragma once

#include <cstdint>
#include <cstring>

constexpr uint32_t ID_SIZE = sizeof(uint32_t);
constexpr uint32_t USERNAME_SIZE = 32;
constexpr uint32_t EMAIL_SIZE = 255;

constexpr uint32_t ID_OFFSET = 0;
constexpr uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
constexpr uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

constexpr uint32_t ROW_SIZE =
    ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

class Row {
public:
    uint32_t id;
    char username[USERNAME_SIZE];
    char email[EMAIL_SIZE];

    void serialize(void* destination) const;
    void deserialize(const void* source);
};