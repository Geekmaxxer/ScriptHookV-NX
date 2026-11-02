#pragma once
// Shared configuration for the pinned yyjson implementation and its clients.
#define YYJSON_FREESTANDING 1
#define YYJSON_FREESTANDING_HEADER "string.h"
#define YYJSON_DISABLE_FILE 1
#define YYJSON_DISABLE_UTILS 1
#define YYJSON_DISABLE_INCR_READER 1
#define YYJSON_DISABLE_NON_STANDARD 1
#define YYJSON_READER_DEPTH_LIMIT 64
#define YYJSON_WRITER_DEPTH_LIMIT 64
