#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#include <mwe_ta.h>

#define TA_UUID TA_MWE_UUID

#define TA_FLAGS TA_FLAG_EXEC_DDR

#define TA_STACK_SIZE (3 * 1024)

#define TA_DATA_SIZE (25 * 1024 * 1024)

#define TA_VERSION "1.0"

#define TA_DESCRIPTION "WaTZ benchmarks: SQLite native TEE"

#endif /* USER_TA_HEADER_DEFINES_H */
