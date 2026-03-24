#include "../../headers/database/db.h"

bool dbClass::createTable(const std::string &name,
                          const std::vector<std::string> &attributes) {

  // 1. Check if table already exists
  if (tables.find(name) != tables.end()) {
    return false;
  }

  // 2. Create the empty table
  Table *new_table = new Table(name);

  // 3. Let the Table class wire up its own columns!
  for (const auto &attribute : attributes) {
    new_table->add_column(attribute);
  }

  // 4. Save to database
  tables[name] = new_table;

  return true;
}

bool dbClass::deleteTable(const std::string &name) {

  if (tables.find(name) == tables.end())
    return false;

  delete tables[name];
  tables.erase(name);
  return true;
}

Table *dbClass::selectTable(const std::string &name) {
  if (tables.find(name) == tables.end())
    return nullptr;

  Table *selectedTable = tables[name];
  return selectedTable;
}

Table* dbClass::db_open(const char* filename)
{
    Pager* pager = Pager::pager_open(filename);

    
    Table* table = new Table();
    table->pager = pager;
    table->root_page_num = 0;
    if (pager->num_pages == 0) {
    void* root_node = get_page(pager, 0);
    initialize_leaf_node(root_node);
  }
    return table;
}


void dbClass::db_close(Table* table)
{
    Pager* pager = table->pager;

    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (uint32_t i = 0; i < num_full_pages; i) {
        if (pager->pages[i] == nullptr) {
            continue;
        }

        pager_flush(pager, i, PAGE_SIZE);

        delete[] static_cast<char*>(pager->pages[i]);
        pager->pages[i] = nullptr;
    }

    // Handle partial page
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;

    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;

        if (pager->pages[page_num] != nullptr) {
            pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);

            delete[] static_cast<char*>(pager->pages[page_num]);
            pager->pages[page_num] = nullptr;
        }
    }

    int result = close(pager->file_descriptor);
    if (result == -1) {
        std::cerr << "Error closing db file.\n";
        std::exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i) {
        void* page = pager->pages[i];
        if (page != nullptr) {
            delete[] static_cast<char*>(page);
            pager->pages[i] = nullptr;
        }
    }

    delete pager;
    delete table;
}