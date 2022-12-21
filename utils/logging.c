#include "logging.h"

log_level_t g_level = LOG_QUIET;

void set_log_level(log_level_t level) { g_level = level; }
