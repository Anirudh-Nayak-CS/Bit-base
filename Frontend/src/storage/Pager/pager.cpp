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

        if (file_length % PAGE_SIZE) {
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

        if(page_num>=num_pages) {
            num_pages=page_num+1;
        }
    }

    return pages[page_num];
}


void Pager::flush(uint32_t page_num) {
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
}