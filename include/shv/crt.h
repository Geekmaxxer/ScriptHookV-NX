#pragma once
#include <stddef.h>
#include <stdarg.h>
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
// Buffer formatting only. See FORMATTED-TEXT.md for the supported conversions.
int snprintf(char* output,size_t capacity,const char* format,...);
int vsnprintf(char* output,size_t capacity,const char* format,va_list arguments);
// Integer parsing belongs to c_runtime. Decimal float parsing additionally
// requires numeric_runtime. See docs/reference/NUMERIC-PARSING.md.
long strtol(const char* input,char** end,int base);
unsigned long strtoul(const char* input,char** end,int base);
long long strtoll(const char* input,char** end,int base);
unsigned long long strtoull(const char* input,char** end,int base);
int atoi(const char* input);
long atol(const char* input);
long long atoll(const char* input);
double strtod(const char* input,char** end);
float strtof(const char* input,char** end);
double atof(const char* input);
#ifdef __cplusplus
}
#endif
