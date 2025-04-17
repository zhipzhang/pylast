#pragma once

#include <memory>
#include <string>
#include "CompressionHandler/CompressionHandler.h"
#include "CompressionHandler/FileHandler.h"

/*
 * Try to provide the user_function for the eventio format.
 */
class EventIOHandler {
   public:
    EventIOHandler(const std::string& fname, const char mode,
                   const std::string& url = "root://eos01.ihep.ac.cn");
    ~EventIOHandler()
    {
        compressionHandler_->close();
    }
    /*
   * user_function defined in eventio_en.pdf
   */
    int user_function1(unsigned char* buffer, long bytes);
    int user_function2(unsigned char* buffer, long bytes);
    int user_function3(unsigned char* buffer, long bytes);
    int user_function4(unsigned char* buffer, long bytes);

   private:
    size_t read(unsigned char* buffer, size_t bytes);
    size_t write(unsigned char* buffer, size_t bytes);
    int seek_current(size_t bytes);
    static bool endsWith(const std::string& str, const std::string& suffix);
    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<CompressionHandler> compressionHandler_;
};
