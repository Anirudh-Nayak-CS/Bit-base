#include "../../../headers/storage/row/row.h"

void Row::serialize(void* destination) const {
    std::memcpy(reinterpret_cast<char*>(destination) + ID_OFFSET,
                &id, ID_SIZE);

    std::memcpy(reinterpret_cast<char*>(destination) + USERNAME_OFFSET,
                username, USERNAME_SIZE);

    std::memcpy(reinterpret_cast<char*>(destination) + EMAIL_OFFSET,
                email, EMAIL_SIZE);
}

void Row::deserialize(const void* source) {
    std::memcpy(&id,
                reinterpret_cast<const char*>(source) + ID_OFFSET,
                ID_SIZE);

    std::memcpy(username,
                reinterpret_cast<const char*>(source) + USERNAME_OFFSET,
                USERNAME_SIZE);

    std::memcpy(email,
                reinterpret_cast<const char*>(source) + EMAIL_OFFSET,
                EMAIL_SIZE);
}