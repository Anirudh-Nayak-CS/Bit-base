// #pragma once
// #include <iostream>
// #include <vector>
// #include <cstdint>
// #include <optional>

// // Forward declaration
// class Pager;

// template <typename KeyType>
// class BPlusTreeNode {
// public:
//     bool isLeaf;
    
   
//     uint32_t page_id;
//     uint32_t parent_page_id; 

//     // The keys 
//     std::vector<KeyType> keys;

//     //Internal nodes -> Disk pointers to children
//     std::vector<uint32_t> child_page_ids; 

//     //leaf nodes -> Serialized bytes of the whole row
//     std::vector<std::vector<uint8_t>> row_data; 

//     // leaf linkage -> Disk pointer to the right sibling for fast range scans
//     uint32_t next_page_id; 

//     BPlusTreeNode(bool leaf) : isLeaf(leaf), page_id(0), parent_page_id(0), next_page_id(0) {}
// };


// template <typename KeyType>
// class BPlusTree {
// private:
//     // ID of the root page. (0 means the tree is completely empty)
//     uint32_t root_page_id;  
    
//     // The Pager dependency to fetch/save pages to disk/RAM.
//     Pager* pager; 
    
//     int maxKeys;

   
//     uint32_t searchInternal(const KeyType &key, uint32_t node_page_id);

//     void insertInternal(const KeyType &key, 
//                         const std::vector<uint8_t>& row_data,
//                         uint32_t node_page_id);

//     void splitNode(uint32_t node_page_id);

//     // Deletion helpers
//     void handleUnderflow(uint32_t node_page_id);
    
//     void borrowFromSibling(uint32_t node_page_id,
//                            uint32_t sibling_page_id,
//                            uint32_t parent_page_id,
//                            bool borrow_from_left);

//     void mergeNodes(uint32_t left_page_id,
//                     uint32_t right_page_id,
//                     uint32_t parent_page_id);

// public:
   
//     BPlusTree(Pager* pager_instance, int maxKeysPerNode);

//     // Core CRUD Operations
//     void insertEntry(const KeyType &key, const std::vector<uint8_t>& row_data);

//     void deleteEntry(const KeyType &key);

//     std::optional<std::vector<uint8_t>> searchEntry(const KeyType &key);

   
//     bool updateData(const KeyType &key, const std::vector<uint8_t>& new_row_data);


// };