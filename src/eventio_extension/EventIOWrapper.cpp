#include "EventIOWrapper.h"
#include "EventIOHandler.h"


thread_local EventIOHandler* eventiohandler = nullptr;
void EventIOHandler_init(const char* fname, const char mode, const char* url) {
    eventiohandler = new EventIOHandler(fname, mode, url);
}
void EventIOHandler_finalize() {
    delete eventiohandler;
}

int userfunction1(unsigned char* buffer, long size) {
    if(eventiohandler == nullptr) {
        return -1;
    }
    return eventiohandler->user_function1(buffer, size);
}
int userfunction2(unsigned char* buffer, long size) {
    if(eventiohandler == nullptr) {
        return -1;
    }
    return eventiohandler->user_function2(buffer, size);
}
int userfunction3(unsigned char* buffer, long size) {
    if(eventiohandler == nullptr) {
        return -1;
    }
    return eventiohandler->user_function3(buffer, size);
}
int userfunction4(unsigned char* buffer, long size) {
    if(eventiohandler == nullptr) {
        return -1;
    }
    return eventiohandler->user_function4(buffer, size);
}
int myuser_function(unsigned char* buffer, long size, int function_id) {
    if(function_id == 1) {
        return userfunction1(buffer, size);
    } else if(function_id == 2) {
        return userfunction2(buffer, size);
    } else if(function_id == 3) {
        return userfunction3(buffer, size);
    } else if(function_id == 4) {
        return userfunction4(buffer, size);
    }
    return -1;
}
