#include "../../headers/storage/b_plus_tree/b_plus_tree.h"

void B_Plus_Tree::leaf_node_insert(Cursor* cursor, uint32_t key, Row* value)  {
    void* node = cursor->table->pager->get_page(cursor->page_num);

    uint32_t num_cells = *leafNodeNumCells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
       leaf_node_split_and_insert(cursor, key, value);
       return;
    }

    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            std::memcpy(
                leafNodeCell(node, i),
                leafNodeCell(node, i - 1),
                LEAF_NODE_CELL_SIZE
            );
        }
    }

    *(leafNodeNumCells(node))+=1;

    *leafNodeKey(node, cursor->cell_num) = key;

    value->serialize(leafNodeValue(node, cursor->cell_num));
}


 Cursor* B_Plus_Tree::leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
   void* node = table->pager->get_page(page_num);
   uint32_t num_cells = *leafNodeNumCells(node);
 
   Cursor* cursor = new Cursor();
   cursor->table = table;
   cursor->page_num = page_num;

   // Binary search to find key 
   // with this we get 3 cases -> key already there, position where to insert or at end
   uint32_t min_index = 0;
   uint32_t one_past_max_index = num_cells;
   while (one_past_max_index != min_index) {
     uint32_t index = (min_index +one_past_max_index) / 2;
     uint32_t key_at_index = *leafNodeKey(node, index);
     if (key == key_at_index) {
       cursor->cell_num = index;
      return cursor;
     }
     if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index+1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}

 void B_Plus_Tree::leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
  /*
  Create a new node and move half the cells over.
  Insert the new value in one of the two nodes.
  Update parent or create a new parent.
  */

  void* old_node = cursor->table->pager->get_page(cursor->page_num);
  uint32_t new_page_num = cursor->table->pager->get_unused_page_num();
  void* new_node = cursor->table->pager->get_page(new_page_num);
  initializeLeafNode(new_node);

    /*
  All existing keys plus new key should be divided
  evenly between old (left) and new (right) nodes.
  Starting from the right, move each key to correct position.
  */

  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void* destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
      destination_node = new_node;
    } else {
      destination_node = old_node;
    }
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void* destination = leafNodeCell(destination_node, index_within_node);

    if (i == cursor->cell_num) {
      value->serialize(destination);
    } else if (i > cursor->cell_num) {
      memcpy(destination, leafNodeCell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    } else {
      memcpy(destination, leafNodeCell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }
    /* Update cell count on both leaf nodes */
  *(leafNodeNumCells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leafNodeNumCells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
  if (is_node_root(old_node)) {
    return create_new_root(cursor->table, new_page_num);
  } else {
    printf("Need to implement updating parent after split\n");
    exit(EXIT_FAILURE);
  }

}

void B_Plus_Tree::create_new_root(Table* table, uint32_t right_child_page_num) {
  /*
  Handle splitting the root.
  Old root copied to new page, becomes left child.
  Address of right child passed in.
  Re-initialize root page to contain the new root node.
  New root node points to two children.
  */

  void* root = table->pager->get_page(table->root_page_num);
  void* right_child = table->pager->get_page(right_child_page_num);

  uint32_t left_child_page_num = table->pager->get_unused_page_num();
  void* left_child = table->pager->get_page(left_child_page_num);

  /* Left child has data copied from old root */
  std::memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  /* Root node is a new internal node with one key and two children */
  initialize_internal_node(root);
  set_node_root(root, true);

  *internal_node_num_keys(root) = 1;
  *internal_node_child(root, 0) = left_child_page_num;

  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;

  *internal_node_right_child(root) = right_child_page_num;
}