// #include "../../headers/database/b_plus_tree.h"

// #include <algorithm>

// #include <vector>

// #include <cstdint>

// #include <algorithm>

// #include <optional>

// // Constructor
// template < typename KeyType >
//   BPlusTree < KeyType > ::BPlusTree(Pager * pager_instance, int maxKeysPerNode): pager(pager_instance), maxKeys(maxKeysPerNode), root_page_id(0) {

//   }

// // Internal Search Logic (Returns the Leaf's Page ID)
// template < typename KeyType >
//   uint32_t BPlusTree < KeyType > ::searchInternal(const KeyType & key, uint32_t current_page_id) {

//     // FETCH FROM DISK/RAM
//     BPlusTreeNode < KeyType > * node = pager -> fetch_page(current_page_id);

//     while (!node -> isLeaf) {
//       auto it = std::upper_bound(node -> keys.begin(), node -> keys.end(), key);
//       int index = std::distance(node -> keys.begin(), it);

//       // Move to the child page
//       uint32_t next_page_id = node -> child_page_ids[index];

//       // FETCH THE NEXT PAGE
//       node = pager->fetch_page(next_page_id);
//     }

//     // Node is now a leaf. We return its Page ID (not the pointer!)
//     // We have to ask the pager for its ID because we only have the raw pointer right now.
//     return node -> page_id;
//   }

// // Public Search Entry
// template < typename KeyType >
//   std::optional < std::vector < uint8_t >> BPlusTree < KeyType > ::searchEntry(const KeyType & key) {
//     if (root_page_id == 0) return std::nullopt; // Tree is empty

//     uint32_t leaf_id = searchInternal(key, root_page_id);
//     BPlusTreeNode < KeyType > * leaf = pager -> fetch_page(leaf_id);

//     auto it = std::lower_bound(leaf -> keys.begin(), leaf -> keys.end(), key);

//     if (it != leaf -> keys.end() && * it == key) {
//       int index = std::distance(leaf -> keys.begin(), it);
//       return leaf -> row_data[index]; // Return the serialized row!
//     }

//     return std::nullopt;
//   }
// // Insert a key-row pair
// template < typename KeyType >
//   void BPlusTree < KeyType > ::insertEntry(const KeyType & key,
//     const std::vector < uint8_t > & row_data) {
//     // 1. If tree is completely empty, allocate the first page
//     if (root_page_id == 0) {
//       root_page_id = pager -> allocate_page(); // Ask OS for a new 4KB block
//       BPlusTreeNode < KeyType > * root_node = pager -> fetch_page(root_page_id);

//       root_node -> isLeaf = true;
//       root_node -> keys.push_back(key);
//       root_node -> row_data.push_back(row_data);

//       pager -> mark_dirty(root_page_id); // SAVE IT
//       return;
//     }

//     insertInternal(key, row_data, root_page_id);
//   }

// // Internal insert logic
// template < typename KeyType >
//   void BPlusTree < KeyType > ::insertInternal(const KeyType & key,
//     const std::vector < uint8_t > & row_data, uint32_t node_id) {

//     BPlusTreeNode < KeyType > * node = pager -> fetch_page(node_id);

//     if (node -> isLeaf) {
//       auto it = std::lower_bound(node -> keys.begin(), node -> keys.end(), key);
//       int index = std::distance(node -> keys.begin(), it);

//       if (it != node -> keys.end() && * it == key) {
//         // UNIQUE KEY RULE: Update the existing row instead of adding duplicates
//         node -> row_data[index] = row_data;
//         pager -> mark_dirty(node_id);
//       } else {
//         // Key does not exist, insert key and row
//         node -> keys.insert(it, key);
//         node -> row_data.insert(node -> row_data.begin() + index, row_data);
//         pager -> mark_dirty(node_id);

//         if (node -> keys.size() > maxKeys) {
//           splitNode(node_id);
//         }
//       }
//     } else {
//       auto it = std::upper_bound(node -> keys.begin(), node -> keys.end(), key);
//       int index = std::distance(node -> keys.begin(), it);

//       uint32_t child_id = node -> child_page_ids[index];
//       insertInternal(key, row_data, child_id);
//     }
//   }

// // Split Node
// template < typename KeyType >
//   void BPlusTree < KeyType > ::splitNode(uint32_t node_id) {

//     BPlusTreeNode < KeyType > * node = pager -> fetch_page(node_id);
//     int midIndex = node -> keys.size() / 2;

//     // Create a new page on disk
//     uint32_t new_page_id = pager -> allocate_page();
//     BPlusTreeNode < KeyType > * newNode = pager -> fetch_page(new_page_id);
//     newNode -> isLeaf = node -> isLeaf;

