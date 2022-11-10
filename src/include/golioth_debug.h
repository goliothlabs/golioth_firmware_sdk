#pragma once

#include "golioth_config.h"
#include "golioth_time.h"

typedef void* golioth_client_t;

typedef enum {
    GOLIOTH_DEBUG_LOG_LEVEL_NONE,
    GOLIOTH_DEBUG_LOG_LEVEL_ERROR,
    GOLIOTH_DEBUG_LOG_LEVEL_WARN,
    GOLIOTH_DEBUG_LOG_LEVEL_INFO,
    GOLIOTH_DEBUG_LOG_LEVEL_DEBUG,
    GOLIOTH_DEBUG_LOG_LEVEL_VERBOSE,
} golioth_debug_log_level_t;

void golioth_debug_set_log_level(golioth_debug_log_level_t level);
golioth_debug_log_level_t golioth_debug_get_log_level(void);
void golioth_debug_hexdump(const char* tag, const void* addr, int len);
void golioth_debug_set_client(golioth_client_t client);
void golioth_debug_printf(
        uint64_t tstamp_ms,
        golioth_debug_log_level_t level,
        const char* tag,
        const char* format,
        ...);
