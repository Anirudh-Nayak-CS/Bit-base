#include "../../../headers/storage/table/table.h"
#include "../../../headers/storage/node/leaf_node.h"
#include "../../../headers/cursor/cursor.h"
#include "../../../headers/storage/b_plus_tree/b_plus_tree.h"
#include <iostream>
#include <string>

Table::Table(const std::string& name,
             const Schema&      schema,
             const std::string& filename)
    : name(name), schema(schema) {

    pager = Pager::pager_open(filename.c_str());

    if (pager->num_pages == 0) {
        void* root = pager->get_page(0);
        initializeLeafNode(root);
        set_node_root(root, true);
        root_page_num = 0;
    } else {
        root_page_num = 0;
    }
}

std::unique_ptr<Cursor> Table::find(uint32_t key) {
    return Cursor::table_find(this, key);
}

void Table::insert(uint32_t key, const void* data, uint32_t size) {
    auto cursor = find(key);

    void*    node      = pager->get_page(cursor->page_num);
    uint32_t num_cells = *leafNodeNumCells(node);

    // duplicate check
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leafNodeKey(node, cursor->cell_num);
        if (key_at_index == key) {
            std::cerr << "Duplicate key\n";
            return;
        }
    }

    // size guard — serialized row must fit in the fixed slot
    if (size > LEAF_NODE_VALUE_SIZE) {
        std::cerr << "Row too large to insert (" << size
                  << " bytes, max " << LEAF_NODE_VALUE_SIZE << ")\n";
        return;
    }

    B_Plus_Tree::leaf_node_insert(cursor.get(), key, data, size);
}

Table::~Table() {}