//     KeyType upKey;

//     if (node -> isLeaf) {
//       // LEAF SPLIT
//       newNode -> keys.assign(node -> keys.begin() + midIndex, node -> keys.end());
//       node -> keys.resize(midIndex);

//       newNode -> row_data.assign(node -> row_data.begin() + midIndex, node -> row_data.end());
//       node -> row_data.resize(midIndex);

//       upKey = newNode -> keys.front();

//       // Link the leaves together
//       newNode -> next_page_id = node -> next_page_id;
//       node -> next_page_id = new_page_id;

//     } else {
//       // INTERNAL SPLIT
//       upKey = node -> keys[midIndex];

//       newNode -> keys.assign(node -> keys.begin() + midIndex + 1, node -> keys.end());
//       node -> keys.resize(midIndex);

//       newNode -> child_page_ids.assign(node -> child_page_ids.begin() + midIndex + 1, node -> child_page_ids.end());
//       node -> child_page_ids.resize(midIndex + 1);

//       // Update parent IDs of the children we just moved
//       for (uint32_t child_id: newNode -> child_page_ids) {
//         BPlusTreeNode < KeyType > * child = pager -> fetch_page(child_id);
//         child -> parent_page_id = new_page_id;
//         pager -> mark_dirty(child_id);
//       }
//     }

//     // Mark both split nodes as changed
//     pager -> mark_dirty(node_id);
//     pager -> mark_dirty(new_page_id);

//     // Handle Root Split
//     if (node_id == root_page_id) {
//       uint32_t new_root_id = pager -> allocate_page();
//       BPlusTreeNode < KeyType > * newRoot = pager -> fetch_page(new_root_id);

//       newRoot -> isLeaf = false;
//       newRoot -> keys.push_back(upKey);
//       newRoot -> child_page_ids.push_back(node_id);
//       newRoot -> child_page_ids.push_back(new_page_id);

//       node -> parent_page_id = new_root_id;
//       newNode -> parent_page_id = new_root_id;

//       root_page_id = new_root_id; // Update the tree's root tracker

//       pager -> mark_dirty(new_root_id);
//       pager -> mark_dirty(node_id);
//       pager -> mark_dirty(new_page_id);
//     } else {
//       // Update existing parent node
//       uint32_t parent_id = node -> parent_page_id;
//       BPlusTreeNode < KeyType > * parent = pager -> fetch_page(parent_id);

//       auto it = std::upper_bound(parent -> keys.begin(), parent -> keys.end(), upKey);
//       int index = std::distance(parent -> keys.begin(), it);

//       parent -> keys.insert(parent -> keys.begin() + index, upKey);
//       parent -> child_page_ids.insert(parent -> child_page_ids.begin() + index + 1, new_page_id);

//       newNode -> parent_page_id = parent_id;

//       pager -> mark_dirty(parent_id);
//       pager -> mark_dirty(new_page_id);

//       if (parent -> keys.size() > maxKeys) {
//         splitNode(parent_id);
//       }
//     }
//   }

// // 1. DELETE ENTRY (Unique Key)
// template < typename KeyType >
//   void BPlusTree < KeyType > ::deleteEntry(const KeyType & key) {
//     if (root_page_id == 0) return; // Empty tree

//     // 1. Find the leaf node
//     uint32_t leaf_id = searchInternal(key, root_page_id);
//     BPlusTreeNode < KeyType > * leaf = pager -> fetch_page(leaf_id);

//     // 2. Find the key
//     auto it = std::lower_bound(leaf -> keys.begin(), leaf -> keys.end(), key);
//     if (it == leaf -> keys.end() || * it != key) {
//       return; // Key doesn't exist, nothing to delete
//     }

//     int index = std::distance(leaf -> keys.begin(), it);

//     // 3. Erase the key and the row data
//     leaf -> keys.erase(it);
//     leaf -> row_data.erase(leaf -> row_data.begin() + index);
//     pager -> mark_dirty(leaf_id); // SAVE IT

//     // 4. Handle Underflow
//     int minKeys = leaf -> isLeaf ? (maxKeys) / 2 : (maxKeys - 1) / 2;

//     if (leaf_id == root_page_id) {
//       // Root is allowed to be empty unless it's an internal node with 1 child
//       if (leaf -> keys.empty() && !leaf -> isLeaf) {
//         root_page_id = leaf -> child_page_ids[0];
//         BPlusTreeNode < KeyType > * new_root = pager -> fetch_page(root_page_id);
//         new_root -> parent_page_id = 0; // It has no parent now

