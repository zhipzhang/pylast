#include "EventIOHandler.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include "CompressionHandler/GzipHandler.h"
#include "CompressionHandler/ZstdHandler.h"
#include "CompressionHandler/LocalFileHandler.h"
#include "CompressionHandler/XRootDHandler.h"

EventIOHandler::EventIOHandler(const std::string& fname, const char mode,
                               const std::string& url) {
    if (fname.find("/eos") == 0) {
        fileHandler_ = std::make_unique<XRootDFileHandler>(url + fname, mode);
    } else {
        fileHandler_ = std::make_unique<LocalFileHandler>(fname, mode);
    }
    if (endsWith(fname, ".zst")) {
        compressionHandler_ = std::make_unique<ZstdHandler>(*fileHandler_);
    } else if (endsWith(fname, ".gz")) {
        compressionHandler_ = std::make_unique<GzipHandler>(*fileHandler_);
    } else {
        compressionHandler_ = nullptr;
    }
}
bool EventIOHandler::endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

size_t EventIOHandler::read(unsigned char* buffer, size_t size) {
    if (compressionHandler_ == nullptr) {
        return fileHandler_->read(buffer, size);
    } else {
        return compressionHandler_->read(buffer, size);
    }
}
size_t EventIOHandler::write(unsigned char* buffer, size_t size) {
    if (compressionHandler_ == nullptr) {
        return fileHandler_->write(buffer, size);
    } else {
        return compressionHandler_->write(buffer, size);
    }
}
int EventIOHandler::seek_current(size_t bytes) {
    /*
   * Check whether we can just use seek
   */
    if (compressionHandler_ == nullptr) {
        fileHandler_->seek(bytes, SEEK_CUR);
    } else {
        unsigned char tbuf[512];
        auto nbuf = bytes / 512;
        auto rbuf = bytes % 512;
        for (int i = 0; i < nbuf; i++) {
            compressionHandler_->read(tbuf, 512);
        }
        compressionHandler_->read(tbuf, rbuf);
    }
    return 0;
}
/*
 *
 * Write Method
 */
int EventIOHandler::user_function1(unsigned char* buffer, long size) {
    auto write_size = write(buffer, static_cast<size_t>(size));
    if (write_size == static_cast<size_t>(size)) {
        return 0;
    } else {
        return -1;
    }
}
/*
 *
 * find_io_block()
 */
int EventIOHandler::user_function2(unsigned char* buffer, long size) {
    /*
   * find the header and read bytes
   */
    const unsigned char sync_tag_byte[] = {0xD4, 0x1F, 0x8A, 0x37};
    if (size == 16) {
        long sync_count = 0;
        int block_found, byte_number, byte_order = 0;
        int rc;
        for (sync_count = -4L, block_found = byte_number = byte_order = 0;
             !block_found; sync_count++) {
            rc = read(buffer + byte_number, 1);
            if (rc <= 0) {
                if (fileHandler_->IsEnd()) {
                    return -2;
                } else {
                    return -1;
                }
            }
            if (byte_order == 0) {
                if (*buffer == sync_tag_byte[0]) {
                    byte_order = 1;
                } else if (*buffer == sync_tag_byte[3]) {
                    byte_order = -1;
                } else {
                    continue;
                }
                byte_number = 1;
            } else if (byte_order == 1) {
                if (buffer[byte_number] != sync_tag_byte[byte_number]) {
                    byte_number = byte_order = 0;
                    continue;
                }
                byte_number++;
            } else if (byte_order == -1) {
                if (buffer[byte_number] != sync_tag_byte[3 - byte_number]) {
                    byte_number = byte_order = 0;
                    continue;
                }
                byte_number++;
            }
            if (byte_number == 4) {
                block_found = 1;
            }
        }
        rc = read(buffer + 4, 12);
        if (rc != 12) {
            return -1;
        }
        if (sync_count > 0) {
            std::cout << "Skipping " << sync_count << " bytes \n";
        }
        return 16;
    } else if (size == 4) {

        int rc;
        rc = read(buffer, 4);
        if (rc != 4) {
            return -1;
        }
        return 4;
    } else {
        return -1;
    }
}
int EventIOHandler::user_function3(unsigned char* buffer, long size) {
    auto bytesread = read(buffer, static_cast<size_t>(size));
    if (bytesread == static_cast<size_t>(size)) {
        return 0;
    } else {
        if (fileHandler_->IsEnd()) {
            return -2;
        } else {
            return -1;
        }
    }
}
int EventIOHandler::user_function4(unsigned char* buffer, long size) {
    seek_current(static_cast<size_t>(size));
    if (fileHandler_->IsEnd()) {
        return -2;
    }
    return 0;
}
