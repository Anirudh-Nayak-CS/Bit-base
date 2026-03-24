#include "../../headers/constants/constant.h"
#include "../../headers/table/table.h"
#include <functional>
#include <iostream>

void Table::add_column(const std::string &col_name) {
  column_order.push_back(col_name);
  Node *new_col = new Node(col_name);
  attributeHeaders[col_name] = new Column();

  attributeHeaders[col_name]->head = new_col;
  attributeHeaders[col_name]->tail = new_col;

  if (!attributeHead) {
    attributeHead = new_col;
    attributeTail = new_col;
  } else {
    attributeTail->right = new_col;
    new_col->left = attributeTail;
    attributeTail = new_col;
  }
}

void Table::add_row(const std::vector<std::string> &values) {
  if (values.size() != attributeHeaders.size()) {
    std::cout << "Number of values does not match the number of columns."
              << std::endl;
    throw Commandstatus::CMD_FAILURE;
  }

  Node *prev_node = nullptr;
  Node *row_head = nullptr;
  for (size_t i = 0; i < values.size(); ++i) {

    const std::string &col_name = column_order[i];

    Node *new_node = new Node(values[i]);
    Column *col = attributeHeaders[col_name];

    // Link vertically
    if (col->tail) {
      col->tail->down = new_node;
      new_node->up = col->tail;
    }
    col->tail = new_node;

    // Link horizontally
    if (prev_node) {
      prev_node->right = new_node;
      new_node->left = prev_node;
    } else {
      row_head = new_node;
    }
    prev_node = new_node;
  }

  // Increment the global row counter and add to the map
  int row_id = ++globalRowCounter;
  rowMap[row_id] = row_head;

  // Adding the row ID to the B+ -tree
  bTree.insertEntry(, row_id);
}

void Table::delete_row(int row_id) {

  auto it = rowMap.find(row_id);
  if (it == rowMap.end()) {
    std::cout << "Row ID not found." << std::endl;
    return;
  }

  Node *current = it->second;
  int i = 0;

  // Delete the row from the column structures
  while (current) {
    Node *next = current->right;

    std::string col_name = column_order[i];
    Column *col = attributeHeaders[col_name];

    // 1. Vertical Link: Update Head or Up-Pointer
    if (!current->up) {
      col->head = current->down; // deleting the top row, move head down
    } else {
      current->up->down = current->down;
    }

    // 2. Vertical Link: Update Tail or Down-Pointer
    if (!current->down) {
      col->tail = current->up; // deleting the bottom row, move tail up
    } else {
      current->down->up = current->up;
    }

    delete current;
    current = next;
    ++i; // Move to the next column name
  }

  rowMap.erase(it);

  // bTree.remove(row_id);
}

void Table::update_row(const std::function<bool(const Node *)> &condition,
                       const std::vector<std::string> &new_values) {

  for (auto &[row_id, row_head] : rowMap) {
    Node *current = row_head;

    // Check if the row satisfies the condition
    if (condition(current)) {
      int i = 0;
      while (current && i < new_values.size()) {
        current->data = new_values[i];
        current = current->right;
        ++i;
      }
    }
  }
}

void Table::delete_column(const std::string &col_name) {
  // 1. Check if the column actually exists
  auto it = attributeHeaders.find(col_name);
  if (it == attributeHeaders.end()) {
    std::cout << "Column '" << col_name << "' not found." << std::endl;
    return;
  }

  // 2. Find the index of the column to delete
  int col_index = -1;
  for (size_t i = 0; i < column_order.size(); ++i) {
    if (column_order[i] == col_name) {
      col_index = i;
      break;
    }
  }

  // handling deletion of the first column
  if (col_index == 0) {
    for (auto &[row_id, row_head] : rowMap) {
      if (row_head) {
        rowMap[row_id] = row_head->right;
      }
    }
  }

  // 4. Traverse down the column and surgically remove nodes
  Column *col = it->second;
  Node *current = col->head;

  while (current) {
    Node *next_down = current->down;
    // Fix the Left-to-Right links
    if (current->left) {
      current->left->right = current->right;
    } else if (current == attributeHead) {

      attributeHead = current->right;
    }

    // Fix the Right-to-Left links
    if (current->right) {
      current->right->left = current->left;
    } else if (current == attributeTail) {

      attributeTail = current->left;
    }

    delete current;
    current = next_down;
  }

  delete it->second;
  attributeHeaders.erase(it);
  column_order.erase(column_order.begin() + col_index);
}