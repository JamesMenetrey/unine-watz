#ifndef LOGGING_H
#define LOGGING_H

#include <trace.h>

#undef trace_printf_helper
#define trace_printf_helper(level, level_ok, ...) \
	trace_printf(__func__, __LINE__, (level), (level_ok), \
		     "[ATTESTER] " __VA_ARGS__)

/* Formatted trace tagged with TRACE_DEBUG level */
#if (!(TRACE_LEVEL < TRACE_DEBUG))
#define DEBUG_MESSAGE
#endif

#endif