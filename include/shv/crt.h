#pragma once
#include <stddef.h>
// Explicit declarations for the opt-in freestanding subset. GCC's freestanding
// <cstdlib> intentionally omits hosted allocation declarations.
#ifdef __cplusplus
extern "C" {
#endif
void* malloc(size_t bytes);
void* calloc(size_t count,size_t bytes);
void* realloc(void* pointer,size_t bytes);
void free(void* pointer);
void* aligned_alloc(size_t alignment,size_t bytes);
void* memmove(void*,const void*,size_t);
int memcmp(const void*,const void*,size_t);
size_t strlen(const char*);
size_t strnlen(const char*,size_t);
int strcmp(const char*,const char*);
int strncmp(const char*,const char*,size_t);
char* strcpy(char*,const char*);
char* strncpy(char*,const char*,size_t);
char* strcat(char*,const char*);
char* strncat(char*,const char*,size_t);
#ifdef __cplusplus
}
#endif
