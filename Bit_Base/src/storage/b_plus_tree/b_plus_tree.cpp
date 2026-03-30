#include "../../../headers/storage/b_plus_tree/b_plus_tree.h"
#include "../../../headers/storage/node/internal_node.h"
#include "../../../headers/storage/node/node_utils.h"
#include "../../../headers/storage/table/table.h"
#include <memory>
#include <cstring>

void B_Plus_Tree::leaf_node_insert(Cursor* cursor, uint32_t key,
                                   const void* data, uint32_t size) {
    void* node = cursor->table->pager->get_page(cursor->page_num);
    uint32_t num_cells = *leafNodeNumCells(node);
 
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, data, size);
        return;
    }
 
    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            std::memcpy(leafNodeCell(node, i),
                        leafNodeCell(node, i - 1),
                        LEAF_NODE_CELL_SIZE);
        }
    }
 
    *(leafNodeNumCells(node)) += 1;
    *leafNodeKey(node, cursor->cell_num) = key;
 
    // zero the slot first so unused bytes are clean, then copy data in
    std::memset(leafNodeValue(node, cursor->cell_num), 0, LEAF_NODE_VALUE_SIZE);
    std::memcpy(leafNodeValue(node, cursor->cell_num), data, size);
}

std::unique_ptr<Cursor>
B_Plus_Tree::leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = table->pager->get_page(page_num);
  uint32_t num_cells = *leafNodeNumCells(node);

  auto cursor = std::make_unique<Cursor>();
  cursor->table = table;
  cursor->page_num = page_num;

  // Binary search to find key
  // with this we get 3 cases -> key already there, position where to insert or
  // at end
  uint32_t min_index = 0;
  uint32_t one_past_max_index = num_cells;
  while (one_past_max_index != min_index) {
    uint32_t index = (min_index + one_past_max_index) / 2;
    uint32_t key_at_index = *leafNodeKey(node, index);
    if (key == key_at_index) {
      cursor->cell_num = index;
      return cursor;
    }
    if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}