//         pager -> mark_dirty(root_page_id);
//         pager -> free_page(leaf_id); // The old root page is deleted from disk!
//       }
//       return;
//     }

//     if (leaf -> keys.size() < minKeys) {
//       handleUnderflow(leaf_id);
//     }
//   }

// // 2. HANDLE UNDERFLOW (The router for Borrow vs Merge)
// template < typename KeyType >
//   void BPlusTree < KeyType > ::handleUnderflow(uint32_t node_id) {
//     BPlusTreeNode < KeyType > * node = pager -> fetch_page(node_id);
//     uint32_t parent_id = node -> parent_page_id;
//     BPlusTreeNode < KeyType > * parent = pager -> fetch_page(parent_id);

//     // Find this node's position in the parent
//     auto it = std::find(parent -> child_page_ids.begin(), parent -> child_page_ids.end(), node_id);
//     int index = std::distance(parent -> child_page_ids.begin(), it);

//     uint32_t left_sibling_id = (index > 0) ? parent -> child_page_ids[index - 1] : 0;
//     uint32_t right_sibling_id = (index < parent -> child_page_ids.size() - 1) ? parent -> child_page_ids[index + 1] : 0;

//     int minKeys = node -> isLeaf ? (maxKeys) / 2 : (maxKeys - 1) / 2;

//     // Try to borrow from left
//     if (left_sibling_id != 0) {
//       BPlusTreeNode < KeyType > * left = pager -> fetch_page(left_sibling_id);
//       if (left -> keys.size() > minKeys) {
//         borrowFromSibling(node_id, left_sibling_id, parent_id, true);
//         return;
//       }
//     }

//     // Try to borrow from right
//     if (right_sibling_id != 0) {
//       BPlusTreeNode < KeyType > * right = pager -> fetch_page(right_sibling_id);
//       if (right -> keys.size() > minKeys) {
//         borrowFromSibling(node_id, right_sibling_id, parent_id, false);
//         return;
//       }
//     }

//     // If we can't borrow, we MUST merge. 
//     // Always merge into the left node to keep logic simple.
//     if (left_sibling_id != 0) {
//       mergeNodes(left_sibling_id, node_id, parent_id);
//     } else {
//       mergeNodes(node_id, right_sibling_id, parent_id);
//     }
//   }

// // 3. MERGE NODES (Crushing two pages into one)
// template < typename KeyType >
//   void BPlusTree < KeyType > ::mergeNodes(uint32_t left_id, uint32_t right_id, uint32_t parent_id) {
//     BPlusTreeNode < KeyType > * left = pager -> fetch_page(left_id);
//     BPlusTreeNode < KeyType > * right = pager -> fetch_page(right_id);
//     BPlusTreeNode < KeyType > * parent = pager -> fetch_page(parent_id);

//     // Find where the right node is in the parent
//     auto it = std::find(parent -> child_page_ids.begin(), parent -> child_page_ids.end(), right_id);
//     int parent_index = std::distance(parent -> child_page_ids.begin(), it) - 1;

//     // Move everything from right to left
//     if (!left -> isLeaf) {
//       left -> keys.push_back(parent -> keys[parent_index]); // Pull down the routing key
//     }

//     left -> keys.insert(left -> keys.end(), right -> keys.begin(), right -> keys.end());

//     if (left -> isLeaf) {
//       left -> row_data.insert(left -> row_data.end(), right -> row_data.begin(), right -> row_data.end());
//       left -> next_page_id = right -> next_page_id; // Fix the linked list!
//     } else {
//       left -> child_page_ids.insert(left -> child_page_ids.end(), right -> child_page_ids.begin(), right -> child_page_ids.end());
//       // Update parents for all moved children
//       for (uint32_t child_id: right -> child_page_ids) {
//         BPlusTreeNode < KeyType > * child = pager -> fetch_page(child_id);
//         child -> parent_page_id = left_id;
//         pager -> mark_dirty(child_id);
//       }
//     }

//     // Remove the routing key and right child from the parent
//     parent -> keys.erase(parent -> keys.begin() + parent_index);
//     parent -> child_page_ids.erase(parent -> child_page_ids.begin() + parent_index + 1);

//     // MARK EVERYTHING DIRTY AND FREE THE DEAD PAGE
//     pager -> mark_dirty(left_id);
//     pager -> mark_dirty(parent_id);
//     pager -> free_page(right_id); // The disk reclaims this 4KB block!

