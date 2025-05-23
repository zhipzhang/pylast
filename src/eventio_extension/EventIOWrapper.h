#ifdef __cplusplus
extern "C" {
#include "stddef.h"
#endif
int EventIOHandler_init(const char* fname, const char mode, const char* url = "root://eos01.ihep.ac.cn");
int userfunction1(unsigned char* buffer, long size);
int userfunction2(unsigned char* buffer, long size);
int userfunction3(unsigned char* buffer, long size);
int userfunction4(unsigned char* buffer, long size);

int myuser_function(unsigned char* buffer, long size, int function_id);
void EventIOHandler_finalize();
#ifdef __cplusplus
}
#endif
