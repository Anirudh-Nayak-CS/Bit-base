#include "../../../headers/storage/transaction/transaction.h"
#include <cstring>

 
//  pin_page 
// Saves a full before-image of the page the very first time it is dirtied

 void Transaction::pin_page(uint32_t page_num,
                           const void* page_data,
                           size_t page_size) {
    if (has_snapshot(page_num)) return;

    std::vector<uint8_t> buffer(page_size);
    std::memcpy(buffer.data(), page_data, page_size);

    snapshots_.emplace(page_num, std::move(buffer));
}

bool Transaction::has_snapshot(uint32_t page_num) const {
    return snapshots_.find(page_num) != snapshots_.end();
}
 