//     // Check if the parent now underflows
//     int minKeys = (maxKeys - 1) / 2;
//     if (parent_id == root_page_id && parent -> keys.empty()) {
//       root_page_id = left_id;
//       left -> parent_page_id = 0;
//       pager -> mark_dirty(left_id);
//       pager -> free_page(parent_id);
//     } else if (parent_id != root_page_id && parent -> keys.size() < minKeys) {
//       handleUnderflow(parent_id);
//     }
//   }

// template < typename KeyType >
//   bool BPlusTree < KeyType > ::updateData(const KeyType & key,
//     const std::vector < uint8_t > & new_row_data) {
//     if (root_page_id == 0) return false; // Tree is empty

//     // 1. Find the leaf node where this key lives
//     uint32_t leaf_id = searchInternal(key, root_page_id);
//     BPlusTreeNode < KeyType > * leaf = pager -> fetch_page(leaf_id);

//     // 2. Look for the exact key
//     auto it = std::lower_bound(leaf -> keys.begin(), leaf -> keys.end(), key);

//     // 3. If found, overwrite the bytes!
//     if (it != leaf -> keys.end() && * it == key) {
//       int index = std::distance(leaf -> keys.begin(), it);

//       leaf -> row_data[index] = new_row_data; // Replace the old row with the new one
//       pager -> mark_dirty(leaf_id); // Tell the Pager to save to disk

//       return true; // Success
//     }

//     return false; // Key not found
//   }

// template < typename KeyType >
//   void BPlusTree < KeyType > ::borrowFromSibling(uint32_t node_id, uint32_t sibling_id, uint32_t parent_id, bool borrow_from_left) {
//     BPlusTreeNode < KeyType > * node = pager -> fetch_page(node_id);
//     BPlusTreeNode < KeyType > * sibling = pager -> fetch_page(sibling_id);
//     BPlusTreeNode < KeyType > * parent = pager -> fetch_page(parent_id);

//     // Find sibling's position in the parent
//     auto it = std::find(parent -> child_page_ids.begin(), parent -> child_page_ids.end(), sibling_id);
//     int sibling_index = std::distance(parent -> child_page_ids.begin(), it);

//     if (borrow_from_left) {
//       // === BORROW FROM LEFT SIBLING ===
//       int parent_key_index = sibling_index; // The key separating left sibling and node

//       if (node -> isLeaf) {
//         node -> keys.insert(node -> keys.begin(), sibling -> keys.back());
//         node -> row_data.insert(node -> row_data.begin(), sibling -> row_data.back());
//         sibling -> keys.pop_back();
//         sibling -> row_data.pop_back();
//         parent -> keys[parent_key_index] = node -> keys.front();
//       } else {
//         node -> keys.insert(node -> keys.begin(), parent -> keys[parent_key_index]);
//         parent -> keys[parent_key_index] = sibling -> keys.back();
//         sibling -> keys.pop_back();

//         // Steal the child page pointer
//         uint32_t stolen_child = sibling -> child_page_ids.back();
//         node -> child_page_ids.insert(node -> child_page_ids.begin(), stolen_child);
//         sibling -> child_page_ids.pop_back();

//         // Update the stolen child's parent pointer
//         BPlusTreeNode < KeyType > * child = pager -> fetch_page(stolen_child);
//         child -> parent_page_id = node_id;
//         pager -> mark_dirty(stolen_child);
//       }
//     } else {
//       // === BORROW FROM RIGHT SIBLING ===
//       int parent_key_index = sibling_index - 1; // The key separating node and right sibling

//       if (node -> isLeaf) {
//         node -> keys.push_back(sibling -> keys.front());
//         node -> row_data.push_back(sibling -> row_data.front());
//         sibling -> keys.erase(sibling -> keys.begin());
//         sibling -> row_data.erase(sibling -> row_data.begin());
//         parent -> keys[parent_key_index] = sibling -> keys.front();
//       } else {
//         node -> keys.push_back(parent -> keys[parent_key_index]);
//         parent -> keys[parent_key_index] = sibling -> keys.front();
//         sibling -> keys.erase(sibling -> keys.begin());

//         // Steal the child page pointer
//         uint32_t stolen_child = sibling -> child_page_ids.front();
//         node -> child_page_ids.push_back(stolen_child);
//         sibling -> child_page_ids.erase(sibling -> child_page_ids.begin());

//         // Update the stolen child's parent pointer
//         BPlusTreeNode < KeyType > * child = pager -> fetch_page(stolen_child);
//         child -> parent_page_id = node_id;
//         pager -> mark_dirty(stolen_child);
//       }
//     }

//     // Mark all affected pages as dirty
//     pager -> mark_dirty(node_id);
//     pager -> mark_dirty(sibling_id);
//     pager -> mark_dirty(parent_id);
//   }