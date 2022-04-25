#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#include <wamr_ta.h>

#define TA_UUID TA_WAMR_UUID

#define TA_FLAGS TA_FLAG_EXEC_DDR

#define TA_STACK_SIZE (3 * 1024)

#ifndef TA_DATA_SIZE
#define TA_DATA_SIZE (12 * 1024 * 1024)
#endif

#define TA_VERSION "1.0"

#define TA_DESCRIPTION "Wasm Micro Runtime TrustZone implementation."

#define TA_CURRENT_TA_EXT_PROPERTIES \
  { "ch.unine.optee.wamr.property1", \
    USER_TA_PROP_TYPE_STRING, \
    "Foo bar" }, \
    { "ch.unine.optee.wamr.property2", \
	USER_TA_PROP_TYPE_U32, &(const uint32_t){ 0x0010 } }

#endif /* USER_TA_HEADER_DEFINES_H */
