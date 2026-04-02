#include "../../../headers/storage/Pager/pager.h"

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

void* Pager::get_page(uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        std::cerr << "Tried to fetch page number out of bounds. "
                  << page_num << " >= " << TABLE_MAX_PAGES << "\n";
        std::exit(EXIT_FAILURE);
    }

    if (pages[page_num] == nullptr) {
        void* page = std::malloc(PAGE_SIZE);
        if (!page) {
            std::cerr << "Memory allocation failed\n";
            std::exit(EXIT_FAILURE);
        }

        uint32_t num_pages = file_length / PAGE_SIZE;
        if (file_length % PAGE_SIZE != 0) {
            num_pages += 1;
        }

        if (page_num < num_pages) {
            off_t offset = lseek(file_descriptor,
                                 page_num * PAGE_SIZE,
                                 SEEK_SET);

            if (offset == -1) {
                std::cerr << "Error seeking: "
                          << std::strerror(errno) << "\n";
                std::exit(EXIT_FAILURE);
            }

            ssize_t bytes_read =
                read(file_descriptor, page, PAGE_SIZE);

            if (bytes_read == -1) {
                std::cerr << "Error reading file: "
                          << std::strerror(errno) << "\n";
                std::exit(EXIT_FAILURE);
            }
        }

        pages[page_num] = page;

        if (page_num >= this->num_pages) {
            this->num_pages = page_num + 1;
        }
    }

    return pages[page_num];
}


void Pager::flush_page(uint32_t page_num) {
        if (pages[page_num] == nullptr) {
        std::cerr << "Tried to flush null page\n";
        std::exit(EXIT_FAILURE);
    }

    off_t offset = lseek(file_descriptor,
                         page_num * PAGE_SIZE,
                         SEEK_SET);

    if (offset == -1) {
        std::cerr << "Error seeking: "
                  << std::strerror(errno) << "\n";
        std::exit(EXIT_FAILURE);
    }

    ssize_t bytes_written =
        write(file_descriptor, pages[page_num],PAGE_SIZE);

    if (bytes_written == -1) {
        std::cerr << "Error writing: "
                  << std::strerror(errno) << "\n";
        std::exit(EXIT_FAILURE);
    }
      file_length = std::max(file_length, (page_num + 1) * PAGE_SIZE);
       dirty_pages_.erase(page_num); 
}

// flush_all_dirty: commit every dirty page to disk and clear the set.
void Pager::flush_all_dirty() {
    std::unordered_set<uint32_t> to_flush = std::move(dirty_pages_);
    dirty_pages_.clear();
    for (uint32_t pnum : to_flush)
        flush_page(pnum);
}

Pager* Pager::pager_open(const char* filename)
{
    int fd = open(filename,
                  O_RDWR | O_CREAT,
                  S_IWUSR | S_IRUSR);

    if (fd == -1) {
        std::cerr << "Unable to open file\n";
        std::exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager* pager = new Pager();
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = nullptr;
    }

    return pager;
}

uint32_t Pager::get_unused_page_num() {
  uint32_t page_num = this->num_pages;
  this->num_pages++;
  return page_num;
}

void Pager::pager_close() {
    if (file_descriptor == -1) return; 
    for (uint32_t i = 0; i < num_pages; i++) {
        if (pages[i] == nullptr) continue;

        flush_page(i);
        std::free(pages[i]);
        pages[i] = nullptr;
    }

    if (close(file_descriptor) == -1) {
        std::cerr << "Error closing file\n";
        std::exit(EXIT_FAILURE);
    }
   file_descriptor = -1; 
  
}


void Pager::mark_dirty(uint32_t page_num) {
    dirty_pages_.insert(page_num);
}
 
bool Pager::is_dirty(uint32_t page_num) const {
    return dirty_pages_.count(page_num) > 0;
}

Pager::~Pager() {
    pager_close();
}