void B_Plus_Tree::leaf_node_split_and_insert(Cursor* cursor, uint32_t key,
                                             const void* data, uint32_t size) {
    std::cout << "  [split] page=" << cursor->page_num << " key=" << key << "\n";
 
    void* old_node = cursor->table->pager->get_page(cursor->page_num);
    uint32_t old_max_key = get_node_max_key(cursor->table->pager, old_node);
    uint32_t new_page_num = cursor->table->pager->get_unused_page_num();
    void* new_node = cursor->table->pager->get_page(new_page_num);
    initializeLeafNode(new_node);
    *node_parent(new_node)        = *node_parent(old_node);
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;
 
    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* destination_node;
        uint32_t index_within_node;
 
        if (i >= (int32_t)LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node   = new_node;
            index_within_node  = i - LEAF_NODE_LEFT_SPLIT_COUNT;
        } else {
            destination_node  = old_node;
            index_within_node = i;
        }
 
        void* destination = leafNodeCell(destination_node, index_within_node);
 
        if (i == (int32_t)cursor->cell_num) {
            // write new key + new data into slot
            *leafNodeKey(destination_node, index_within_node) = key;
            std::memset(leafNodeValue(destination_node, index_within_node), 0,
                        LEAF_NODE_VALUE_SIZE);
            std::memcpy(leafNodeValue(destination_node, index_within_node),
                        data, size);
        } else if (i > (int32_t)cursor->cell_num) {
            std::memcpy(destination, leafNodeCell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            std::memcpy(destination, leafNodeCell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }
 
    *(leafNodeNumCells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *(leafNodeNumCells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
 
    if (is_node_root(old_node)) {
        return create_new_root(cursor->table, new_page_num);
    } else {
        uint32_t parent_page_num = *node_parent(old_node);
        uint32_t new_key = get_node_max_key(cursor->table->pager, old_node);
        void* parent = cursor->table->pager->get_page(parent_page_num);
        update_internal_node_key(parent, old_max_key, new_key);
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
    }
}
 

void B_Plus_Tree::create_new_root(Table *table, uint32_t right_child_page_num) {
     std::cout << "  [create_new_root] root_page=" << table->root_page_num 
            << " right_child_page=" << right_child_page_num << "\n";
  void *root = table->pager->get_page(table->root_page_num);
  void *right_child = table->pager->get_page(right_child_page_num);

  uint32_t left_child_page_num = table->pager->get_unused_page_num();
  void *left_child = table->pager->get_page(left_child_page_num);

  if (get_node_type(root) == NodeType::INTERNAL) {
    initialize_internal_node(right_child);
    initialize_internal_node(left_child);
  }

  *(node_parent(left_child)) = table->root_page_num;
  *(node_parent(right_child)) = table->root_page_num;
  std::memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);
  *(node_parent(left_child)) = table->root_page_num;

  if (get_node_type(left_child) == NodeType::INTERNAL) {
    void *child;
    for (int i = 0; i < *internal_node_num_keys(left_child); i++) {
      child = table->pager->get_page(*internal_node_child(left_child, i));
      *node_parent(child) = left_child_page_num;
    }
    child = table->pager->get_page(*internal_node_right_child(left_child));
    *node_parent(child) = left_child_page_num;
  }

  initialize_internal_node(root);
  set_node_root(root, true);

  *internal_node_num_keys(root) = 1;
  *internal_node_child(root, 0) = left_child_page_num;

  uint32_t left_child_max_key = get_node_max_key(table->pager, left_child);
  *internal_node_key(root, 0) = left_child_max_key;

  *internal_node_right_child(root) = right_child_page_num;

 
}

inline void indent(uint32_t level) {
  for (uint32_t i = 0; i < level; i++) {
    std::cout << "  ";
  }
}

void B_Plus_Tree::print_tree(Pager *pager, uint32_t page_num,
                             uint32_t indentation_level) {
  void *node = pager->get_page(page_num);
  uint32_t num_keys, child;

  switch (get_node_type(node)) {
  case NodeType::LEAF: {
    num_keys = *leafNodeNumCells(node);
    indent(indentation_level);
    std::cout << "- leaf (size " << num_keys << ")\n";

    for (uint32_t i = 0; i < num_keys; i++) {
      indent(indentation_level + 1);
      std::cout << "- " << *leafNodeKey(node, i) << "\n";
    }
    break;
  }

  case NodeType::INTERNAL: {
    num_keys = *internal_node_num_keys(node);
    indent(indentation_level);
    std::cout << "- internal (size " << num_keys << ")\n";

    if (num_keys > 0) {
      for (uint32_t i = 0; i < num_keys; i++) {
        child = *internal_node_child(node, i);
        print_tree(pager, child, indentation_level + 1);

        indent(indentation_level + 1);
        printf("- key %u\n", *internal_node_key(node, i));
      }
      child = *internal_node_right_child(node);
      print_tree(pager, child, indentation_level + 1);
    }

    break;
  }
  }
}

uint32_t B_Plus_Tree::internal_node_find_child(void *node, uint32_t key) {
  uint32_t num_keys = *internal_node_num_keys(node);

  uint32_t min_index = 0;
  uint32_t max_index = num_keys;

  while (min_index != max_index) {
    uint32_t index = (min_index + max_index) / 2;
    uint32_t key_to_right = *internal_node_key(node, index);

    if (key_to_right >= key) {
      max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  return min_index;
}

std::unique_ptr<Cursor>
B_Plus_Tree::internal_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = table->pager->get_page(page_num);

  uint32_t child_index = internal_node_find_child(node, key);
  uint32_t child_page_num;

  uint32_t num_keys = *internal_node_num_keys(node);

  if (child_index == num_keys) {
    child_page_num = *internal_node_right_child(node);
  } else {
    child_page_num = *internal_node_child(node, child_index);
  }

  void *child = table->pager->get_page(child_page_num);

  switch (get_node_type(child)) {
  case NodeType::LEAF:
    return leaf_node_find(table, child_page_num, key);

  case NodeType::INTERNAL:
    return internal_node_find(table, child_page_num, key);
  }

  std::cerr << "Unknown node type\n";
  exit(EXIT_FAILURE);
}

void B_Plus_Tree::internal_node_split_and_insert(Table *table,
                                                 uint32_t parent_page_num,
                                                 uint32_t child_page_num) {
  void *old_node = table->pager->get_page(parent_page_num);
  uint32_t old_max = get_node_max_key(table->pager, old_node);

  void *child = table->pager->get_page(child_page_num);
  uint32_t child_max = get_node_max_key(table->pager, child);

  uint32_t new_page_num = table->pager->get_unused_page_num();
  void *new_node = table->pager->get_page(new_page_num);
  initialize_internal_node(new_node);

  uint32_t splitting_root = is_node_root(old_node);

  void *parent;
  if (splitting_root) {
    create_new_root(table, new_page_num);
    parent = table->pager->get_page(table->root_page_num);
    uint32_t left_child_page = *internal_node_child(parent, 0);
    old_node = table->pager->get_page(left_child_page);
  } else {
    parent = table->pager->get_page(*node_parent(old_node));
  }

  uint32_t *old_num_keys = internal_node_num_keys(old_node);
  uint32_t total_keys = *old_num_keys;
  int split = total_keys / 2;

  uint32_t old_right = *internal_node_right_child(old_node);
  *internal_node_right_child(new_node) = old_right;

  if (old_right != INVALID_PAGE_NUM) {
    void *old_right_node = table->pager->get_page(old_right);
    *node_parent(old_right_node) = new_page_num;
  }

  for (int i = total_keys - 1; i > split; i--) {
    uint32_t child_page = *internal_node_child(old_node, i);
    void *child_node = table->pager->get_page(child_page);

    *internal_node_child(new_node, i - split - 1) = child_page;
  
    *internal_node_key(new_node, i - split - 1) =
        *internal_node_key(old_node, i);

    *node_parent(child_node) = new_page_num;
  }

 
  uint32_t new_right_page = *internal_node_child(old_node, split);
  *internal_node_right_child(old_node) = new_right_page;
  void *new_right_node = table->pager->get_page(new_right_page);
  *node_parent(new_right_node) = parent_page_num;

  *old_num_keys = split;
  *internal_node_num_keys(new_node) = total_keys - split - 1;

  uint32_t max_after_split = get_node_max_key(table->pager, old_node);
  uint32_t destination_page =
      (child_max < max_after_split) ? parent_page_num : new_page_num;

  internal_node_insert(table, destination_page, child_page_num);
  *node_parent(child) = destination_page;

  update_internal_node_key(parent, old_max,
                           get_node_max_key(table->pager, old_node));

  if (!splitting_root) {
    internal_node_insert(table, *node_parent(old_node), new_page_num);
    *node_parent(new_node) = *node_parent(old_node);
  }
}


void B_Plus_Tree::update_internal_node_key(void *node, uint32_t old_key,
                                           uint32_t new_key) {
  uint32_t old_child_index = internal_node_find_child(node, old_key);
 
  uint32_t num_keys = *internal_node_num_keys(node);
  if (old_child_index < num_keys) {
    *internal_node_key(node, old_child_index) = new_key;
  }

}

void B_Plus_Tree::internal_node_insert(Table *table, uint32_t parent_page_num,
                                       uint32_t child_page_num) {
  void *parent = table->pager->get_page(parent_page_num);
  void *child = table->pager->get_page(child_page_num);


  uint32_t child_max_key = get_node_max_key(table->pager, child);

  uint32_t index = internal_node_find_child(parent, child_max_key);
  uint32_t original_num_keys = *internal_node_num_keys(parent);

  if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
    internal_node_split_and_insert(table, parent_page_num, child_page_num);
    return;
  }

  uint32_t right_child_page_num = *internal_node_right_child(parent);

  if (right_child_page_num == INVALID_PAGE_NUM) {
    *internal_node_right_child(parent) = child_page_num;
    return;
  }

  void *right_child = table->pager->get_page(right_child_page_num);
  *internal_node_num_keys(parent) = original_num_keys + 1;

  
  uint32_t right_key = get_node_max_key(table->pager, right_child);

  if (child_max_key > right_key) {
    *internal_node_child(parent, original_num_keys) = right_child_page_num;
    *internal_node_key(parent, original_num_keys) = right_key;
    *internal_node_right_child(parent) = child_page_num;
  } else {
    for (uint32_t i = original_num_keys; i > index; i--) {
      *internal_node_child(parent, i) = *internal_node_child(parent, i - 1);
      *internal_node_key(parent, i) = *internal_node_key(parent, i - 1);
    }
    *internal_node_child(parent, index) = child_page_num;
    *internal_node_key(parent, index) = child_max_key;
  }
}