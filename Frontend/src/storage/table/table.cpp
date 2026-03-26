
#include <vector>
#include <string>
#include "../../../headers/storage/table/table.h"
#include "../../../headers/storage/node/leaf_node.h"
#include "../../../headers/cursor/cursor.h"
#include "../../../headers/storage/b_plus_tree/b_plus_tree.h"

Table::Table(const std::string& name,
             const std::vector<std::string>& attrs,
             const std::string& filename)
    : name(name), attributes(attrs) {

    pager = Pager::pager_open(filename.c_str());
   

    if (pager->num_pages == 0) {
       void* root = pager->get_page(0);
        initializeLeafNode(root);
        root_page_num = 0;
    } else {
        root_page_num = 0;
    }
}


Cursor* Table::find(uint32_t key) {
    return B_Plus_Tree::internal_node_find(this, root_page_num, key);
}

void Table::insert(uint32_t key, const Row& value) {
    Cursor* cursor = find(key);

    void* node = pager->get_page(cursor->page_num);
    uint32_t num_cells = *leafNodeNumCells(node);

    // check duplicate
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leafNodeKey(node, cursor->cell_num);
        if (key_at_index == key) {
            std::cerr << "Duplicate key\n";
            delete cursor;
            return;
        }
    }

    return B_Plus_Tree::leaf_node_insert(cursor, key, &value);
    delete cursor;
}

 Table::~Table() {
    pager->pager_close();
};

